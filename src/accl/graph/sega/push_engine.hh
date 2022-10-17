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

#ifndef __ACCL_GRAPH_SEGA_PUSH_ENGINE_HH__
#define __ACCL_GRAPH_SEGA_PUSH_ENGINE_HH__

#include "accl/graph/base/data_structs.hh"
#include "accl/graph/base/graph_workload.hh"
#include "accl/graph/sega/base_memory_engine.hh"
#include "base/intmath.hh"
#include "params/PushEngine.hh"

namespace gem5
{

class MPU;

class PushEngine : public BaseMemoryEngine
{
  private:
    class ReqPort : public RequestPort
    {
      private:
        PushEngine* owner;
        PacketPtr blockedPacket;
        PortID _id;

      public:
        ReqPort(const std::string& name, PushEngine* owner, PortID id) :
          RequestPort(name, owner),
          owner(owner), blockedPacket(nullptr), _id(id)
        {}
        void sendPacket(PacketPtr pkt);
        bool blocked() { return (blockedPacket != nullptr); }
        PortID id() { return _id; }

      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvReqRetry();
    };

    class EdgeReadInfoGen {
      private:
        Addr _src;
        uint32_t _delta;

        Addr _start;
        Addr _end;
        size_t _step;
        size_t _atom;

      public:
        EdgeReadInfoGen(Addr src, uint32_t delta, Addr start,
                        Addr end, size_t step, size_t atom):
                        _src(src), _delta(delta), _start(start),
                        _end(end), _step(step), _atom(atom)
        {}

        Addr src() { return _src; }
        uint32_t delta() { return _delta; }

        std::tuple<Addr, Addr, int> nextReadPacketInfo()
        {
            panic_if(done(), "Should not call nextPacketInfo when done.\n");
            Addr aligned_addr = roundDown<Addr, Addr>(_start, _atom);
            Addr offset = _start - aligned_addr;
            int num_items = 0;

            if (_end > (aligned_addr + _atom)) {
                num_items = (_atom - offset) / _step;
            } else {
                num_items = (_end - _start) / _step;
            }

            return std::make_tuple(aligned_addr, offset, num_items);
        }

        void iterate()
        {
            panic_if(done(), "Should not call iterate when done.\n");
            Addr aligned_addr = roundDown<Addr, Addr>(_start, _atom);
            _start = aligned_addr + _atom;
        }

        bool done() { return (_start >= _end); }
    };
    struct PushInfo {
        Addr src;
        uint32_t value;
        Addr offset;
        int numElements;
    };
    MPU* owner;
    GraphWorkload* graphWorkload;

    bool _running;
    Tick lastIdleEntranceTick;

    AddrRangeList localAddrRange;

    int numPendingPulls;
    int edgePointerQueueSize;
    std::deque<std::tuple<EdgeReadInfoGen, Tick>> edgePointerQueue;
    std::unordered_map<RequestPtr, PushInfo> reqInfoMap;

    int onTheFlyMemReqs;
    int edgeQueueSize;
    int maxPropagatesPerCycle;
    std::deque<std::tuple<MetaEdge, Tick>> metaEdgeQueue;

    int updateQueueSize;
    template<typename T> PacketPtr createUpdatePacket(Addr addr, T value);
    bool enqueueUpdate(Update update);
    std::unordered_map<PortID, AddrRangeList> portAddrMap;
    std::unordered_map<PortID, std::deque<std::tuple<Update, Tick>>> updateQueues;
    std::vector<ReqPort> outPorts;

    bool vertexSpace();
    bool workLeft();

    EventFunctionWrapper nextVertexPullEvent;
    void processNextVertexPullEvent();

    MemoryEvent nextMemoryReadEvent;
    void processNextMemoryReadEvent();

    EventFunctionWrapper nextPropagateEvent;
    void processNextPropagateEvent();

    EventFunctionWrapper nextUpdatePushEvent;
    void processNextUpdatePushEvent();

    struct PushStats : public statistics::Group
    {
      PushStats(PushEngine &push);

      void regStats() override;

      PushEngine &push;

      statistics::Scalar numPropagates;
      statistics::Scalar numNetBlocks;
      statistics::Scalar numIdleCycles;
      statistics::Scalar updateQueueCoalescions;
      statistics::Scalar numUpdates;
      statistics::Scalar numWastefulEdgesRead;

      statistics::Formula TEPS;

      statistics::Histogram edgePointerQueueLatency;
      statistics::Histogram edgeQueueLatency;
      statistics::Histogram updateQueueLength;
      statistics::Histogram numPropagatesHist;
    };

    PushStats stats;

  protected:
    virtual void recvMemRetry();
    virtual bool handleMemResp(PacketPtr pkt);

  public:
    PARAMS(PushEngine);
    PushEngine(const Params& params);
    Port& getPort(const std::string& if_name,
                PortID idx = InvalidPortID) override;
    virtual void init() override;
    void registerMPU(MPU* mpu);

    void recvWorkload(GraphWorkload* workload) { graphWorkload = workload; }
    virtual void recvFunctional(PacketPtr pkt) { memPort.sendFunctional(pkt); }

    void start();
    bool running() { return _running; }
    void recvVertexPush(Addr addr, uint32_t delta,
                        uint32_t edge_index, uint32_t degree);

    void recvReqRetry();

    bool done();
};

}

#endif // __ACCL_GRAPH_SEGA_PUSH_ENGINE_HH__
