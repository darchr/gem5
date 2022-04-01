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

#include "accl/graph/sega/push_engine.hh"

#include "debug/MPU.hh"

namespace gem5
{

PushEngine::PushEngine(const PushEngineParams &params):
    BaseReadEngine(params),
    reqPort(name() + ".req_port", this),
    baseEdgeAddr(params.base_edge_addr),
    pushReqQueueSize(params.push_req_queue_size),
    memRespQueueSize(params.mem_resp_queue_size),
    onTheFlyReadReqs(0),
    nextAddrGenEvent([this] { processNextAddrGenEvent(); }, name()),
    nextReadEvent([this] { processNextReadEvent(); }, name()),
    nextPushEvent([this] { processNextPushEvent(); }, name())
{}

Port&
PushEngine::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "req_port") {
        return reqPort;
    } else if (if_name == "mem_port") {
        return BaseReadEngine::getPort(if_name, idx);
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

void
PushEngine::startup()
{
    uint8_t* first_update_data = new uint8_t [4];
    uint32_t* tempPtr = (uint32_t*) first_update_data;
    *tempPtr = 0;

    PacketPtr first_update = createUpdatePacket(0, 4, first_update_data);

    sendPushUpdate(first_update);
}


void
PushEngine::ReqPort::sendPacket(PacketPtr pkt)
{
    panic_if(_blocked, "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt))
    {
        blockedPacket = pkt;
        _blocked = true;
    }
}

bool
PushEngine::ReqPort::recvTimingResp(PacketPtr pkt)
{
    panic("recvTimingResp called on the request port.");
}

void
PushEngine::ReqPort::recvReqRetry()
{
    panic_if(!(_blocked && blockedPacket), "Received retry without a blockedPacket");

    _blocked = false;
    sendPacket(blockedPacket);

    if (!blocked()) {
        blockedPacket = nullptr;
    }
}

bool
PushEngine::recvWLItem(WorkListItem wl)
{
    assert(pushReqQueue.size() <= pushReqQueueSize);
    if ((pushReqQueueSize != 0) && (pushReqQueue.size() == pushReqQueueSize)) {
        return false;
    }

    pushReqQueue.push(wl);

    if ((!nextAddrGenEvent.scheduled()) &&
        (!pushReqQueue.empty())) {
        schedule(nextAddrGenEvent, nextCycle());
    }
    return true;
}

void
PushEngine::processNextAddrGenEvent()
{
    WorkListItem wl = pushReqQueue.front();

    std::vector<Addr> addr_queue;
    std::vector<Addr> offset_queue;
    std::vector<int> num_edge_queue;

    for (uint32_t index = 0; index < wl.degree; index++) {
        Addr edge_addr = baseEdgeAddr + (wl.edgeIndex + index) * sizeof(Edge);
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
        PacketPtr pkt = createReadPacket(addr_queue[index], 64);
        reqOffsetMap[pkt->req] = offset_queue[index];
        reqNumEdgeMap[pkt->req] = num_edge_queue[index];
        reqValueMap[pkt->req] = wl.prop;
        pendingReadReqs.push(pkt);
    }

    pushReqQueue.pop();

    if ((!nextAddrGenEvent.scheduled()) && (!pushReqQueue.empty())) {
        schedule(nextAddrGenEvent, nextCycle());
    }

    if ((!nextReadEvent.scheduled()) && (!pendingReadReqs.empty())) {
        schedule(nextReadEvent, nextCycle());
    }
}

void
PushEngine::processNextReadEvent()
{
    if (((memRespQueue.size() + onTheFlyReadReqs) <= memRespQueueSize) &&
        (!memPortBlocked())) {
        PacketPtr pkt = pendingReadReqs.front();
        sendMemReq(pkt);
        onTheFlyReadReqs++;
        pendingReadReqs.pop();
    }

    if ((!nextReadEvent.scheduled()) && (!pendingReadReqs.empty())) {
        schedule(nextReadEvent, nextCycle());
    }
}

bool
PushEngine::handleMemResp(PacketPtr pkt)
{
    onTheFlyReadReqs--;
    memRespQueue.push(pkt);

    if ((!nextPushEvent.scheduled()) && (!memRespQueue.empty())) {
        schedule(nextPushEvent, nextCycle());
    }
    return true;
}

// FIXME: FIX THIS FUNCTION FOR TIMING AND FUNCTIONAL ACCURACY.
void
PushEngine::processNextPushEvent()
{
    PacketPtr pkt = memRespQueue.front();
    RequestPtr req = pkt->req;
    uint8_t *data = pkt->getPtr<uint8_t>();

    DPRINTF(MPU, "%s: Looking at the front of the queue. pkt->Addr: %lu.\n",
            __func__, pkt->getAddr());

    Addr offset = reqOffsetMap[req];
    int num_edges = reqNumEdgeMap[req];
    uint32_t value = reqValueMap[req];

    for (int i = 0; i < num_edges; i++) {
        uint8_t *curr_edge_data = data + offset + (i * sizeof(Edge));
        Edge* e = (Edge*) (curr_edge_data);
        DPRINTF(MPU, "%s: Read %s\n", __func__, e->to_string());
        int data_size = sizeof(uint32_t) / sizeof(uint8_t);
        uint32_t* update_data = (uint32_t*) (new uint8_t [data_size]);
        // TODO: Implement propagate function here
        *update_data = value + 1;
        DPRINTF(MPU, "%s: Sending an update to %lu with value: %d.\n",
                __func__, e->neighbor, *update_data);
        PacketPtr update = createUpdatePacket(e->neighbor,
            sizeof(uint32_t) / sizeof(uint8_t), (uint8_t*) update_data);

        if (sendPushUpdate(update) && (i == num_edges - 1)) {
            memRespQueue.pop();
            // TODO: Erase map entries here.
        }
    }

    if (!nextPushEvent.scheduled() && !memRespQueue.empty()) {
        schedule(nextPushEvent, nextCycle());
    }
}

PacketPtr
PushEngine::createUpdatePacket(Addr addr, unsigned int size, uint8_t *data)
{
    RequestPtr req = std::make_shared<Request>(addr, size, 0, _requestorId);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr) _requestorId) << 2);

    // FIXME: MemCmd::UpdateWL
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);

    pkt->allocate();
    pkt->setData(data);

    return pkt;
}

bool
PushEngine::sendPushUpdate(PacketPtr pkt)
{
    if (!reqPort.blocked()) {
        reqPort.sendPacket(pkt);
        return true;
    }
    return false;
}

}
