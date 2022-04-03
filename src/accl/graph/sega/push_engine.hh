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

#include "accl/graph/base/base_read_engine.hh"
#include "accl/graph/base/data_structs.hh"
#include "params/PushEngine.hh"

namespace gem5
{

class PushEngine : public BaseReadEngine
{
  private:
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

    ReqPort reqPort;

    Addr baseEdgeAddr;

    int pushReqQueueSize;
    std::deque<std::pair<std::pair<Addr, Addr>, uint32_t>> pushReqQueue;

    // TODO: Add size one size for all these maps
    std::unordered_map<RequestPtr, Addr> reqOffsetMap;
    std::unordered_map<RequestPtr, int> reqNumEdgeMap;
    std::unordered_map<RequestPtr, uint32_t> reqValueMap;

    // TODO: Possibility of infinite queueing
    std::deque<PacketPtr> pendingReadReqs;

    int memRespQueueSize;
    int onTheFlyReadReqs;
    std::deque<PacketPtr> memRespQueue;

    virtual void startup();

    // PacketPtr createUpdatePacket(Addr addr, unsigned int size, uint8_t* data);
    PacketPtr createUpdatePacket(Addr addr, unsigned int size, uint32_t value);

    bool sendPushUpdate(PacketPtr pkt);

    EventFunctionWrapper nextAddrGenEvent;
    void processNextAddrGenEvent();

    EventFunctionWrapper nextReadEvent;
    void processNextReadEvent();

    EventFunctionWrapper nextPushEvent;
    void processNextPushEvent();

  protected:
    virtual bool handleMemResp(PacketPtr pkt);

  public:
    PARAMS(PushEngine);
    PushEngine(const PushEngineParams &params);

    Port& getPort(const std::string &if_name,
                PortID idx=InvalidPortID) override;

    bool recvWLItem(WorkListItem wl);
};

}

#endif // __ACCL_GRAPH_SEGA_PUSH_ENGINE_HH__
