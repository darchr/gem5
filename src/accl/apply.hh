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

#ifndef __ACCL_APPLY_HH__
#define __ACCL_APPLY_HH__

#include <queue>
#include <unordered_map>

#include "base/addr_range_map.hh"
#include "base/statistics.hh"
#include "mem/port.hh"
#include "mem/packet.hh"
#include "params/MPU.hh"
#include "sim/clocked_object.hh"

class Apply : public ClockedObject
{
  private:

    class ApplyRespPort : public ResponsePort
    {
      private:
        Apply *owner;
        bool _blocked;
        PacketPtr blockedPacket;

      public:
        ApplyRespPort(const std::string& name, SimObject* _owner,
              PortID id=InvalidPortID);

        virtual AddrRangeList getAddrRanges();
        virtual bool recvTimingReq(PacketPtr pkt);
    }

    class ApplyReqPort : public RequestPort
    {
      private:
        Apply *owner;
        bool _blocked;
        PacketPtr blockedPacket;

        struct ApplyQueue{
          std::queue<PacketPtr> applyQueue;
          const uint_32 queueSize;
          bool sendPktRetry;

          bool blocked(){
            return applyQueue.size() == queueSize;
          }
          bool empty(){
            return applyQueue.empty();
          }
          void push(PacketPtr pkt){
            applyQueue.push(pkt);
          }

          ApplyQueue(uint32_t qSize):
            queueSize(qSize){}
        };
      public:
        ApplyReqPort(const std::string& name, SimObject* _owner,
              PortID id=InvalidPortID);

        virtual bool recvTimingResp(PacketPtr pkt);
    }

    class ApplyMemPort : public RequestPort
    {
      private:
        Apply *owner;
        bool _blocked;
        PacketPtr blockedPacket;
      public:
        ApplyReqPort(const std::string& name, SimObject* _owner,
              PortID id=InvalidPortID);
        bool sendPacket(PacktPtr pkt);
        virtual bool recvTimingResp(PacketPtr pkt);

    }
    bool handleWL(PacketPtr pkt);
    bool sendPacket();
    //one queue for write and one for read a priotizes write over read
    void readApplyBuffer();
    bool handleMemResp(PacktPtr resp);
    void writePushBuffer();


    //Events
    void processNextApplyCheckEvent();
    /* Syncronously checked
       If there are any active vertecies:
       create memory read packets + MPU::MPU::MemPortsendTimingReq
    */
    void processNextApplyEvent();
    /* Activated by MPU::MPUMemPort::recvTimingResp and handleMemResp
       Perform apply and send the write request and read edgeList
       read + write
       Write edgelist loc in buffer
    */

    ApplyQueue applyReadQueue;
    ApplyQueue applyWriteQueue;
    ApplyMemPort memPort;
    std::pair<Addr, int>
   public(const ApplyParams &apply);  //fix this
};

#endif // __ACCL_APPLY_HH__