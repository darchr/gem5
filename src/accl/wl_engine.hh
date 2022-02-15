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

#ifndef __ACCL_WLE_HH__
#define __ACCL_WLE_HH__

#include <queue>
#include <unordered_map>

#include "accl/util.hh"
#include "base/addr_range.hh"
#include "mem/port.hh"
#include "mem/packet.hh"
#include "params/WLEngine.hh"
#include "sim/clocked_object.hh"
#include "sim/port.hh"
#include "sim/system.hh"

namespace gem5
{

class WLEngine : public ClockedObject
{
  private:

    struct WLQueue{
      std::queue<PacketPtr> wlQueue;
      uint32_t queueSize;
      bool sendPktRetry;

      void resize(uint32_t size){
        queueSize = size;
      }

      bool blocked(){
        return (wlQueue.size() == queueSize);
      }
      bool empty(){
        return wlQueue.empty();
      }
      void push(PacketPtr pkt){
        wlQueue.push(pkt);
      }
      void pop(){
        wlQueue.pop();
      }
      PacketPtr front(){
        return wlQueue.front();
      }

      WLQueue(uint32_t qSize):
        queueSize(qSize),
        sendPktRetry(false){}
    };

    class WLRespPort : public ResponsePort //From Push engine
    {
      private:
        WLEngine *owner;

      public:
        WLRespPort(const std::string& name, WLEngine* owner):
          ResponsePort(name, owner), owner(owner)
        {}
        virtual AddrRangeList getAddrRanges();

      protected:
        virtual bool recvTimingReq(PacketPtr pkt);
        virtual Tick recvAtomic(PacketPtr pkt);
        virtual void recvFunctional(PacketPtr pkt);
        virtual void recvRespRetry();
    };

    class WLReqPort : public RequestPort //To Apply Engine
    {
      private:
        WLEngine *owner;
        bool _blocked;
        PacketPtr blockedPacket;

      public:
        WLReqPort(const std::string& name, WLEngine* owner):
          RequestPort(name, owner), owner(owner),
          _blocked(false), blockedPacket(nullptr)
        {}
        void sendPacket(PacketPtr pkt);
        bool blocked() { return _blocked; }

      protected:
        void recvReqRetry() override;
        virtual bool recvTimingResp(PacketPtr pkt);
    };

    class WLMemPort : public RequestPort
    {
      private:
        WLEngine *owner;
        bool _blocked;
        PacketPtr blockedPacket;

      public:
        WLMemPort(const std::string& name, WLEngine* owner):
          RequestPort(name, owner), owner(owner),
          _blocked(false), blockedPacket(nullptr)
        {}
        void sendPacket(PacketPtr pkt);
        bool blocked() { return _blocked; }

      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        void recvReqRetry() override;
    };

    System* const system;
    const uint32_t queueSize;
    const RequestorID requestorId;

    WLReqPort reqPort;
    WLRespPort respPort;
    WLMemPort memPort;

    bool handleWLU(PacketPtr pkt);
    bool sendPacket();
    //one queue for write and one for read a priotizes write over read
    void readWLBuffer();


    //Events
    EventFunctionWrapper nextWLReadEvent;
    void processNextWLReadEvent();
    /* Syncronously checked
       If there are any active vertecies:
       create memory read packets + MPU::MPU::MemPortsendTimingReq
    */
    EventFunctionWrapper nextWLReduceEvent;
    void processNextWLReduceEvent();
    /* Activated by MPU::MPUMemPort::recvTimingResp and handleMemResp
       Perform apply and send the write request and read edgeList
       read + write
       Write edgelist loc in buffer
    */

    std::unordered_map<RequestPtr, int> requestOffset;

    WLQueue updateQueue;
    WLQueue responseQueue;

    AddrRangeList getAddrRanges() const;
    bool handleWLUpdate(PacketPtr pkt);
    bool handleMemResp(PacketPtr resp);
    void recvFunctional(PacketPtr pkt);

   public:
    WLEngine(const WLEngineParams &params);

    Port& getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;
};

}

#endif // __ACCL_WLE_HH__
