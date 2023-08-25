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

#include <queue>

#include "mem/packet.hh"
#include "mem/port.hh"
#include "params/RouterEngine.hh"
#include "sim/clocked_object.hh"
#include "sim/system.hh"
#include "base/types.hh"

namespace gem5
{
class CenteralController;
class RouterEngine : public ClockedObject
{
  private:
    class GPTReqPort : public RequestPort
    {
      private:
        RouterEngine* owner;
        PacketPtr blockedPacket;
        PortID _id;

      public:
        GPTReqPort(const std::string& name, RouterEngine* owner, PortID id) :
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

    class GPNReqPort : public RequestPort
    {
      private:
        RouterEngine* owner;
        PacketPtr blockedPacket;
        PortID _id;

      public:
        GPNReqPort(const std::string& name, RouterEngine* owner, PortID id) :
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

    class GPTRespPort : public ResponsePort
    {
      private:
        RouterEngine* owner;
        bool needSendRetryReq;
        PortID _id;

      public:
        GPTRespPort(const std::string& name, RouterEngine* owner, PortID id):
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

    class GPNRespPort : public ResponsePort
    {
      private:
        RouterEngine* owner;
        bool needSendRetryReq;
        PortID _id;

      public:
        GPNRespPort(const std::string& name, RouterEngine* owner, PortID id):
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

    System* system;
    CenteralController* centeralController;
    bool handleRequest(PortID portId, PacketPtr pkt);
    bool handleRemoteRequest(PortID portId, PacketPtr pkt);
    void wakeUpInternal();
    void wakeUpExternal();
    void checkRetryExternal();
    void checkRetryInternal();
    std::vector<GPTReqPort> gptReqPorts;
    std::vector<GPTRespPort> gptRespPorts;

    std::vector<GPNReqPort> gpnReqPorts;
    std::vector<GPNRespPort> gpnRespPorts;


    std::unordered_map<PortID, AddrRangeList> gptAddrMap;
    std::unordered_map<PortID, AddrRangeList> routerAddrMap;

    std::unordered_map<PortID, std::queue<PacketPtr>> gptReqQueues;
    std::unordered_map<PortID, std::queue<PacketPtr>> gpnRespQueues;

    std::unordered_map<PortID, std::queue<PacketPtr>> gptRespQueues;
    std::unordered_map<PortID, std::queue<PacketPtr>> gpnReqQueues;

    std::unordered_map<PortID, Cycles> externalLatency;
    std::unordered_map<PortID, Cycles> internalLatency;
    std::vector<int> inFlightTraffic;
    std::vector<int> tokenVector;
    std::vector<int> sample;

    const uint32_t gptQSize;
    const uint32_t gpnQSize;
    bool emptyQueues;
    const Cycles routerLatency;
    int start;
    Tick sampleTime;
    int tokens;

    EventFunctionWrapper nextGPTGPNEvent;
    void processNextGPTGPNEvent();

    EventFunctionWrapper nextInternalRequestEvent;
    void processNextInternalRequestEvent();

    EventFunctionWrapper nextGPNGPTEvent;
    void processNextGPNGPTEvent();

    EventFunctionWrapper nextExternalRequestEvent;
    void processNextExternalRequestEvent();

    EventFunctionWrapper nextTrafficTrackEvent;
    void processNextTrafficTrackEvent();

    struct RouterEngineStat : public statistics::Group
    {
      RouterEngineStat(RouterEngine &push);

      void regStats() override;

      RouterEngine &router;

      statistics::Vector internalBlockedTraffic;
      statistics::Vector externalBlockedTraffic;
      statistics::Vector internalAcceptedTraffic;
      statistics::Vector externalAcceptedTraffic;
      statistics::Vector bandwidthBlocked;
      statistics::Vector totalInternalTraffic;
    //   statistics::Vector2d internalTrafficVector;
      std::vector<statistics::Histogram *> internalTrafficHist;
      std::vector<statistics::Formula *> internalPortBW;
    };
    RouterEngineStat stats;
  public:
    PARAMS(RouterEngine);
    RouterEngine(const Params &params);
<<<<<<< HEAD
=======

>>>>>>> accl: Improving the router latency
    void registerCenteralController(CenteralController* centeral_controller);
    virtual void init() override;
    virtual void startup() override;
    Port& getPort(const std::string& if_name,
              PortID idx = InvalidPortID) override;

    AddrRangeList getGPNRanges();
    AddrRangeList getGPTRanges();
    void recvReqRetry();

    void checkGPTRetryReq();
    void checkGPNRetryReq();
    bool done();
<<<<<<< HEAD
=======

>>>>>>> accl: Improving the router latency
};

}

#endif // __ACCL_GRAPH_SEGA_ROUTER_ENGINE_HH__
