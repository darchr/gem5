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

#include "cpu/simple/probes/looppoint_analysis.hh"


namespace gem5
{

LooppointAnalysis::LooppointAnalysis(const LooppointAnalysisParams &p)
    : ProbeListenerObject(p),
      manager(p.ptmanager),
      validAddrLowerBound(p.validAddrRangeStart),
      validAddrUpperBound(p.validAddrRangeStart+p.validAddrRangeSize)
{
    DPRINTF(LooppointAnalysis, "the valid address range start from %i to "
    " %i \n", validAddrLowerBound, validAddrUpperBound);
}

void
LooppointAnalysis::regProbeListeners()
{
    // connect the probe listener with the probe "RetriedInstsPC" in the
    // corresponding core.
    // when "RetiredInstsPC" notifies the probe listener, then the function
    // 'check_pc' is automatically called
    typedef ProbeListenerArg<LooppointAnalysis, std::pair<SimpleThread*,StaticInstPtr>>
     LooppointAnalysisListener;
    listeners.push_back(new LooppointAnalysisListener(this, "Commit",
                                             &LooppointAnalysis::checkPc));
}

void
LooppointAnalysis::checkPc(const std::pair<SimpleThread*, StaticInstPtr>& p) {
    SimpleThread* thread = p.first;
    const StaticInstPtr &inst = p.second;
    auto &pcstate = thread->getTC()->pcState().as<GenericISA::PCStateWithNext>();
    if (inst->isMicroop() && !inst->isLastMicroop())
        return;
    if(validAddrUpperBound!=0) {
        if(pcstate.pc() < validAddrLowerBound || pcstate.pc() > validAddrUpperBound)
            return;
    }
    if (inst->isControl() && inst-> isDirectCtrl() && thread->getIsaPtr()->inUserMode()) {
        if(pcstate.npc() < pcstate.pc())
            manager->countPc(pcstate.npc());
    }
} 

LooppointAnalysisManager::LooppointAnalysisManager(const LooppointAnalysisManagerParams &p)
    : SimObject(p),
    currentPc(0)
{
    
}

void
LooppointAnalysisManager::countPc(const Addr pc)
{
    if (counter.find(pc) == counter.end()){
        counter.insert(std::make_pair(pc,0));
    }
    else{
        ++counter.find(pc)->second;
    }
    currentPc = pc;
    while (mostRecentPc.size() >= 5) {
        mostRecentPc.pop();
    }
    mostRecentPc.push(pc);
}

}
