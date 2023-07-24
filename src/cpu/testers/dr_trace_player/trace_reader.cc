/*
 * Copyright (c) 2022 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cpu/testers/dr_trace_player/trace_reader.hh"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <zlib.h>

#include <filesystem>

#include "base/trace.hh"
#include "debug/DRTrace.hh"

namespace gem5
{

DRTraceReader::DRTraceReader(const Params &params) :
    SimObject(params),
    directory(params.directory)
{
    // Check to make sure the directory exists and has files in it named
    // *.memtrace.gz
    fatal_if(!is_directory(directory), "DRTraceReader requires the "
             "directory parameter to point to valid directory. %s is not a"
             "directory", directory.string().c_str());

    for (auto const& dir_entry :
        std::filesystem::directory_iterator{directory}) {
        // get all files that end with ".memtrace.gz"
        std::filesystem::path p(dir_entry);
        if (p.extension().string() == std::string(".gz") &&
            p.stem().extension().string() == std::string(".memtrace")) {
            filenames.emplace_back(p.string());
        }
    }

    fatal_if(filenames.empty(), "Did not find any trace files in %s. Make sure"
             " you pass a directory which has files *.memtrace.gz to "
             "DRTraceReader.", directory.string().c_str());

    DPRINTF(DRTrace, "Found %d trace files in %s\n",
            filenames.size(), directory.string().c_str());

    currentTids.resize(params.num_players, -1);
}

// TODO: Close all files on exit

void
DRTraceReader::init()
{
    // open the files
    for (auto const &file : filenames) {
        gzFile fdz = gzopen(file.c_str(), "rb");
        fatal_if(!fdz, "Could not open the file %s.", file);
        traceFiles.push_back(fdz);
    }
    timestamps.resize(traceFiles.size(), 0);

    // Get the first timestamp in each file.
    for (int i = 0; i < traceFiles.size(); i++) {
        DRTraceEntry raw_entry = _getNextEntry(i);
        while (raw_entry.size != TRACE_MARKER_TYPE_TIMESTAMP) {
            // Keep getting entries until we get the first timestamp
            raw_entry = _getNextEntry(i);
        }
        timestamps[i] = raw_entry.addr; // set the timestamp
    }
}

DRTraceReader::DRTraceEntry
DRTraceReader::_getNextEntry(unsigned int tid)
{
    panic_if(tid > traceFiles.size(), "tid (%d) out of range.", tid);
    auto bytes = sizeof(DRTraceReader::DRTraceEntry);
    DRTraceReader::DRTraceEntry entry{};
    auto bytes_read = gzread(traceFiles[tid], (void*)&entry, bytes);
    assert(bytes_read == bytes);
    return entry;
}

DRTraceReader::TraceRef
DRTraceReader::getNextTraceReference(unsigned player_id)
{
    assert(player_id < currentTids.size());

    TraceRef ref;

    int cur_tid = currentTids[player_id];
    if (cur_tid < 0) {
        // Nothing executing on this player. See if there's something else
        // to execute
        unsigned int next_tid = getLowestTimestampThread();
        if (next_tid == std::numeric_limits<unsigned int>::max()) {
            // nothing else to do, return invalid reference
            return ref;
        }
        currentTids[player_id] = next_tid;
        return getNextTraceReference(player_id);
    }

    DRTraceReader::DRTraceEntry raw_entry = _getNextEntry(cur_tid);

    switch (raw_entry.type) {
        case TRACE_TYPE_MARKER:
            if (raw_entry.size == TRACE_MARKER_TYPE_TIMESTAMP) {
                uint64_t delta = raw_entry.addr - timestamps[cur_tid];
                // Switch???
                if (delta > 0) {
                    // for now, assert true as we shouldn't see the same
                    // timestamp twice. In the future, we want to make this
                    // delta check a parameter.
                    timestamps[cur_tid] = raw_entry.addr;
                    unsigned int next_tid = getLowestTimestampThread();
                    if (next_tid == std::numeric_limits<unsigned int>::max()) {
                        // nothing else to do
                        currentTids[player_id] = -1;
                    } else {
                        currentTids[player_id] = next_tid;
                        // Use this new TID to get the next instruction
                        return getNextTraceReference(player_id);
                    }
                }
            } else {
                warn_once("Skipping unknown marker type: %d, value: %d.\n",
                          raw_entry.size, raw_entry.addr);
                return getNextTraceReference(player_id);
            }
            break;
        case TRACE_TYPE_READ:
            ref.addr = raw_entry.addr;
            ref.size = raw_entry.size;
            ref.isValid = true;
            ref.type = TraceRef::READ;
            break;
        case TRACE_TYPE_WRITE:
            ref.addr = raw_entry.addr;
            ref.size = raw_entry.size;
            ref.isValid = true;
            ref.type = TraceRef::WRITE;
            break;
        case TRACE_TYPE_PREFETCH:
        case TRACE_TYPE_PREFETCH_READ_L1:
        case TRACE_TYPE_PREFETCHT1:
        case TRACE_TYPE_PREFETCHT2:
        case TRACE_TYPE_PREFETCHNTA:
            if (raw_entry.addr == 0) {
                warn("Encountered a prefetch req with Addr = 0 \n");
                return getNextTraceReference(player_id);                
            }
            ref.addr = raw_entry.addr;
            ref.size = raw_entry.size;
            ref.isValid = true;
            ref.type = TraceRef::PREFETCH;
            break;
        case TRACE_TYPE_INSTR:
        case TRACE_TYPE_INSTR_SYSENTER:
        case TRACE_TYPE_INSTR_NO_FETCH:
            ref.pc = raw_entry.addr;
            ref.size = raw_entry.size;
            ref.isValid = true;
            ref.type = TraceRef::GENERIC_INST;
            break;
        case TRACE_TYPE_INSTR_DIRECT_JUMP:
            ref.pc = raw_entry.addr;
            ref.size = raw_entry.size;
            ref.isValid = true;
            ref.type = TraceRef::DIRECT_JUMP;
            break;
        case TRACE_TYPE_INSTR_INDIRECT_JUMP:
            ref.pc = raw_entry.addr;
            ref.size = raw_entry.size;
            ref.isValid = true;
            ref.type = TraceRef::INDIRECT_JUMP;
            break;
        case TRACE_TYPE_INSTR_CONDITIONAL_JUMP:
            ref.pc = raw_entry.addr;
            ref.size = raw_entry.size;
            ref.isValid = true;
            ref.type = TraceRef::CONDITIONAL_JUMP;
            break;
        case TRACE_TYPE_INSTR_DIRECT_CALL:
            ref.pc = raw_entry.addr;
            ref.size = raw_entry.size;
            ref.isValid = true;
            ref.type = TraceRef::DIRECT_CALL;
            break;
        case TRACE_TYPE_INSTR_INDIRECT_CALL:
            ref.pc = raw_entry.addr;
            ref.size = raw_entry.size;
            ref.isValid = true;
            ref.type = TraceRef::INDIRECT_CALL;
            break;
        case TRACE_TYPE_INSTR_RETURN:
            ref.pc = raw_entry.addr;
            ref.size = raw_entry.size;
            ref.isValid = true;
            ref.type = TraceRef::RETURN;
            break;
        case TRACE_TYPE_HEADER:
            DPRINTF(DRTrace, "Found header in trace. Version %d\n",
                    raw_entry.addr);
            panic_if(raw_entry.addr != 3,
                     "DRTraceReader only works with version 3");
            return getNextTraceReference(player_id);
            break;
        case TRACE_TYPE_FOOTER:
        case TRACE_TYPE_THREAD_EXIT:
            DPRINTF(DRTrace, "Found footer or thread exit in trace\n");
            // TODO: Close the file
            // Return an invalid reference since we're at the end
            break;
        case TRACE_TYPE_THREAD:
            DPRINTF(DRTrace, "Found thread %d\n", raw_entry.addr);
            return getNextTraceReference(player_id);
        case TRACE_TYPE_PID:
            DPRINTF(DRTrace, "Found pid %d\n", raw_entry.addr);
            return getNextTraceReference(player_id);
        default:
            panic("Unknown trace type %d\n", raw_entry.type);
    }

    return ref;
}

} // namespace gem5
