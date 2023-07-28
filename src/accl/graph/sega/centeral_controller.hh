/*
 * Copyright (c) 2021 The Regents of the University of California.
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

#ifndef __ACCL_GRAPH_SEGA_CENTERAL_CONTROLLER_HH__
#define __ACCL_GRAPH_SEGA_CENTERAL_CONTROLLER_HH__

#include <vector>

#include "accl/graph/base/data_structs.hh"
#include "accl/graph/base/graph_workload.hh"
#include "accl/graph/sega/enums.hh"
#include "accl/graph/sega/mpu.hh"
#include "base/addr_range.hh"
#include "params/CenteralController.hh"
#include "sim/clocked_object.hh"
#include "sim/system.hh"

namespace gem5
{

class CenteralController : public ClockedObject
{
  private:
    System* system;
    Addr maxVertexAddr;

    ProcessingMode mode;

    std::vector<MPU*> mpuVector;
    std::unordered_map<MPU*, AddrRangeList> addrRangeListMap;

    PacketPtr createReadPacket(Addr addr, unsigned int size);

  public:

    GraphWorkload* workload;

    PARAMS(CenteralController);
    CenteralController(const CenteralControllerParams &params);
    virtual void startup() override;

    void setAsyncMode() { mode = ProcessingMode::ASYNCHRONOUS; }
    void setBSPMode() { mode = ProcessingMode::BULK_SYNCHRONOUS; }

    void createPopCountDirectory(int atoms_per_block);

    void createBFSWorkload(Addr init_addr, uint32_t init_value);
    void createBFSVisitedWorkload(Addr init_addr, uint32_t init_value);
    void createSSSPWorkload(Addr init_addr, uint32_t init_value);
    void createCCWorkload();
    void createPRWorkload(float alpha);
    void createBCWorkload(Addr init_addr, uint32_t init_value);

    void recvDoneSignal();

    int workCount();
    void printAnswerToHostSimout();
};

}

#endif // __ACCL_GRAPH_SEGA_CENTERAL_CONTROLLER_HH__
