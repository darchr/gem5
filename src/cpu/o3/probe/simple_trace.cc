/*
 * Copyright (c) 2013 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
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

#include "cpu/o3/probe/simple_trace.hh"

#include "base/trace.hh"
#include "cpu/o3/dyn_inst.hh"
#include "debug/SimpleTrace.hh"

namespace gem5
{

namespace o3
{

SimpleTrace::SimpleTrace(const SimpleTraceParams &params) :
    ProbeListenerObject(params),
    resetCalls(0), dumpCalls(0),
    startResetCall(params.start_on_ith_stats_reset),
    stopDumpCall(params.stop_on_ith_stats_dump),
    doAttach(true), fetch(params.get_fetch), commit(params.get_commit)
{
    if (params.handle_trace_collection_manually) {
        doAttach = false;
        statistics::registerResetCallback([this] () { handleResetStats(); });
        statistics::registerDumpCallback([this] () { handleDumpStats(); });
    }
}

void
SimpleTrace::handleResetStats()
{
    resetCalls++;
    if (resetCalls == startResetCall) {
        doAttach = true;
        regProbeListeners();
    }
}

void
SimpleTrace::handleDumpStats()
{
    dumpCalls++;
    if (dumpCalls == stopDumpCall) {
        doAttach = false;
        unregProbeListeners();
    }
}

void
SimpleTrace::traceCommit(const DynInstConstPtr& dynInst)
{
    DPRINTFR(SimpleTrace, "[%s]: Commit 0x%08x %s.\n", name(),
             dynInst->pcState().instAddr(),
             dynInst->staticInst->disassemble(dynInst->pcState().instAddr()));
}

void
SimpleTrace::traceFetch(const DynInstConstPtr& dynInst)
{
    DPRINTFR(SimpleTrace, "[%s]: Fetch 0x%08x %s.\n", name(),
             dynInst->pcState().instAddr(),
             dynInst->staticInst->disassemble(dynInst->pcState().instAddr()));
}

void
SimpleTrace::regProbeListeners()
{
    typedef ProbeListenerArg<SimpleTrace,
            DynInstConstPtr> DynInstListener;
    if (doAttach && fetch) {
        listeners.push_back(new DynInstListener(this, "Fetch",
                &SimpleTrace::traceFetch));
    }
    if (doAttach && commit) {
        listeners.push_back(new DynInstListener(this, "Commit",
                &SimpleTrace::traceCommit));
    }
}

} // namespace o3
} // namespace gem5
