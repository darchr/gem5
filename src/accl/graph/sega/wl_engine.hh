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

#ifndef __ACCL_GRAPH_SEGA_WL_ENGINE_HH__
#define __ACCL_GRAPH_SEGA_WL_ENGINE_HH__

#include <queue>
#include <unordered_map>

#include "accl/graph/base/base_reduce_engine.hh"
#include "accl/graph/base/data_structs.hh"
#include "base/statistics.hh"
#include "params/WLEngine.hh"

namespace gem5
{

class MPU;

class WLEngine : public BaseReduceEngine
{
  private:
    MPU* owner;

    int updateQueueSize;
    std::deque<std::tuple<Addr, uint32_t>> updateQueue;

    int registerFileSize;
    std::unordered_map<Addr, uint32_t> registerFile;
    std::unordered_map<Addr, Tick> vertexReadTime;

    std::unordered_map<Addr, WorkListItem> workListFile;

    std::string workload;
    uint32_t reduce(uint32_t update, uint32_t value);

    EventFunctionWrapper nextReadEvent;
    void processNextReadEvent();

    EventFunctionWrapper nextReduceEvent;
    void processNextReduceEvent();

    struct WorkListStats : public statistics::Group
    {
      WorkListStats(WLEngine &worklist);

      void regStats() override;

      WLEngine &wl;

      statistics::Scalar numReduce;
      statistics::Scalar registerFileCoalesce;
      statistics::Scalar registerShortage;

      statistics::Histogram vertexReadLatency;
    };

    WorkListStats stats;

  public:
    PARAMS(WLEngine);
    WLEngine(const Params& params);
    void registerMPU(MPU* mpu);

    bool handleIncomingUpdate(PacketPtr pkt);
    void handleIncomingWL(Addr addr, WorkListItem wl);

    bool done();
};

}
#endif // __ACCL_GRAPH_SEGA_WL_ENGINE_HH__
