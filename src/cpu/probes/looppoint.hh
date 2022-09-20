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

#ifndef __CPU_PROBES_LOOPPOINT_HH__
#define __CPU_PROBES_LOOPPOINT_HH__

#include <unordered_set>

#include "cpu/base.hh"
#include "cpu/probes/looppointmanager.hh"
#include "params/LoopPoint.hh"
#include "sim/probe/probe.hh"

namespace gem5
{

class LoopPoint : public ProbeListenerObject
{
  public:
    LoopPoint(const LoopPointParams &params);
    virtual ~LoopPoint();
    virtual void init();
    /**
     * @brief This connects a ProbeListener to the "RetiredInstsPC" ProbePoint 
     * from the corresponding core. 
     */
    virtual void regProbeListeners();
    /**
     * @brief When the "RetiredInstsPC" ProbePoint notifys the ProbeListener,
     * this function is called. This function will check if the committed PC
     * matches the target PC. If it does, then it will call the check_count()
     * in its LoopPointManager.
     */
    void check_pc(const Addr&);

  private:
    /** This stores the targetPC.
     */
    std::unordered_set<Addr> targetPC;
    /** The pointer to the connected core.
     */
    BaseCPU *cpuptr;
    /** The pointer to its LoopPointManager.
     */
    LoopPointManager *manager;

};

}

#endif // __CPU_PROBES_LOOPPOINT_HH__
