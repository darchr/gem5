/*
 * Copyright (c) 2020 The Regents of the University of California.
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

#ifndef __ACCL_GRAPH_SEGA_MPU_HH__
#define __ACCL_GRAPH_SEGA_MPU_HH__

#include <unordered_map>
#include <vector>

#include "accl/graph/base/data_structs.hh"
#include "accl/graph/sega/coalesce_engine.hh"
#include "accl/graph/sega/enums.hh"
#include "accl/graph/sega/push_engine.hh"
#include "accl/graph/sega/wl_engine.hh"
#include "base/addr_range.hh"
#include "mem/packet.hh"
#include "sim/sim_object.hh"
#include "sim/system.hh"
#include "params/MPU.hh"

namespace gem5
{

class CenteralController;

class MPU : public SimObject
{
  private:
    System* system;
    CenteralController* centeralController;

    WLEngine* wlEngine;
    CoalesceEngine* coalesceEngine;
    PushEngine* pushEngine;
    
  public:
    PARAMS(MPU);
    MPU(const Params& params);
    void registerCenteralController(CenteralController* centeral_controller);

    void setProcessingMode(ProcessingMode mode) { coalesceEngine->setProcessingMode(mode); }
    void createAsyncPopCountDirectory(int atoms_per_block) { coalesceEngine->createAsyncPopCountDirectory(atoms_per_block); }
    void createBSPPopCountDirectory(int atoms_per_block) { coalesceEngine->createBSPPopCountDirectory(atoms_per_block); }

    unsigned int vertexAtomSize() { return coalesceEngine->params().attached_memory_atom_size; }
    AddrRangeList getAddrRanges() { return coalesceEngine->getAddrRanges(); }
    uint64_t getCacheSize() { return coalesceEngine->params().cache_size; }
    void recvFunctional(PacketPtr pkt) { coalesceEngine->recvFunctional(pkt); }
    void postMemInitSetup() { coalesceEngine->postMemInitSetup(); }
    void postConsumeProcess() { coalesceEngine->postConsumeProcess(); }
    void swapDirectories() { coalesceEngine->swapDirectories(); }

    bool handleIncomingUpdate(PacketPtr pkt);

    void handleIncomingWL(Addr addr, WorkListItem wl);
    ReadReturnStatus recvWLRead(Addr addr) { return coalesceEngine->recvWLRead(addr); }
    void recvWLWrite(Addr addr, WorkListItem wl);
    void recvWorkload(GraphWorkload* Workload);

    int workCount() { return coalesceEngine->workCount(); }
    void recvVertexPull() { return coalesceEngine->recvVertexPull(); }
    bool running() { return pushEngine->running(); }
    void start() { return pushEngine->start(); }
    void recvVertexPush(Addr addr, uint32_t delta,
                        uint32_t edge_index, uint32_t degree);

    void recvMirrorPush(Addr addr, uint32_t delta,
                        uint32_t edge_index, uint32_t degree);
    void startProcessingMirrors(Tick time_to_wait) { pushEngine->startProcessingMirrors(time_to_wait); }

    void recvDoneSignal();
    bool done();

    uint64_t getBaseAddr() {return pushEngine->params().base_addr;};
};

} // namespace gem5

#endif // __ACCL_GRAPH_SEGA_MPU_HH__
