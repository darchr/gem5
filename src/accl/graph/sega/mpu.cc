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

void
MPU::startup()
{
    if (((int16_t) applyEngine->getRequestorId) == -1) {
        applyEngine->setRequestorId(nextRequestorId++);
    }
    if (((int16_t) pushEngine->getRequestorId) == -1) {
        pushEngine->setRequestorId(nextRequestorId++);
    }
    if (((int16_t) wlEngine->getRequestorId) == -1) {
        wlEngine->setRequestorId(nextRequestorId++);
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
    return wlEngine->handleWLUpdate(pkt);
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

bool
MPU::MPUMemPort::recvTimingResp(PacketPtr pkt)
{
    return owner->handleMemResp(pkt);
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
    if (pkt->isUpdateWL()) {
        panic("Functional requests should not be made to WL.")
        //TODO: Might be a good idea to implement later.
        // wlEngine->recvFunctional(pkt);
    } else {
        memPort.recvFuctional(pkt);
    }
}

bool
MPU::handleMemReq(PacketPtr pkt)
{
    return memPort.recvTimingReq(pkt);
}

void
MPU::handleMemResp(PacketPtr pkt)
{
    //TODO: Implement this;
}

bool
MPU::recvWLNotif(WorkListItem wl)
{
    return applyEngine->recvWLUpdate(wl);
}

bool
MPU::recvApplyNotif(uint32_t prop, uint32_t degree, uint32_t edgeIndex)
{
    return pushEngine->recvApplyUpdate(prop, degree, edgeIndex);
}

bool
MPU::recvPushUpdate(PacketPtr pkt)
{
    // TODO: Implement this Mahyar
}
