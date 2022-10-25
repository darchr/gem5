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

#ifndef __CPU_PROBES_LOOPPOINTMANAGER_HH__
#define __CPU_PROBES_LOOPPOINTMANAGER_HH__

#include <unordered_map>

#include "cpu/base.hh"
#include "params/LoopPointManager.hh"
#include "sim/probe/probe.hh"
#include "base/output.hh"
#include "sim/sim_exit.hh"

namespace gem5
{

class LoopPointManager : public SimObject
{
  public:
    LoopPointManager(const LoopPointManagerParams &params);
    virtual ~LoopPointManager();

    /**
     * @brief This function checks if the current count for the input PC
     * matches the target count of the PC. If they match, then it raises an
     * exit event to exit the Simulation loop.
     *
     * @param pc The target PC of LoopPoint
     */
    void check_count(Addr pc);

  private:
    /** This output a file thats has [currTick : PC : Count] for each exit event
    * raised.
    */
    OutputStream *info;
    /** This stores the target counts for each target PC.
     * ex. PC 0x4069d0 has target count [211076617, 219060252, 407294228]
     */
    std::unordered_map<Addr, std::vector<int>> targetCount;
    /** This stores the current count for each target PC.
     */
    std::unordered_map<Addr, int> counter;

    struct pair_hash {
      std::size_t operator () (const std::pair<Addr,int> &p) const {
        auto h1 = std::hash<Addr>{}(p.first);
        auto h2 = std::hash<int>{}(p.second);
        return h1 ^ h2;
    }
    };

    std::unordered_map<std::pair<Addr,int>, std::pair<std::vector<Addr>,std::vector<int>>, pair_hash> relativePC;
    std::unordered_map<std::pair<Addr,int>, int, pair_hash> regionId;
    bool if_inputRelative;
};

}

#endif // __CPU_PROBES_LOOPPOINTMANAGER_HH__
