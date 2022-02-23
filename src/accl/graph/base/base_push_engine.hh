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

#ifndef __ACCL_GRAPH_BASE_BASE_PUSH_ENGINE_HH__
#define __ACCL_GRAPH_BASE_BASE_PUSH_ENGINE_HH__

#include <queue>

#include "accl/graph/base/base_engine.hh"
#include "mem/port.hh"
#include "mem/request.hh"
#include "mem/packet.hh"
#include "params/BasePushEngine.hh"

namespace gem5
{

class BasePushEngine : public BaseEngine
{
  private:

    struct ApplyNotif {
        uint32_t prop;
        uint32_t degree;
        uint32_t edgeIndex;

        ApplyNotif(uint32_t prop, uint32_t degree, uint32_t edge_index):
        prop(prop), degree(degree), edgeIndex(edge_index)
        {}
    };
    std::queue<ApplyNotif> notifQueue;
    // int vertexQueueSize;
    // int vertexQueueLen;

    std::unordered_map<RequestPtr, Addr> reqOffsetMap;
    std::unordered_map<RequestPtr, int> reqNumEdgeMap;
    std::unordered_map<RequestPtr, uint32_t> reqValueMap;

    std::queue<PacketPtr> updateQueue;
    // int updateQueueSize;
    // int updateQueueLen;

    EventFunctionWrapper nextReceiveEvent;
    void processNextReceiveEvent();

    EventFunctionWrapper nextReadEvent;
    void processNextReadEvent();

    EventFunctionWrapper nextSendEvent;
    void processNextSendEvent();

  protected:
    virtual bool sendPushUpdate(PacketPtr pkt) = 0;
    virtual bool handleMemResp(PacketPtr pkt);

  public:

    PARAMS(BasePushEngine);

    BasePushEngine(const BasePushEngineParams &params);

    bool recvApplyNotif(uint32_t prop, uint32_t degree, uint32_t edge_index);

};

}

#endif // __ACCL_GRAPH_BASE_BASE_PUSH_ENGINE_HH__
