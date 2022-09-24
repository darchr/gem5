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

#include "cpu/probes/looppoint.hh"

namespace gem5
{

LoopPoint::LoopPoint(const LoopPointParams &p)
    : ProbeListenerObject(p),
    cpuptr(p.core),
    manager(p.lpmanager)
{
    if (!cpuptr || !manager) {
        fatal("%s is NULL", !cpuptr ? "CPU":"LoopPointManager");
    }
    // It loops through the target_pc vector param to construct the targetPC 
    // set. Only the unseen PC will be inserted in the targetPC set.
    for (int i = 0; i< p.target_pc.size(); i++) {
        if(p.target_pc[i]!= 0 && targetPC.find(p.target_pc[i]) == targetPC.end()) {
            targetPC.insert(p.target_pc[i]);
        }
    }
}

LoopPoint::~LoopPoint()
{}

void
LoopPoint::init()
{}

void
LoopPoint::regProbeListeners()
{
    typedef ProbeListenerArg<LoopPoint, Addr> LoopPointListener;
    listeners.push_back(new LoopPointListener(this, "RetiredInstsPC",
                                             &LoopPoint::check_pc));
}

void
LoopPoint::check_pc(const Addr& pc)
{
    if(targetPC.find(pc) != targetPC.end()) {
        manager->check_count(pc);
    }
}

}
