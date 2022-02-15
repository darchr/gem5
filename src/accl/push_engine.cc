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

#include "accl/util.hh"
#include "accl/push_engine.hh"

namespace gem5
{

PushEngine::PushEngine(const PushEngineParams &params) : ClockedObject(params),
    system(params.system),
    requestorId(system->getRequestorId(this)),
    reqPort(name() + ".reqPort", this),
    respPort(name() + ".respPort", this),
    memPort(name() + ".memPort", this),
    // vertexQueueSize(params.vertex_queue_size),
    // vertexQueueLen(0),
    // updateQueue(params.update_queue_size),
    // updateQueueLen(0),
    nextReceiveEvent([this] { processNextReceiveEvent(); }, name()),
    nextReadEvent([this] { processNextReadEvent(); }, name()),
    nextSendEvent([this] { processNextSendEvent(); }, name())
{
}

Port &
PushEngine::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "reqPort") {
        return reqPort;
    } else if (if_name == "respPort") {
        return respPort;
    } else if (if_name == "memPort") {
        return memPort;
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

void
PushEngine::startup()
{
    //FIXME: This is the current version of our initializer.
    // This should be updated in the future.
    WorkListItem vertices [5] = {
                                {0, 0, 3, 0}, // Addr: 0
                                {0, 0, 1, 3}, // Addr: 16
                                {0, 0, 1, 4}, // Addr: 32
                                {0, 0, 0, 5}, // Addr: 48
                                {0, 0, 0, 5}  // Addr: 64
                                };
    Edge edges [6] = {
                    {0, 16}, // Addr: 1048576
                    {0, 32}, // Addr: 1048592
                    {0, 48}, // Addr: 1048608
                    {0, 32}, // Addr: 1048624
                    {0, 64}  // Addr: 1048640
                    };

    for (int i = 0; i < 5; i++) {
        uint8_t* data = workListToMemory(vertices[i]);
        PacketPtr pkt = getWritePacket(0 + i * sizeof(WorkListItem),
                                        16, data, requestorId);
        memPort.sendFunctional(pkt);
    }

    for (int i = 0; i < 6; i++) {
        uint8_t* data = edgeToMemory(edges[i]);
        PacketPtr pkt = getWritePacket(1048576 + i * sizeof(Edge),
                                        16, data, requestorId);
        memPort.sendFunctional(pkt);
    }

}

AddrRangeList
PushEngine::PushRespPort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

bool
PushEngine::PushRespPort::recvTimingReq(PacketPtr pkt)
{
    return owner->handleUpdate(pkt);
}

Tick
PushEngine::PushRespPort::recvAtomic(PacketPtr pkt)
{
    panic("recvAtomic unimpl.");
}

void
PushEngine::PushRespPort::recvFunctional(PacketPtr pkt)
{
    owner->recvFunctional(pkt);
}

void
PushEngine::PushRespPort::recvRespRetry()
{
    panic("recvRespRetry from response port is called.");
}

AddrRangeList
PushEngine::getAddrRanges()
{
    return memPort.getAddrRanges();
}

void
PushEngine::recvFunctional(PacketPtr pkt)
{
    memPort.sendFunctional(pkt);
}

bool
PushEngine::handleUpdate(PacketPtr pkt)
{
    //FIXME: There should be a check if the queues are full.
    // if (vertexQueueLen < vertexQueueSize) {
    //     vertexQueue.push(pkt)
    //         vertexQueueLen++;
    //     if (!nextReceiveEvent.scheduled()) {
    //         schedule(nextReceiveEvent, nextCycle());
    //     }
    //     return true;
    // }
    // return false;
    vertexQueue.push(pkt);
    if (!nextReceiveEvent.scheduled()) {
        schedule(nextReceiveEvent, nextCycle());
    }
    return true;
}

void PushEngine::processNextReceiveEvent()
{
    PacketPtr updatePkt = vertexQueue.front();
    uint8_t* data = updatePkt->getPtr<uint8_t>();

    // data: (edge_index: 32 bits, degree: 32 bits, value: 32 bits)
    uint32_t edge_index = *((uint32_t *)data);
    uint32_t degree = *((uint32_t *)(data + 4));
    uint32_t value = *((uint32_t *)(data + 8));

    std::vector<Addr> addr_queue;
    std::vector<Addr> offset_queue;
    std::vector<int> num_edge_queue;

    for (uint32_t index = 0; index < degree; index++) {
        // FIXME: For now the base edge address is 1048576
        Addr edge_addr = 1048576 + (edge_index + index) * sizeof(Edge);
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
    }

    for (int index = 0; index < addr_queue.size(); index++) {
        PacketPtr pkt = getReadPacket(addr_queue[index], 64, requestorId);
        memReqQueue.push(pkt);
        reqOffsetMap[pkt->req] = offset_queue[index];
        reqNumEdgeMap[pkt->req] = num_edge_queue[index];
        reqValueMap[pkt->req] = value;
    }

    vertexQueue.pop();

    if (!nextReadEvent.scheduled() && !memReqQueue.empty()) {
        schedule(nextReadEvent, nextCycle());
    }
}

void PushEngine::processNextReadEvent()
{
    PacketPtr pkt = memReqQueue.front();
    if (!memPort.blocked()) {
        memPort.sendPacket(pkt);
        memReqQueue.pop();
    }

    if (!nextReadEvent.scheduled() && !memReqQueue.empty()) {
        schedule(nextReadEvent, nextCycle());
    }
}

bool PushEngine::PushMemPort::recvTimingResp(PacketPtr pkt)
{
    return owner->handleMemResp(pkt);
}

void
PushEngine::PushMemPort::sendPacket(PacketPtr pkt)
{
    panic_if(_blocked, "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt))
    {
        blockedPacket = pkt;
        _blocked = true;
    }
}

bool PushEngine::handleMemResp(PacketPtr pkt)
{
    RequestPtr req = pkt->req;
    uint8_t *data = pkt->getPtr<uint8_t>();

    Addr offset = reqOffsetMap[req];
    int num_edges = reqNumEdgeMap[req];
    uint32_t value = reqValueMap[req];

    int edge_in_bytes = sizeof(Edge) / sizeof(uint8_t);
    for (int i = 0; i < num_edges; i++) {
        uint8_t *curr_edge_data = data + offset + i * edge_in_bytes;
        Edge e = memoryToEdge(curr_edge_data);
        uint32_t *update_data = new uint32_t;

        // TODO: Implement propagate function here
        *update_data = value + 1;
        PacketPtr update = getUpdatePacket(e.neighbor,
            sizeof(uint32_t) / sizeof(uint8_t), (uint8_t*) update_data);
        updateQueue.push(update);
    }

    if (!nextSendEvent.scheduled() && !updateQueue.empty()) {
        schedule(nextSendEvent, nextCycle());
    }

    //TODO: Should we always return true? It's the response from the memory
    // so maybe yes. We assume the receiving bandwidth of the PushEngine is
    // higher than its demand bandwidth
    return true;
}

void PushEngine::processNextSendEvent()
{
    PacketPtr pkt = updateQueue.front();
    if (!reqPort.blocked()) {
        reqPort.sendPacket(pkt);
        updateQueue.pop();
    }

    if (!nextSendEvent.scheduled() && !updateQueue.empty()) {
        schedule(nextSendEvent, nextCycle());
    }
}

void
PushEngine::PushReqPort::sendPacket(PacketPtr pkt)
{
    panic_if(_blocked, "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt))
    {
        blockedPacket = pkt;
        _blocked = true;
    }
}

void
PushEngine::PushReqPort::recvReqRetry()
{
    panic_if(!(_blocked && blockedPacket), "Received retry without a blockedPacket");

    _blocked = false;
    sendPacket(blockedPacket);

    if (!blocked()) {
        blockedPacket = nullptr;
    }
}

void
PushEngine::PushMemPort::recvReqRetry()
{
    panic_if(!(_blocked && blockedPacket), "Received retry without a blockedPacket");

    _blocked = false;
    sendPacket(blockedPacket);

    if (!blocked()) {
        blockedPacket = nullptr;
    }
}

}
