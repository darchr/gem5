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

#ifndef __ACCL_GRAPH_BASE_BASE_WL_ENGINE_HH__
#define __ACCL_GRAPH_BASE_BASE_WL_ENGINE_HH__

#include <queue>
#include <unordered_map>

#include "accl/graph/base/util.hh"
#include "base/addr_range.hh"
#include "mem/port.hh"
#include "mem/packet.hh"
#include "params/BaseWLEngine.hh"
#include "sim/clocked_object.hh"
#include "sim/port.hh"
#include "sim/system.hh"

namespace gem5
{

class BaseWLEngine : public ClockedObject
{
  private:
    //FIXME: Change this
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

    RequestorID requestorId;
    WLQueue updateQueue;
    WLQueue responseQueue;

    std::unordered_map<RequestPtr, int> requestOffset;

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
  protected:
    virtual bool sendMemReq(PacketPtr pkt) = 0;
    virtual bool sendWLNotif(Addr addr) = 0;

  public:
    BaseWLEngine(const BaseWLEngineParams &params);

    Port& getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

    RequestorID getRequestorId();
    void setRequestorId(RequestorID requestorId);

    bool handleWLUpdate(PacketPtr pkt);
    bool handleMemResp(PacketPtr resp);
};

}

#endif // __ACCL_GRAPH_BASE_BASE_WL_ENGINE_HH__
