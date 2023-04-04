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

#include <map>
#include <queue>

#include "params/LooppointAnalysis.hh"
#include "params/LooppointAnalysisManager.hh"
#include "sim/probe/probe.hh"
#include "cpu/simple_thread.hh"
#include "arch/generic/pcstate.hh"
#include "cpu/probes/pc_count_pair.hh"
#include "debug/LooppointAnalysis.hh"

namespace gem5
{

class LooppointAnalysis : public ProbeListenerObject
{
  public:
    LooppointAnalysis(const LooppointAnalysisParams &params);

    virtual void regProbeListeners();

    void checkPc(const std::pair<SimpleThread*, StaticInstPtr>&);

  private:

    LooppointAnalysisManager *manager;
    Addr validAddrLowerBound;
    Addr validAddrUpperBound;
};

class LooppointAnalysisManager : public SimObject 
{
  public:
    LooppointAnalysisManager(const LooppointAnalysisManagerParams &params);
    void countPc(Addr pc);

  private:
    /**
     * a set of Program Counter addresses that should notify the
     * PcCounterTrackerManager for
     */
    std::map<Addr, int> counter;
    std::queue<Addr> mostRecentPc;
    Addr currentPc;


  public:
    std::map<Addr, int> 
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

    std::vector<Addr>
    getMostRecentPc() const
    {
      std::vector<Addr> mostRecentPcVector;
      std::queue<Addr> mostRecentPcCopy = mostRecentPc;
      while (!mostRecentPcCopy.empty()) {
        mostRecentPcVector.push_back(mostRecentPcCopy.front());
        mostRecentPcCopy.pop();
      }
      return mostRecentPcVector;
    }

    Addr
    getCurrentPc() const
    {
      return currentPc;
    }

};




}

#endif // __CPU_SIMPLE_PROBES_LOOPPOINT_ANALYSIS_HH__
