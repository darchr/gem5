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

#ifndef __CPU_SIMPLE_PROBES_O3LOOPPOINT_ANALYSIS_HH__
#define __CPU_SIMPLE_PROBES_O3LOOPPOINT_ANALYSIS_HH__

#include <list>
#include <map>

#include "params/O3LooppointAnalysis.hh"
#include "params/O3LooppointAnalysisManager.hh"
#include "sim/probe/probe.hh"
#include "cpu/simple_thread.hh"
#include "arch/generic/pcstate.hh"
#include "cpu/probes/pc_count_pair.hh"
#include "debug/O3LooppointAnalysis.hh"
#include "cpu/o3/dyn_inst_ptr.hh"

namespace gem5
{

namespace o3
{
class O3LooppointAnalysis : public ProbeListenerObject
{
  public:
    O3LooppointAnalysis(const O3LooppointAnalysisParams &params);

    virtual void regProbeListeners();

    void checkPc(const DynInstConstPtr& dynInst);

  private:

    O3LooppointAnalysisManager *manager;
    Addr validAddrLowerBound;
    Addr validAddrUpperBound;
};

class O3LooppointAnalysisManager : public SimObject
{
  public:
    O3LooppointAnalysisManager(const O3LooppointAnalysisManagerParams &params);
    void countPc(Addr pc);

  private:
    /**
     * a set of Program Counter addresses that should notify the
     * PcCounterTrackerManager for
     * counter maps addresses to a pair of
     * counter and the last tick the address was accessed
     */
    std::map<Addr, std::pair<uint64_t,Tick>> counter;
    std::list<std::pair<Addr,Tick>> mostRecentPc;
    Addr currentPc;


  public:
    std::map<Addr, std::pair<uint64_t,Tick>>
    getCounter() const
    {
        return counter;
    }

    // returns a pair of the count and last tick
    // the count was incremented
    std::pair<uint64_t,Tick>
    getPcCount(Addr pc) const
    {
        if(counter.find(pc) != counter.end()) {
            return counter.find(pc)->second;
        }
        return std::make_pair(-1, -1);
    }

    // returns a vector of the most recently
    // accessed PCs
    std::vector<std::pair<Addr,Tick>>
    getMostRecentPc() const
    {
        std::vector<std::pair<Addr,Tick>> recent_pcs;
        for (auto it = mostRecentPc.begin(); it != mostRecentPc.end(); it++) {
            recent_pcs.push_back(*it);
        }
        return recent_pcs;
    }

    Addr
    getCurrentPc() const
    {
      return currentPc;
    }

};



} // namespace o3
} // namespace gem5

#endif // __CPU_SIMPLE_PROBES_O3LOOPPOINT_ANALYSIS_HH__
