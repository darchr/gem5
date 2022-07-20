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

#include "accl/graph/base/base_mem_engine.hh"
#include "accl/graph/base/data_structs.hh"
#include "base/intmath.hh"
#include "params/PushEngine.hh"

namespace gem5
{

class CoalesceEngine;

class PushEngine : public BaseMemEngine
{
  private:
    class PushPacketInfoGen {
      private:
        Addr _start;
        Addr _end;
        size_t _step;
        size_t _atom;
        uint32_t _value;

      public:
        PushPacketInfoGen(Addr start, Addr end, size_t step,
                            size_t atom, uint32_t value):
                        _start(start), _end(end), _step(step),
                        _atom(atom), _value(value)
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

        uint32_t value() { return _value; }
        bool done() { return (_start >= _end); }
    };

    class ReqPort : public RequestPort
    {
      private:
        PushEngine* owner;
        bool _blocked;
        PacketPtr blockedPacket;

      public:
        ReqPort(const std::string& name, PushEngine* owner) :
          RequestPort(name, owner), owner(owner),
          _blocked(false), blockedPacket(nullptr)
        {}
        void sendPacket(PacketPtr pkt);
        bool blocked() { return _blocked; }

      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvReqRetry();
    };

    int numElementsPerLine;
    int retrySpaceAllocated;
    CoalesceEngine* peerCoalesceEngine;

    ReqPort reqPort;

    Addr baseEdgeAddr;

    int numRetries;
    int pushReqQueueSize;
    std::deque<PushPacketInfoGen> pushReqQueue;

    // TODO: Add size one size for all these maps
    std::unordered_map<RequestPtr, Addr> reqOffsetMap;
    std::unordered_map<RequestPtr, int> reqNumEdgeMap;
    std::unordered_map<RequestPtr, uint32_t> reqValueMap;

    // Since the push engine can process incoming packets faster than
    // memory can send those packets, the size of this queue will
    // always be limited by the b/w of the memory.
    std::deque<PacketPtr> memRespQueue;

    template<typename T> PacketPtr createUpdatePacket(Addr addr, T value);

    EventFunctionWrapper nextAddrGenEvent;
    void processNextAddrGenEvent();

    EventFunctionWrapper nextPushEvent;
    void processNextPushEvent();

    struct PushStats : public statistics::Group
    {
      PushStats(PushEngine &push);

      void regStats() override;

      PushEngine &push;

      statistics::Scalar numUpdates;
    };

    PushStats stats;

  protected:
    virtual int respBuffSize() { return memRespQueue.size(); }
    virtual void recvMemRetry();
    virtual bool handleMemResp(PacketPtr pkt);

  public:
    PARAMS(PushEngine);
    PushEngine(const PushEngineParams &params);

    Port& getPort(const std::string &if_name,
                PortID idx=InvalidPortID) override;

    bool allocatePushSpace();

    void deallocatePushSpace(int space);

    void recvWLItem(WorkListItem wl);

    void recvWLItemRetry(WorkListItem wl);

    void registerCoalesceEngine(CoalesceEngine* coalesce_engine,
                                          int elements_per_line);
};

}

#endif // __ACCL_GRAPH_SEGA_PUSH_ENGINE_HH__
