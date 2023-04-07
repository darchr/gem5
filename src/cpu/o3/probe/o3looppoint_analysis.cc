/*
 * Copyright (c) 2023 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cpu/o3/probe/o3looppoint_analysis.hh"
#include "cpu/o3/dyn_inst.hh"


namespace gem5
{

namespace o3
{

O3LooppointAnalysis::O3LooppointAnalysis(const O3LooppointAnalysisParams &p)
    : ProbeListenerObject(p),
      manager(p.ptmanager),
      validAddrLowerBound(p.validAddrRangeStart),
      validAddrUpperBound(p.validAddrRangeStart+p.validAddrRangeSize)
{
    DPRINTF(O3LooppointAnalysis, "the valid address range start from %i to "
    " %i \n", validAddrLowerBound, validAddrUpperBound);
}

/**
 * ProbeListenerArg generates a listener for the class of Arg and the
 * class type T which is the class containing the function that notify will
 * call.
 *
 * Note that the function is passed as a pointer on construction.
 */

void
O3LooppointAnalysis::regProbeListeners()
{
    // connect the probe listener with the probe "RetriedInstsPC" in the
    // corresponding core.
    // when "RetiredInstsPC" notifies the probe listener, then the function
    // 'check_pc' is automatically called
    typedef ProbeListenerArg<O3LooppointAnalysis, DynInstConstPtr>
     O3LooppointAnalysisListener;
    listeners.push_back(new O3LooppointAnalysisListener(this, "Commit",
                                             &O3LooppointAnalysis::checkPc));
}

void
O3LooppointAnalysis::checkPc(const DynInstConstPtr& dynInst) {

    auto &pcstate = dynInst->pcState().as<GenericISA::PCStateWithNext>();
    if (dynInst->staticInst->isMicroop() && !dynInst->staticInst->isLastMicroop())
        return;
    if(validAddrUpperBound!=0) {
        if(pcstate.pc() < validAddrLowerBound || pcstate.pc() > validAddrUpperBound)
            return;
    }
    if (dynInst->staticInst->isControl() && dynInst->staticInst->isDirectCtrl() && dynInst->tcBase()->getIsaPtr()->inUserMode()) {
        if(pcstate.npc() < pcstate.pc())
            manager->countPc(pcstate.npc());
    }
} 

O3LooppointAnalysisManager::O3LooppointAnalysisManager(const O3LooppointAnalysisManagerParams &p)
    : SimObject(p),
    currentPc(0)
{
    
}

void
O3LooppointAnalysisManager::countPc(const Addr pc)
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

} // namespace o3
} // namespace gem5