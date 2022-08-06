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

#include "cpu/testers/dr_trace_player/trace_player.hh"

#include "base/trace.hh"
#include "debug/DRTrace.hh"

namespace gem5
{

DRTracePlayer::DRTracePlayer(const Params &params) :
    ClockedObject(params),
    onClockEvent([this]{ tryExecuteInsts(); }, name()+"clock_event"),
    reader(params.reader)
{

}

void
DRTracePlayer::startup()
{
    schedule(onClockEvent, 0);
}

void
DRTracePlayer::tryExecuteInsts()
{
    DRTraceReader::TraceRef next_ref = reader->getNextTraceReference(0);

    DPRINTF(DRTrace, "Got reference pc: %0#x, addr: %0#x, size: %d, "
                     "%s, type: %d, valid: %d\n", next_ref.pc, next_ref.addr,
                      next_ref.size, next_ref.isInst ? "inst" : "memory",
                      next_ref.type, next_ref.isValid);
    schedule(onClockEvent, nextCycle());
}

} // namespace gem5
