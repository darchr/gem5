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

#ifndef __CPU_SIMPLE_PROBES_LOOPPOINT_ANALYSIS_HH__
#define __CPU_SIMPLE_PROBES_LOOPPOINT_ANALYSIS_HH__

#include <list>
#include <unordered_map>
#include <unordered_set>

#include "arch/generic/pcstate.hh"
#include "cpu/probes/pc_count_pair.hh"
#include "cpu/simple_thread.hh"
#include "debug/LooppointAnalysis.hh"
#include "params/LooppointAnalysis.hh"
#include "params/LooppointAnalysisManager.hh"
#include "sim/sim_exit.hh"

namespace gem5
{

class LooppointAnalysis : public ProbeListenerObject
{
  public:
    LooppointAnalysis(const LooppointAnalysisParams &params);

    void updateMostRecentPcCount(Addr npc);

    void checkPc(const std::pair<SimpleThread*, StaticInstPtr>&);

    typedef ProbeListenerArg<LooppointAnalysis,
                                    std::pair<SimpleThread*,StaticInstPtr>>
    LooppointAnalysisListener;

    void startListening();
    void stopListening();

  private:

    LooppointAnalysisManager *manager;
    Addr validAddrLowerBound;
    Addr validAddrUpperBound;
    // The upper bound of the valid instruction address range

    std::unordered_set<Addr> encountered_PC;
    // Record the PC that was encountered by this thread before

    std::list<std::pair<PcCountPair, Tick>> localMostRecentPcCount;

    int localInstCounter;

  //=== BBV related start here ===
  private:

    int BBInstCounter;
    Addr BBstart;

    std::unordered_map<Addr, int> BBfreq;

  public:

    std::unordered_map<Addr, int>
    getBBfreq() const
    {
      return BBfreq;
    }

    void
    clearBBfreq()
    {
      BBfreq.clear();
    }

  //=== BBV related end here ===

  public:

    std::vector<std::pair<PcCountPair,Tick>>
    getlocalMostRecentPcCount() const
    {
      std::vector<std::pair<PcCountPair,Tick>> recent_vec;
      for (auto iter = localMostRecentPcCount.begin();
                              iter != localMostRecentPcCount.end(); iter++) {
        recent_vec.push_back(*iter);
      }
      return recent_vec;
    }

};

class LooppointAnalysisManager : public SimObject
{
  public:
    LooppointAnalysisManager(const LooppointAnalysisManagerParams &params);
    void countPc(Addr pc, int instCount);
    void updateBBinst(Addr BBstart, int inst);

  private:

    std::unordered_map<Addr, int> counter;
    // The counter for all recorded PCs

    std::unordered_map<Addr, int> BBinst;

    int regionLength;

    int globalInstCounter;

  public:

    std::unordered_map<Addr, int>
    getCounter() const
    {
        return counter;
    }

    int
    getPcCount(Addr pc) const
    {
        if(counter.find(pc) != counter.end()) {
            return counter.find(pc)->second;
        }
        return -1;
    }

    std::unordered_map<Addr, int>
    getBBinst() const
    {
      return BBinst;
    }

    int
    getGlobalInstCounter() const
    {
      return globalInstCounter;
    }

    void
    clearGlobalInstCounter()
    {
      globalInstCounter = 0;
    }

};

}

#endif // __CPU_SIMPLE_PROBES_LOOPPOINT_ANALYSIS_HH__
