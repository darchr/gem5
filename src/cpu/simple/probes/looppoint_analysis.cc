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
      validAddrUpperBound(p.validAddrRangeStart+p.validAddrRangeSize),
      localInstCounter(0),
      BBInstCounter(0)
{
    DPRINTF(LooppointAnalysis, "the valid address range start from %i to "
    " %i \n", validAddrLowerBound, validAddrUpperBound);
}

void
LooppointAnalysis::startListening()
{
    if (listeners.empty()) {
        listeners.push_back(new LooppointAnalysisListener(this, "Commit",
                                                &LooppointAnalysis::checkPc));
    }
    DPRINTF(LooppointAnalysis, "Current size of listener: %i\n",
                                                    listeners.size());
}

void
LooppointAnalysis::stopListening()
{
    for (auto l = listeners.begin(); l != listeners.end(); ++l) {
        delete (*l);
    }
    listeners.clear();
    DPRINTF(LooppointAnalysis, "Current size of listener: %i\n",
                                                    listeners.size());
}

void
LooppointAnalysis::updateMostRecentPcCount(Addr npc) {
    int count;
    count=manager->getPcCount(npc);
    if (count==-1){
        count = 1;
    }
    PcCountPair pair = PcCountPair(npc,count);
    auto it = std::find_if(localMostRecentPcCount.begin(),
                                localMostRecentPcCount.end(),
                                [&pair](const std::pair<PcCountPair,Tick>& p)
                                { return p.first.getPC()==pair.getPC();});
    if (it == localMostRecentPcCount.end()) {
        while (localMostRecentPcCount.size() >= 5) {
            localMostRecentPcCount.pop_back();
        }
        localMostRecentPcCount.push_front(
                            std::make_pair(pair, curTick()));
    } else {
        if (it != localMostRecentPcCount.begin()) {
            localMostRecentPcCount.push_front(*it);
            localMostRecentPcCount.erase(it);
        }
        it->first = pair;
        it->second = curTick();
    }
}

void
LooppointAnalysis::checkPc(const std::pair<SimpleThread*, StaticInstPtr>& p) {
    SimpleThread* thread = p.first;
    const StaticInstPtr &inst = p.second;
    auto &pcstate =
                thread->getTC()->pcState().as<GenericISA::PCStateWithNext>();

    if (inst->isMicroop() && !inst->isLastMicroop())
        return;

    if (validAddrUpperBound!=0 && (pcstate.pc() < validAddrLowerBound ||
                                        pcstate.pc() > validAddrUpperBound)) {
    // If there is a valid address range
    // and the current PC is outside of the valid address range
    // then we discard it
        return;
    }

    if (!thread->getIsaPtr()->inUserMode()) {
        return;
    }

    if (!BBInstCounter) {
        BBstart = pcstate.pc();
    }

    localInstCounter ++;
    BBInstCounter ++;

    if (inst->isControl()){
        if (BBfreq.find(BBstart)!=BBfreq.end()) {
            ++BBfreq.find(BBstart)->second;
        } else {
            BBfreq.insert(std::make_pair(BBstart,1));
        }
        if (encountered_PC.find(BBstart)==encountered_PC.end()) {
            encountered_PC.insert(BBstart);
            manager->updateBBinst(BBstart, BBInstCounter);
        }
        BBInstCounter = 0;

        if (inst->isDirectCtrl()) {
            // If the current instruction is a branch instruction and a
            //User mode
            // instuction, then we check if it jumps backward
            if (pcstate.npc() < pcstate.pc()){
                // If the current branch instruction jumps backward
                // we send the destination PC of this branch to the manager
                updateMostRecentPcCount(pcstate.npc());
                manager->countPc(pcstate.npc(), localInstCounter);
                localInstCounter = 0;
            }
        }
    }
}

LooppointAnalysisManager::LooppointAnalysisManager(
    const LooppointAnalysisManagerParams &p)
    : SimObject(p),
    regionLength(p.regionLen),
    globalInstCounter(0)
{
    DPRINTF(LooppointAnalysis, "The region length is %i\n", regionLength);
}

void
LooppointAnalysisManager::countPc(Addr pc, int instCount)
{
    if (counter.find(pc) == counter.end()){
        // If the PC is not in the counter
        // then we insert it with a count of 1
        counter.insert(std::make_pair(pc,1));
    }
    else{
        ++counter.find(pc)->second;
    }

    globalInstCounter += instCount;
    if (globalInstCounter >= regionLength) {
        exitSimLoopNow("simpoint starting point found");
    }
}

void
LooppointAnalysisManager::updateBBinst(Addr BBstart, int inst)
{
    if (BBinst.find(BBstart)==BBinst.end()) {
        BBinst.insert(std::make_pair(BBstart,inst));
    }
}

}
