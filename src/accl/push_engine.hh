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

#ifndef __ACCL_PUSH_ENGINE_HH__
#define __ACCL_PUSH_ENGINE_HH__

#include <queue>

#include "base/addr_range_map.hh"
#include "base/statistics.hh"
#include "mem/port.hh"
#include "mem/packet.hh"
#include "params/PushEngine.hh"
#include "sim/clocked_object.hh"
#include "sim/system.hh"

namespace gem5
{

class PushEngine : public ClockedObject
{
  private:

    class PushRespPort : public ResponsePort
    {
      private:
        PushEngine* owner;
        bool _blocked;
        PacketPtr blockedPacket;

      public:
        PushRespPort(const std::string& name, PushEngine* owner):
          ResponsePort(name, owner), owner(owner),
          _blocked(false), blockedPacket(nullptr)
        {}
        virtual AddrRangeList getAddrRanges();
        virtual bool recvTimingReq(PacketPtr pkt);
    }

    class PushReqPort : public RequestPort
    {
      private:
        PushEngine* owner;
        bool _blocked;
        PacketPtr blockedPacket;

      public:
        PushReqPort(const std::string& name, PushEngine* owner):
          RequestPort(name, owner), owner(owner),
          _blocked(false), blockedPacket(nullptr)
        {}
        void sendPacket(PacketPtr pkt);
        bool blocked() { return _blocked; }
        virtual bool recvTimingResp(PacketPtr pkt);
    }

    class PushMemPort : public RequestPort
    {
      private:
        PushEngine* owner
        bool _blocked;
        PacketPtr blockedPacket;

      public:
        PushMemPort(const std::string& name, PushEngine* owner):
          RequestPort(name, owner), owner(owner),
          _blocked(false), blockedPacket(nullptr)
        {}

        void sendPacket(PacktPtr pkt);
        bool blocked() { return _blocked; }
        virtual bool recvTimingResp(PacketPtr pkt);
    }

    virtual void startup() override;

    System* const system;
    const RequestorID requestorId;

    PushReqPort reqPort;
    PushRespPort respPort;

    PushMemPort memPort;

    std::queue<PacketPtr> vertexQueue;
    // int vertexQueueSize;
    // int vertexQueueLen;

    std::unordered_map<RequestPtr, Addr> reqOffsetMap;
    std::unordered_map<RequestPtr, int> reqNumEdgeMap;
    std::unordered_map<RequestPtr, uint32_t> reqValueMap;

    std::queue<PacketPtr> memReqQueue; // Infinite queueing?

    std::queue<PacketPtr> updateQueue;
    // int updateQueueSize;
    // int updateQueueLen;

    EventFunctionWrapper nextReceiveEvent;
    void processNextReceiveEvent();

    EventFunctionWrapper nextReadEvent;
    void processNextReadEvent();

    EventFunctionWrapper nextSendEvent;
    void processNextSendEvent();

    bool handleUpdate(PacketPtr pkt);

    bool handleMemResp(PacketPtr pkt);

  public:

    PushEngine(const PushEngineParams &params);

    Port &getPort(const std::string &if_name,
                PortID idx=InvalidPortID) override;

};

}
#endif // __ACCL_PUSH_ENGINE_HH__
