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

#include "accl/graph/base/base_mem_engine.hh"

#include "debug/BaseMemEngine.hh"
#include "debug/SEGAStructureSize.hh"

namespace gem5
{

BaseMemEngine::BaseMemEngine(const BaseMemEngineParams &params):
    ClockedObject(params),
    system(params.system),
    memPort(name() + ".mem_port", this),
    memQueueSize(params.outstanding_mem_req_queue_size),
    onTheFlyReqs(0),
    memRetryRequested(false),
    memSpaceRequested(0),
    nextMemReqEvent([this] { processNextMemReqEvent(); }, name()),
    respQueueSize(params.resp_queue_size),
    _requestorId(system->getRequestorId(this)),
    peerMemoryAtomSize(params.attached_memory_atom_size)
{}

BaseMemEngine::~BaseMemEngine()
{}

Port&
BaseMemEngine::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "mem_port") {
        return memPort;
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

void
BaseMemEngine::MemPort::sendPacket(PacketPtr pkt)
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
BaseMemEngine::MemPort::recvTimingResp(PacketPtr pkt)
{
    //TODO: Investigate sending true all the time
    return owner->recvTimingResp(pkt);
}

void
BaseMemEngine::MemPort::recvReqRetry()
{
    panic_if(!(_blocked && blockedPacket), "Received retry without a blockedPacket");

    _blocked = false;
    sendPacket(blockedPacket);

    if (!blocked()) {
        blockedPacket = nullptr;
    }

    owner->wakeUp();
}

void
BaseMemEngine::processNextMemReqEvent()
{
    if ((respQueueSize == 0) ||
        ((respBuffSize() + onTheFlyReqs) < respQueueSize)) {
        PacketPtr pkt = memQueue.front();
        memPort.sendPacket(pkt);
        onTheFlyReqs++;
        DPRINTF(BaseMemEngine, "%s: Sent a packet to memory with the following info. "
                    "pkt->addr: %lu, pkt->size: %lu.\n",
                    __func__, pkt->getAddr(), pkt->getSize());
        memQueue.pop_front();
        DPRINTF(SEGAStructureSize, "%s: Popped pkt: %s from "
                "memQueue. memQueue.size = %d, memQueueSize = %d.\n",
                __func__, pkt->print(), memQueue.size(), memQueueSize);
        DPRINTF(BaseMemEngine, "%s: Popped pkt: %s from "
                "memQueue. memQueue.size = %d, memQueueSize = %d.\n",
                __func__, pkt->print(), memQueue.size(), memQueueSize);
        if (memRetryRequested &&
            (memQueue.size() <=
            (memQueueSize - memSpaceRequested))) {
            memRetryRequested = false;
            memSpaceRequested = 0;
            recvMemRetry();
        }
    }

    if ((!memPort.blocked()) &&
        (!memQueue.empty()) && (!nextMemReqEvent.scheduled())) {
        schedule(nextMemReqEvent, nextCycle());
    }
}

PacketPtr
BaseMemEngine::createReadPacket(Addr addr, unsigned int size)
{
    RequestPtr req = std::make_shared<Request>(addr, size, 0, _requestorId);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr) _requestorId) << 2);

    // Embed it in a packet
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    pkt->allocate();

    return pkt;
}

PacketPtr
BaseMemEngine::createWritePacket(Addr addr, unsigned int size, uint8_t* data)
{
    RequestPtr req = std::make_shared<Request>(addr, size, 0, _requestorId);

    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr) _requestorId) << 2);

    PacketPtr pkt = new Packet(req, MemCmd::WriteReq);
    pkt->allocate();
    pkt->setData(data);

    return pkt;
}

bool
BaseMemEngine::allocateMemQueueSpace(int space)
{
    assert((memQueueSize == 0) ||
        (memQueue.size() <= memQueueSize));
    return (
        (memQueueSize == 0) ||
        (memQueue.size() <= (memQueueSize - space))
        );
}

bool
BaseMemEngine::memQueueFull()
{
    assert((memQueueSize == 0) ||
        (memQueue.size() <= memQueueSize));
    return (
        (memQueueSize != 0) &&
        (memQueue.size() == memQueueSize));
}

void
BaseMemEngine::enqueueMemReq(PacketPtr pkt)
{
    panic_if(memQueueFull(), "Should not enqueue if queue full.\n");
    memQueue.push_back(pkt);
    DPRINTF(SEGAStructureSize, "%s: Pushed pkt: %s to memQueue. "
                "memQueue.size = %d, memQueueSize = %d.\n", __func__,
                pkt->print(), memQueue.size(), memQueueSize);
    DPRINTF(BaseMemEngine, "%s: Pushed pkt: %s to memQueue. "
                "memQueue.size = %d, memQueueSize = %d.\n", __func__,
                pkt->print(), memQueue.size(), memQueueSize);
    if ((!nextMemReqEvent.scheduled()) && (!memPort.blocked())) {
        schedule(nextMemReqEvent, nextCycle());
    }
}

void
BaseMemEngine::requestMemRetry(int space) {
    panic_if((memRetryRequested == true) || (memSpaceRequested != 0),
            "You should not request another alarm without the first one being"
            "responded to.\n");
    DPRINTF(BaseMemEngine, "%s: Alarm requested with space = %d.\n", __func__, space);
    memRetryRequested = true;
    memSpaceRequested = space;
}

void
BaseMemEngine::wakeUp()
{
    assert(!nextMemReqEvent.scheduled());
    if (!memQueue.empty()) {
        schedule(nextMemReqEvent, nextCycle());
    }
}

bool
BaseMemEngine::recvTimingResp(PacketPtr pkt)
{
    onTheFlyReqs--;
    return handleMemResp(pkt);
}

}
