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

#ifndef __ACCL_GRAPH_SEGA_ROUTER_ENGINE_HH__
#define __ACCL_GRAPH_SEGA_ROUTER_ENGINE_HH__

#include "params/RouterEngine.hh"
#include "sim/clocked_object.hh"
#include "mem/packet.hh"
#include "mem/port.hh"

namespace gem5
{

class RouterEngine : public ClockedObject
{
  private:
    class RouterReqPort : public RequestPort
    {
      private:
        RouterEngine* owner;
        PacketPtr blockedPacket;
        PortID _id;

      public:
        RouterReqPort(const std::string& name, RouterEngine* owner, PortID id) :
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

    class InternalReqPort : public RequestPort
    {
      private:
        RouterEngine* owner;
        PacketPtr blockedPacket;
        PortID _id;

      public:
        InternalReqPort(const std::string& name, RouterEngine* owner, PortID id) :
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

    class RouterRespPort : public ResponsePort
    {
      private:
        RouterEngine* owner;
        bool needSendRetryReq;
        PortID _id;

      public:
        RouterRespPort(const std::string& name, RouterEngine* owner, PortID id):
          ResponsePort(name, owner),
          owner(owner), needSendRetryReq(false), _id(id)
        {}
        virtual AddrRangeList getAddrRanges() const;

        PortID id() { return _id; }
        void checkRetryReq();

      protected:
        virtual bool recvTimingReq(PacketPtr pkt);
        virtual Tick recvAtomic(PacketPtr pkt);
        virtual void recvFunctional(PacketPtr pkt);
        virtual void recvRespRetry();
    };

    class InternalRespPort : public RouterRespPort
    {
      private:
        RouterEngine* owner;
        bool needSendRetryReq;
        PortID _id;

      public:
        InternalRespPort(const std::string& name, RouterEngine* owner, PortID id):
          ResponsePort(name, owner),
          owner(owner), needSendRetryReq(false), _id(id)
        {}
        virtual AddrRangeList getAddrRanges() const;

        PortID id() { return _id; }
        void checkRetryReq();

      protected:
        virtual bool recvTimingReq(PacketPtr pkt);
        virtual Tick recvAtomic(PacketPtr pkt);
        virtual void recvFunctional(PacketPtr pkt);
        virtual void recvRespRetry();
    };

//     struct RequestQueue {
//     std::queue<PacketPtr> reqQ;
//     const uint32_t reqQSize;
//     const PortID PortId;

//     bool blocked() {
//         return reqQ.size() == reqQSize;
//     }

//     void push(PacketPtr pkt) {
//         reqQ.push(pkt);
//     }

//     bool emptyQ(){
//         return reqQ.empty();
//       }
      
//     RequestQueue(uint32_t reqQSize, PortID portId):
//     reqQSize(reqQSize),
//     portId(portId) {}
//   };

//   std::vector<RequestQueue> remoteReqQueues;
  
  std::vector<RouterReqPort> gptReqPorts;
  std::vector<RouterRespPort> gptRespPorts;

  std::vector<InternalReqPort> gpnReqPorts;
  std::vector<InternalRespPort> gpnRespPorts;

  std::unordered_map<PortID, AddrRangeList> gptAddrMap;
  std::unordered_map<PortID, AddrRangeList> routerAddrMap;


  public:
    PARAMS(RouterEngine);
    RouterEngine(const Params &params);

    virtual void init() override;
    virtual void startup() override;

    Port& getPort(const std::string& if_name,
                PortID idx = InvalidPortID) override;

    AddrRangeList getGPNRanges();
    AddrRangeList getGPTRanges();

//   std::unordered_map<PortID, std::vector<AddrRangeList>> routerPortAddrMap;

//   AddrRangeMap<PortID, 0> localPortMap;

};

}

#endif // __ACCL_GRAPH_SEGA_ROUTER_ENGINE_HH__
