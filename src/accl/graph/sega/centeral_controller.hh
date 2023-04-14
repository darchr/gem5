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

#include <cmath>
#include <vector>

#include "accl/graph/base/data_structs.hh"
#include "accl/graph/base/graph_workload.hh"
#include "accl/graph/sega/base_memory_engine.hh"
#include "accl/graph/sega/enums.hh"
#include "accl/graph/sega/mpu.hh"
#include "accl/graph/sega/router_engine.hh"
#include "base/addr_range.hh"
#include "base/intmath.hh"
#include "mem/simple_mem.hh"
#include "params/CenteralController.hh"

namespace gem5
{

class CenteralController : public BaseMemoryEngine
{
  private:
    class ReqPort : public RequestPort
    {
      private:
        CenteralController* owner;
        PacketPtr blockedPacket;
        PortID _id;

      public:
        ReqPort(const std::string& name, CenteralController* owner, PortID id):
          RequestPort(name, owner),
          owner(owner), blockedPacket(nullptr), _id(id)
        {}
        void sendPacket(PacketPtr pkt);
        bool blocked() { return (blockedPacket != nullptr); }

      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvReqRetry();
    };

    ReqPort mapPort;
    Addr maxVertexAddr;
    ProcessingMode mode;

    memory::SimpleMemory* mirrorsMem;

    std::vector<MPU*> mpuVector;
    AddrRangeMap<MPU*> mpuAddrMap;

    int currentSliceId;
    int numTotalSlices;
    int verticesPerSlice;
    int totalUpdatesLeft;

    bool chooseBest;
    int* numPendingUpdates;
    uint32_t* bestPendingUpdate;
    int chooseNextSlice();

    EventFunctionWrapper nextSliceSwitchEvent;
    void processNextSliceSwitchEvent();

    struct ControllerStats : public statistics::Group
    {
      ControllerStats(CenteralController& ctrl);

      void regStats() override;

      CenteralController& ctrl;

      statistics::Scalar numSwitches;
      statistics::Scalar switchedBytes;
      statistics::Scalar switchTicks;
      statistics::Formula switchSeconds;
    };
    ControllerStats stats;

  protected:
    virtual void recvMemRetry() override;
    virtual bool handleMemResp(PacketPtr pkt) override;

  public:
    GraphWorkload* workload;

    PARAMS(CenteralController);
    CenteralController(const Params& params);
    Port& getPort(const std::string& if_name,
                PortID idx = InvalidPortID) override;

    virtual void startup() override;

    virtual void recvFunctional(PacketPtr pkt) override;

    void setAsyncMode() { mode = ProcessingMode::ASYNCHRONOUS; }
    void setBSPMode() { mode = ProcessingMode::BULK_SYNCHRONOUS; }
    void setPGMode() { mode = ProcessingMode::POLY_GRAPH; }

    void createPopCountDirectory(int atoms_per_block);

    void createBFSWorkload(Addr init_addr, uint32_t init_value);
    void createBFSVisitedWorkload(Addr init_addr, uint32_t init_value);
    void createSSSPWorkload(Addr init_addr, uint32_t init_value);
    void createCCWorkload();
    void createAsyncPRWorkload(float alpha, float threshold);
    void createPRWorkload(int num_nodes, float alpha);
    void createBCWorkload(Addr init_addr, uint32_t init_value);

    void recvDoneSignal();

    int workCount();
    float getPRError();
    void printAnswerToHostSimout();
};

}

#endif // __ACCL_GRAPH_SEGA_CENTERAL_CONTROLLER_HH__
