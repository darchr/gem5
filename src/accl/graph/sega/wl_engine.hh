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
#include "accl/graph/sega/coalesce_engine.hh"
#include "base/statistics.hh"
#include "params/WLEngine.hh"

namespace gem5
{

class WLEngine : public BaseReduceEngine
{
  private:
    class RespPort : public ResponsePort
    {
      private:
        WLEngine* owner;
        bool needSendRetryReq;

      public:
        RespPort(const std::string& name, WLEngine* owner):
          ResponsePort(name, owner), owner(owner)
        {}
        virtual AddrRangeList getAddrRanges() const;

        void checkRetryReq();

      protected:
        virtual bool recvTimingReq(PacketPtr pkt);
        virtual Tick recvAtomic(PacketPtr pkt);
        virtual void recvFunctional(PacketPtr pkt);
        virtual void recvRespRetry();
    };

    RespPort respPort;

    bool blockedByCoalescer;
    CoalesceEngine* coalesceEngine;

    int updateQueueSize;
    std::deque<PacketPtr> updateQueue;

    int onTheFlyUpdateMapSize;
    std::unordered_map<Addr, uint32_t> onTheFlyUpdateMap;

    std::unordered_map<Addr, WorkListItem> addrWorkListMap;

    void recvFunctional(PacketPtr pkt);

    AddrRangeList getAddrRanges() const;

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
      statistics::Scalar onTheFlyCoalesce;
    };

    WorkListStats stats;

  public:
    PARAMS(WLEngine);

    WLEngine(const WLEngineParams &params);

    Port& getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

    bool handleIncomingUpdate(PacketPtr pkt);

    void handleIncomingWL(Addr addr, WorkListItem wl);
};

}
#endif // __ACCL_GRAPH_SEGA_WL_ENGINE_HH__
