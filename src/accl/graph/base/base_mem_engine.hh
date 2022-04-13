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

#ifndef __ACCL_GRAPH_BASE_BASE_MEM_ENGINE_HH__
#define __ACCL_GRAPH_BASE_BASE_MEM_ENGINE_HH__

#include <unordered_map>

#include "base/addr_range.hh"
#include "mem/packet.hh"
#include "mem/port.hh"
#include "params/BaseMemEngine.hh"
#include "sim/clocked_object.hh"
#include "sim/system.hh"

namespace gem5
{

class BaseMemEngine : public ClockedObject
{
  private:
    class MemPort : public RequestPort
    {
      private:
        BaseMemEngine* owner;
        bool _blocked;
        PacketPtr blockedPacket;

        public:
        MemPort(const std::string& name, BaseMemEngine* owner):
            RequestPort(name, owner), owner(owner),
            _blocked(false), blockedPacket(nullptr)
        {}

        void sendPacket(PacketPtr pkt);
        bool blocked() { return _blocked; }

        protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvReqRetry();
    };

    System* system;
    MemPort memPort;

    int outstandingMemReqQueueSize;
    bool alarmRequested;
    int spaceRequested;
    std::deque<PacketPtr> outstandingMemReqQueue;

    EventFunctionWrapper nextMemReqEvent;
    void processNextMemReqEvent();

  protected:
    const RequestorID _requestorId;

    size_t peerMemoryAtomSize;

    void sendMemFunctional(PacketPtr pkt) { memPort.sendFunctional(pkt); }

    bool memReqQueueHasSpace(int space);
    bool memReqQueueFull();
    void enqueueMemReq(PacketPtr pkt);
    bool pendingAlarm() { return alarmRequested; }
    void requestAlarm(int space);

    virtual void respondToAlarm() = 0;
    virtual bool handleMemResp(PacketPtr pkt) = 0;

    PacketPtr createReadPacket(Addr addr, unsigned int size);
    PacketPtr createWritePacket(Addr addr, unsigned int size, uint8_t* data);

  public:
    PARAMS(BaseMemEngine);

    BaseMemEngine(const BaseMemEngineParams &params);
    ~BaseMemEngine();

    Port& getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

    RequestorID requestorId() { return _requestorId; }

    AddrRangeList getAddrRanges() {return memPort.getAddrRanges(); }

    void recvFunctional(PacketPtr pkt);

    void wakeUp();

};

}

#endif // __ACCL_GRAPH_BASE_BASE_MEM_ENGINE_HH__
