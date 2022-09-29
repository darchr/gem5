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

#include "accl/graph/sega/base_memory_engine.hh"
#include "accl/graph/base/data_structs.hh"
#include "base/intmath.hh"
#include "params/PushEngine.hh"

namespace gem5
{

class MPU;

class PushEngine : public BaseMemoryEngine
{
  private:
    class EdgeReadInfoGen {
      private:
        Addr _start;
        Addr _end;
        size_t _step;
        size_t _atom;

        Addr _src;
        uint32_t _value;

        Tick _entrance;
      public:
        EdgeReadInfoGen(Addr start, Addr end, size_t step,
                        size_t atom, Addr src, uint32_t value, Tick entrance):
                        _start(start), _end(end), _step(step), _atom(atom),
                        _src(src), _value(value), _entrance(entrance)
        {}

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
            _start = aligned_addr + _atom;

            return std::make_tuple(aligned_addr, offset, num_items);
        }

        bool done() { return (_start >= _end); }

        Addr src() { return _src; }
        uint32_t value() { return _value; }

        Tick entrance() { return _entrance; }
    };
    struct PushInfo {
        Addr src;
        uint32_t value;
        Addr offset;
        int numElements;
    };
    MPU* owner;

    bool _running;
    Tick lastIdleEntranceTick;

    int numPendingPulls;
    int edgePointerQueueSize;
    std::deque<EdgeReadInfoGen> edgePointerQueue;
    std::unordered_map<RequestPtr, PushInfo> reqInfoMap;

    int onTheFlyMemReqs;
    int edgeQueueSize;
    int maxPropagatesPerCycle;
    std::deque<std::deque<std::tuple<MetaEdge, Tick>>> edgeQueue;

    std::string workload;
    uint32_t propagate(uint32_t value, uint32_t weight);

    bool vertexSpace();
    bool workLeft();

    EventFunctionWrapper nextVertexPullEvent;
    void processNextVertexPullEvent();

    MemoryEvent nextMemoryReadEvent;
    void processNextMemoryReadEvent();

    EventFunctionWrapper nextPropagateEvent;
    void processNextPropagateEvent();

    struct PushStats : public statistics::Group
    {
      PushStats(PushEngine &push);

      void regStats() override;

      PushEngine &push;

      statistics::Scalar numUpdates;
      statistics::Scalar numNetBlocks;
      statistics::Scalar numIdleCycles;

      statistics::Formula TEPS;

      statistics::Histogram edgePointerQueueLatency;
      statistics::Histogram edgeQueueLatency;
    };

    PushStats stats;

  protected:
    virtual void recvMemRetry();
    virtual bool handleMemResp(PacketPtr pkt);

  public:
    PARAMS(PushEngine);
    PushEngine(const Params& params);
    void registerMPU(MPU* mpu);

    virtual void recvFunctional(PacketPtr pkt) { memPort.sendFunctional(pkt); }

    void start();
    bool running() { return _running; }
    void recvVertexPush(Addr addr, WorkListItem wl);

    void recvReqRetry();

    bool done();
};

}

#endif // __ACCL_GRAPH_SEGA_PUSH_ENGINE_HH__
