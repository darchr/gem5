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

#include "accl/graph/base/base_push_engine.hh"

#include "accl/graph/base/util.hh"

namespace gem5
{

BasePushEngine::BasePushEngine(const BasePushEngineParams &params) :
    BaseEngine(params),
    nextReadEvent([this] { processNextReadEvent(); }, name()),
    nextPushEvent([this] { processNextPushEvent(); }, name())
{}

bool
BasePushEngine::recvApplyNotif(uint32_t prop,
        uint32_t degree, uint32_t edge_index)
{
    notifQueue.emplace(prop, degree, edge_index);
    if (!nextReadEvent.scheduled()) {
        schedule(nextReadEvent, nextCycle());
    }
    return true;
}

void
BasePushEngine::processNextReadEvent()
{
    ApplyNotif notif = notifQueue.front();

    std::vector<Addr> addr_queue;
    std::vector<Addr> offset_queue;
    std::vector<int> num_edge_queue;

    for (uint32_t index = 0; index < notif.degree; index++) {
        // FIXME: For now the base edge address is 1048576
        Addr edge_addr = 1048576 + (notif.edgeIndex + index) * sizeof(Edge);
        Addr req_addr = (edge_addr / 64) * 64;
        Addr req_offset = edge_addr % 64;
        if (addr_queue.size()) {
            if (addr_queue.back() == req_addr) {
                num_edge_queue.back()++;
            }
            else {
                addr_queue.push_back(req_addr);
                offset_queue.push_back(req_offset);
                num_edge_queue.push_back(1);
            }
        }
        else {
            addr_queue.push_back(req_addr);
            offset_queue.push_back(req_offset);
            num_edge_queue.push_back(1);
        }
    };

    for (int index = 0; index < addr_queue.size(); index++) {
        if (!memPortBlocked()) {
            PacketPtr pkt = getReadPacket(addr_queue[index], 64, requestorId);
            reqOffsetMap[pkt->req] = offset_queue[index];
            reqNumEdgeMap[pkt->req] = num_edge_queue[index];
            reqValueMap[pkt->req] = notif.prop;
            sendMemReq(pkt);
            notifQueue.pop();
        }
    }

    if (!nextReadEvent.scheduled() && !notifQueue.empty()) {
        schedule(nextReadEvent, nextCycle());
    }
}

void
BasePushEngine::processNextPushEvent()
{
    PacketPtr pkt = memRespQueue.front();
    RequestPtr req = pkt->req;
    uint8_t *data = pkt->getPtr<uint8_t>();

    Addr offset = reqOffsetMap[req];
    int num_edges = reqNumEdgeMap[req];
    uint32_t value = reqValueMap[req];

    int edge_in_bytes = sizeof(Edge) / sizeof(uint8_t);
    for (int i = 0; i < num_edges; i++) {
        uint8_t *curr_edge_data = data + offset + (i * edge_in_bytes);
        Edge e = memoryToEdge(curr_edge_data);
        int data_size = sizeof(uint32_t) / sizeof(uint8_t);
        uint32_t* update_data = (uint32_t*) (new uint8_t [data_size]);

        // TODO: Implement propagate function here
        *update_data = value + 1;
        PacketPtr update = getUpdatePacket(e.neighbor,
            sizeof(uint32_t) / sizeof(uint8_t), (uint8_t*) update_data,
            requestorId);
        if (sendPushUpdate(update)) {
            memRespQueue.pop();
            // TODO: Erase map entries here.
        }
    }

    if (!nextPushEvent.scheduled() && !memRespQueue.empty()) {
        schedule(nextPushEvent, nextCycle());
    }
}

void
BasePushEngine::scheduleMainEvent()
{
    if (!memRespQueue.empty() && !nextPushEvent.scheduled()) {
        schedule(nextPushEvent, nextCycle());
    }
}

}
