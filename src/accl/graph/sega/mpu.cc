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

#include "accl/graph/sega/mpu.hh"

namespace gem5
{

MPU::MPU(const MPUParams &params):
    ClockedObject(params),
    nextRequestorId(0),
    respPort(name() + ".respPort", this),
    reqPort(name() + ".reqPort", this),
    memPort(name() + ".memPort", this),
    applyEngine(params.apply_engine),
    pushEngine(params.push_engine),
    wlEngine(params.work_list_engine)
{}

Port&
MPU::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "respPort") {
        return respPort;
    } else if (if_name == "reqPort") {
        return reqPort;
    } else if (if_name == "memPort") {
        return memPort;
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

void
MPU::startup()
{
    if (((int16_t) applyEngine->getRequestorId()) == -1) {
        applyEngine->setRequestorId(nextRequestorId++);
    }
    if (((int16_t) pushEngine->getRequestorId()) == -1) {
        pushEngine->setRequestorId(nextRequestorId++);
    }
    if (((int16_t) wlEngine->getRequestorId()) == -1) {
        wlEngine->setRequestorId(nextRequestorId++);
    }

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
                                        16, data, 0);
        memPort.sendFunctional(pkt);
    }

    for (int i = 0; i < 6; i++) {
        uint8_t* data = edgeToMemory(edges[i]);
        PacketPtr pkt = getWritePacket(1048576 + i * sizeof(Edge),
                                        16, data, 0);
        memPort.sendFunctional(pkt);
    }
}

AddrRangeList
MPU::MPURespPort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

bool
MPU::MPURespPort::recvTimingReq(PacketPtr pkt)
{
    return owner->handleWLUpdate(pkt);
}

Tick
MPU::MPURespPort::recvAtomic(PacketPtr pkt)
{
    panic("recvAtomic unimpl.");
}

void
MPU::MPURespPort::recvFunctional(PacketPtr pkt)
{
    owner->recvFunctional(pkt);
}

void
MPU::MPURespPort::recvRespRetry()
{
    panic("recvRespRetry from response port is called.");
}

void
MPU::MPUReqPort::sendPacket(PacketPtr pkt)
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
MPU::MPUReqPort::recvTimingResp(PacketPtr pkt)
{
    panic("recvTimingResp called on the request port.");
}

void
MPU::MPUReqPort::recvReqRetry()
{
    panic_if(!(_blocked && blockedPacket), "Received retry without a blockedPacket");

    _blocked = false;
    sendPacket(blockedPacket);

    if (!blocked()) {
        blockedPacket = nullptr;
    }
}

void
MPU::MPUMemPort::sendPacket(PacketPtr pkt)
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
MPU::MPUMemPort::recvTimingResp(PacketPtr pkt)
{
    //TODO: Investigate sending true all the time
    owner->handleMemResp(pkt);
    return true;
}

void
MPU::MPUMemPort::recvReqRetry()
{
    panic_if(!(_blocked && blockedPacket), "Received retry without a blockedPacket");

    _blocked = false;
    sendPacket(blockedPacket);

    if (!blocked()) {
        blockedPacket = nullptr;
    }
}

AddrRangeList
MPU::getAddrRanges()
{
    return memPort.getAddrRanges();
}

void
MPU::recvFunctional(PacketPtr pkt)
{
    if (pkt->cmd == MemCmd::UpdateWL) {
        panic("Functional requests should not be made to WL.");
        //TODO: Might be a good idea to implement later.
        // wlEngine->recvFunctional(pkt);
    } else {
        memPort.sendFunctional(pkt);
    }
}

bool
MPU::handleMemReq(PacketPtr pkt)
{
    //TODO: Investigate sending true all the time
    memPort.sendPacket(pkt);
    return true;
}

void
MPU::handleMemResp(PacketPtr pkt)
{
    RequestorID requestorId = pkt->requestorId();
    if (applyEngine->getRequestorId() == requestorId) {
        applyEngine->handleMemResp(pkt);
    } else if (pushEngine->getRequestorId() == requestorId) {
        pushEngine->handleMemResp(pkt);
    } else if (wlEngine->getRequestorId() == requestorId) {
        wlEngine->handleMemResp(pkt);
    } else {
        panic("Received a response with an unknown requestorId.");
    }
}

bool
MPU::handleWLUpdate(PacketPtr pkt)
{
    return wlEngine->handleWLUpdate(pkt);
}

bool
MPU::recvWLNotif(Addr addr)
{
    return applyEngine->recvWLNotif(addr);
}

bool
MPU::recvApplyNotif(uint32_t prop, uint32_t degree, uint32_t edge_index)
{
    return pushEngine->recvApplyNotif(prop, degree, edge_index);
}

bool
MPU::recvPushUpdate(PacketPtr pkt)
{
    Addr addr = pkt->getAddr();
    for (auto addr_range: memPort.getAddrRanges()) {
        if (addr_range.contains(addr)) {
            if (memPort.blocked()) {
                return false;
            } else {
                memPort.sendPacket(pkt);
                return true;
            }
        }
    }

    if (reqPort.blocked()) {
        return false;
    }
    reqPort.sendPacket(pkt);
    return true;

}

}
