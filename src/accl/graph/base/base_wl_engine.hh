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

#include "accl/graph/base/base_engine.hh"
#include "accl/graph/base/util.hh"
#include "params/BaseWLEngine.hh"

namespace gem5
{

class BaseWLEngine : public BaseEngine
{
  private:
    std::queue<PacketPtr> updateQueue;
    std::queue<PacketPtr> responseQueue;

    std::unordered_map<RequestPtr, Addr> requestOffsetMap;
    std::unordered_map<RequestPtr, uint32_t> requestValueMap;

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
    virtual bool sendWLNotif(Addr addr) = 0;
    virtual void scheduleMainEvent();

  public:

    PARAMS(BaseWLEngine);

    BaseWLEngine(const BaseWLEngineParams &params);

    bool handleWLUpdate(PacketPtr pkt);
};

}

#endif // __ACCL_GRAPH_BASE_BASE_WL_ENGINE_HH__
