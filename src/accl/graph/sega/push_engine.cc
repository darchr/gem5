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
#include "mem/packet_access.hh"

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

    // PacketPtr first_update = createUpdatePacket(0, 4, first_update_data);
    PacketPtr first_update = createUpdatePacket(0, 4, (uint32_t) 0);

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

    DPRINTF(MPU, "%s: Received a reqRetry.\n", __func__);

    _blocked = false;
    sendPacket(blockedPacket);

    if (!_blocked) {
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

    Addr start_addr = baseEdgeAddr + (wl.edgeIndex * sizeof(Edge));
    Addr end_addr = start_addr + (wl.degree * sizeof(Edge));
    uint32_t update_value = wl.prop;
    pushReqQueue.push_back(
        std::make_pair(std::make_pair(start_addr, end_addr), update_value));

    if ((!nextAddrGenEvent.scheduled()) &&
        (!pushReqQueue.empty())) {
        schedule(nextAddrGenEvent, nextCycle());
    }
    return true;
}

void
PushEngine::processNextAddrGenEvent()
{
    Addr start_addr, end_addr;
    uint32_t update_value;

    std::pair<std::pair<Addr, Addr>, uint32_t> front = pushReqQueue.front();
    std::tie(start_addr, end_addr) = front.first;
    update_value = front.second;

    Addr req_addr = (start_addr / 64) * 64;
    Addr req_offset = start_addr % 64;
    int num_edges = 0;

    if (end_addr > req_addr + 64) {
        num_edges = (req_addr + 64 - start_addr) / sizeof(Edge);
    } else {
        num_edges = (end_addr - start_addr) / sizeof(Edge);
    }
    PacketPtr pkt = createReadPacket(req_addr, 64);
    reqOffsetMap[pkt->req] = req_offset;
    reqNumEdgeMap[pkt->req] = num_edges;
    reqValueMap[pkt->req] = update_value;
    pendingReadReqs.push_back(pkt);

    pushReqQueue.pop_front();

    if (req_addr + 64 < end_addr) {
        pushReqQueue.push_front(
        std::make_pair(std::make_pair(req_addr + 64, end_addr), update_value)
        );
    }

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
        pendingReadReqs.pop_front();
    }

    if ((!nextReadEvent.scheduled()) && (!pendingReadReqs.empty())) {
        schedule(nextReadEvent, nextCycle());
    }
}

bool
PushEngine::handleMemResp(PacketPtr pkt)
{
    onTheFlyReadReqs--;
    memRespQueue.push_back(pkt);

    if ((!nextPushEvent.scheduled()) && (!memRespQueue.empty())) {
        schedule(nextPushEvent, nextCycle());
    }
    return true;
}

// TODO: Add a parameter to allow for doing multiple pushes at the same time.
void
PushEngine::processNextPushEvent()
{
    PacketPtr pkt = memRespQueue.front();
    RequestPtr req = pkt->req;
    uint8_t *data = pkt->getPtr<uint8_t>();

    Addr offset = reqOffsetMap[req];
    uint32_t value = reqValueMap[req];

    DPRINTF(MPU, "%s: Looking at the front of the queue. pkt->Addr: %lu, "
                "offset: %lu\n",
            __func__, pkt->getAddr(), offset);

    Edge* e = (Edge*) (data + offset);
    // DPRINTF(MPU, "%s: Read %s\n", __func__, e->to_string());

    // TODO: Implement propagate function here
    uint32_t update_value = value + 1;
    DPRINTF(MPU, "%s: Sending an update to %lu with value: %d.\n",
            __func__, e->neighbor, update_value);

    PacketPtr update = createUpdatePacket(e->neighbor,
                        sizeof(uint32_t), update_value);

    if (sendPushUpdate(update)) {
        DPRINTF(MPU, "%s: Send a push update to addr: %lu with value: %d.\n",
                    __func__, e->neighbor, update_value);
        reqOffsetMap[req] = reqOffsetMap[req] + sizeof(Edge);
        reqNumEdgeMap[req]--;
    }

    if (reqNumEdgeMap[req] == 0) {
        memRespQueue.pop_front();
        reqOffsetMap.erase(req);
        reqNumEdgeMap.erase(req);
        reqValueMap.erase(req);
    }

    if (!nextPushEvent.scheduled() && !memRespQueue.empty()) {
        schedule(nextPushEvent, nextCycle());
    }
}

PacketPtr
// PushEngine::createUpdatePacket(Addr addr, unsigned int size, uint8_t* data)
PushEngine::createUpdatePacket(Addr addr, unsigned int size, uint32_t value)
{
    RequestPtr req = std::make_shared<Request>(addr, size, 0, _requestorId);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr) _requestorId) << 2);

    // FIXME: MemCmd::UpdateWL
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);

    pkt->allocate();
    // pkt->setData(data);
    pkt->setLE<uint32_t>(value);

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
