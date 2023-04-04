/*
 * Copyright (c) 2023 The Regents of the University of California.
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

#include "cpu/simple/probes/pc_count_anaylsis.hh"


namespace gem5
{

PcCountAnalsis::PcCountAnalsis(const PcCountAnalsisParams &p)
    : ProbeListenerObject(p),
      manager(p.ptmanager)
{
    
}

void
PcCountAnalsis::regProbeListeners()
{
    // connect the probe listener with the probe "RetriedInstsPC" in the
    // corresponding core.
    // when "RetiredInstsPC" notifies the probe listener, then the function
    // 'check_pc' is automatically called
    typedef ProbeListenerArg<PcCountAnalsis, std::pair<SimpleThread*,StaticInstPtr>>
     PcCountAnalsisListener;
    listeners.push_back(new PcCountAnalsisListener(this, "Commit",
                                             &PcCountAnalsis::checkPc));
}

void
PcCountAnalsis::checkPc(const std::pair<SimpleThread*, StaticInstPtr>& p) {
    SimpleThread* thread = p.first;
    const StaticInstPtr &inst = p.second;
    if (inst->isMicroop() && !inst->isLastMicroop())
        return;
    if (inst->isControl() && inst-> isDirectCtrl() && thread->getIsaPtr()->inUserMode()) {
        auto &pcstate = thread->getTC()->pcState().as<GenericISA::PCStateWithNext>();
        if(pcstate.npc() < pcstate.pc())
            manager->dosth(pcstate.npc());
    }

} 

PcCountAnalsisManager::PcCountAnalsisManager(const PcCountAnalsisManagerParams &p)
    : SimObject(p)
{
    
}

void
PcCountAnalsisManager::dosth(const Addr pc)
{
    if (counter.find(pc) == counter.end()){
        counter.insert(std::make_pair(pc,0));
    }
    else{
        ++counter.find(pc)->second;
    }
}

}
