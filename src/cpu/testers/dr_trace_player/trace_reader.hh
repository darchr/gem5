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

#ifndef __CPU_TESTERS_DR_TRACE_PLAYER_TRACE_READER_HH__
#define __CPU_TESTERS_DR_TRACE_PLAYER_TRACE_READER_HH__

/**
 * @file
 * Contains the reader for dynamario traces
 * See https://dynamorio.org/sec_drcachesim_format.html for details on the
 * trace format.
 * This file reimplements some of the code from drcachesim. See the dynamario
 * git repo for the original code. This is built using version 9.0.19202.
 * https://github.com/DynamoRIO/dynamorio
 */

#include <zlib.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include "params/DRTraceReader.hh"
#include "sim/sim_object.hh"

namespace gem5
{


/**
 * An object which reads Dynamorio traces.
 * Note that this is not a complete reader. It is currently only designed to
 * read the google workload traces
 * https://dynamorio.org/google_workload_traces.html
 */
class DRTraceReader : public SimObject
{
  public:
    struct TraceRef
    {
        // Only used for instruction references
        // (Note: we could add this to memory references in the future)
        Addr pc = 0;
        // Memory address referenced. Only used for memory references
        Addr addr = 0;
        unsigned int size = 0;
        // True if this is an instruction reference, false if memory reference
        bool isInst = false;
        // True if this reference is valid. An invalid reference means the
        // stream is over.
        bool isValid = false;
        // Thread id of the reference
        int tid = 0;
        // See dynamorio/clients/drcachesim/common/trace_entry.h for details
        // There is not necessarily a 1-to-1 mapping with TRACE_TYPE
        enum
        {
            // Inst types
            GENERIC_INST,
            DIRECT_JUMP,
            INDIRECT_JUMP,
            CONDITIONAL_JUMP,
            DIRECT_CALL,
            INDIRECT_CALL,
            RETURN,
            // memory types
            READ,
            WRITE,
            PREFETCH
        } type = {};
    };

  private:
    /**
     * Slightly modified trace entry from dynamorio.
     * See dynamorio/clients/drcachesim/common/trace_entry.h for original.
     * This is compatible with "version 3" of the trace
     */
    struct GEM5_PACKED DRTraceEntry
    {
        unsigned short type;
        unsigned short size;
        uint64_t addr;
    };

  private:

    /// The directory which contains the trace files (one per thread)
    std::filesystem::path directory;
    /// All of the filenames of the trace files
    std::vector<std::string> filenames;

    /// The open trace files (one per thread)
    std::vector<gzFile> traceFiles;

    /// The current timestamps for each trace file. Should be the same length
    /// as the traceFiles above.
    std::vector<uint64_t> timestamps;

    /// The current thread being executed on each player. If -1, then nothing
    /// is executing on that player
    std::vector<int> currentTids;

    /**
     * @brief Get the Next Entry object for the give thread id
     *
     * @param tid The thread id of the trace file to get
     * @return TraceEntry
     */
    DRTraceEntry _getNextEntry(unsigned int tid);

    unsigned int
    getLowestTimestampThread()
    {
        return std::distance(timestamps.begin(),
            std::min_element(timestamps.begin(), timestamps.end()));
    }

    enum
    {
        TRACE_TYPE_READ,
        TRACE_TYPE_WRITE,
        TRACE_TYPE_PREFETCH,
        TRACE_TYPE_PREFETCHT0,
        TRACE_TYPE_PREFETCH_READ_L1 =
            TRACE_TYPE_PREFETCHT0,
        TRACE_TYPE_PREFETCHT1,
        TRACE_TYPE_PREFETCH_READ_L2 =
            TRACE_TYPE_PREFETCHT1,
        TRACE_TYPE_PREFETCHT2,
        TRACE_TYPE_PREFETCH_READ_L3 =
            TRACE_TYPE_PREFETCHT2,
        TRACE_TYPE_PREFETCHNTA,
        TRACE_TYPE_PREFETCH_READ,
        TRACE_TYPE_PREFETCH_WRITE,
        TRACE_TYPE_PREFETCH_INSTR,
        TRACE_TYPE_INSTR,
        TRACE_TYPE_INSTR_DIRECT_JUMP,
        TRACE_TYPE_INSTR_INDIRECT_JUMP,
        TRACE_TYPE_INSTR_CONDITIONAL_JUMP,
        TRACE_TYPE_INSTR_DIRECT_CALL,
        TRACE_TYPE_INSTR_INDIRECT_CALL,
        TRACE_TYPE_INSTR_RETURN,
        TRACE_TYPE_INSTR_BUNDLE,
        TRACE_TYPE_INSTR_FLUSH,
        TRACE_TYPE_INSTR_FLUSH_END,
        TRACE_TYPE_DATA_FLUSH,
        TRACE_TYPE_DATA_FLUSH_END,
        TRACE_TYPE_THREAD,
        TRACE_TYPE_THREAD_EXIT,
        TRACE_TYPE_PID,
        TRACE_TYPE_HEADER,
        TRACE_TYPE_FOOTER,
        TRACE_TYPE_HARDWARE_PREFETCH,
        TRACE_TYPE_MARKER,
        TRACE_TYPE_INSTR_NO_FETCH,
        TRACE_TYPE_INSTR_MAYBE_FETCH,
        TRACE_TYPE_INSTR_SYSENTER,
        TRACE_TYPE_PREFETCH_READ_L1_NT,
        TRACE_TYPE_PREFETCH_READ_L2_NT,
        TRACE_TYPE_PREFETCH_READ_L3_NT,
        TRACE_TYPE_PREFETCH_INSTR_L1,
        TRACE_TYPE_PREFETCH_INSTR_L1_NT,
        TRACE_TYPE_PREFETCH_INSTR_L2,
        TRACE_TYPE_PREFETCH_INSTR_L2_NT,
        TRACE_TYPE_PREFETCH_INSTR_L3,
        TRACE_TYPE_PREFETCH_INSTR_L3_NT,
        TRACE_TYPE_PREFETCH_WRITE_L1,
        TRACE_TYPE_PREFETCH_WRITE_L1_NT,
        TRACE_TYPE_PREFETCH_WRITE_L2,
        TRACE_TYPE_PREFETCH_WRITE_L2_NT,
        TRACE_TYPE_PREFETCH_WRITE_L3,
        TRACE_TYPE_PREFETCH_WRITE_L3_NT,
    };
    enum
    {
        TRACE_MARKER_TYPE_KERNEL_EVENT,
        TRACE_MARKER_TYPE_KERNEL_XFER,
        TRACE_MARKER_TYPE_TIMESTAMP,
        TRACE_MARKER_TYPE_CPU_ID,
        TRACE_MARKER_TYPE_FUNC_ID,
        TRACE_MARKER_TYPE_FUNC_RETADDR,
        TRACE_MARKER_TYPE_FUNC_ARG,
        TRACE_MARKER_TYPE_FUNC_RETVAL,
        TRACE_MARKER_TYPE_SPLIT_VALUE,
        TRACE_MARKER_TYPE_FILETYPE,
        TRACE_MARKER_TYPE_CACHE_LINE_SIZE,
        TRACE_MARKER_TYPE_INSTRUCTION_COUNT,
        TRACE_MARKER_TYPE_VERSION,
        TRACE_MARKER_TYPE_RSEQ_ABORT,
        TRACE_MARKER_TYPE_WINDOW_ID,
        TRACE_MARKER_TYPE_PHYSICAL_ADDRESS,
        TRACE_MARKER_TYPE_PHYSICAL_ADDRESS_NOT_AVAILABLE,
        TRACE_MARKER_TYPE_VIRTUAL_ADDRESS,
        TRACE_MARKER_TYPE_PAGE_SIZE,
        TRACE_MARKER_TYPE_RESERVED_END = 100,
    };

  public:
    PARAMS(DRTraceReader);
    DRTraceReader(const Params &params);

    void init() override;

    ~DRTraceReader()
    {
        for (auto &file : traceFiles) {
            gzclose(file);
        }
    }

    /**
     * Used by the DRTracePlayers to get the next traced instruction
     *
     */
    TraceRef getNextTraceReference(unsigned player_id);


};

} // namespace gem5


#endif //__CPU_TESTERS_DR_TRACE_PLAYER_TRACE_READER_HH__
