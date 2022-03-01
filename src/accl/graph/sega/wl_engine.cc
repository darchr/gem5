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

#include "accl/graph/sega/wl_engine.hh"
#include "debug/MPU.hh"
namespace gem5
{

WLEngine::WLEngine(const WLEngineParams &params):
    BaseWLEngine(params),
    respPort(name() + ".respPort", this),
    applyEngine(params.apply_engine),
    lockDir(params.lock_dir)
{}

Port&
WLEngine::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "resp_port") {
        return respPort;
    } else {
        return BaseWLEngine::getPort(if_name, idx);
    }
}

void
WLEngine::startup()
{
    //FIXME: This is the current version of our initializer.
    // This should be updated in the future.
    WorkListItem vertices [5] = {
                                {10000, 10000, 3, 0}, // Addr: 0
                                {10000, 10000, 1, 3}, // Addr: 16
                                {10000, 10000, 1, 4}, // Addr: 32
                                {10000, 10000, 1, 5}, // Addr: 48
                                {10000, 10000, 0, 6}  // Addr: 64
                                };
    Edge edges [7] = {
                    {0, 16}, // Addr: 1048576
                    {0, 32}, // Addr: 1048592
                    {0, 48}, // Addr: 1048608
                    {0, 32}, // Addr: 1048624
                    {0, 64},  // Addr: 1048640
                    {0, 32}
                    };

    for (int i = 0; i < 5; i++) {
        uint8_t* data = workListToMemory(vertices[i]);
        PacketPtr pkt = getWritePacket(0 + i * sizeof(WorkListItem),
                                        16, data, 0);
        sendMemFunctional(pkt);
    }

    for (int i = 0; i < 7; i++) {
        uint8_t* data = edgeToMemory(edges[i]);
        PacketPtr pkt = getWritePacket(1048576 + i * sizeof(Edge),
                                        16, data, 0);
        sendMemFunctional(pkt);
    }

    uint8_t* first_update_data = new uint8_t [4];
    uint32_t* tempPtr = (uint32_t*) first_update_data;
    *tempPtr = 0;

    PacketPtr first_update = getUpdatePacket(
        0, 4, first_update_data, requestorId);

    handleWLUpdate(first_update);
}

bool
WLEngine::sendWLNotif(Addr addr){
    return applyEngine->recvWLNotif(addr);
}

AddrRangeList
WLEngine::RespPort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

bool
WLEngine::RespPort::recvTimingReq(PacketPtr pkt)
{
    return owner->handleWLUpdate(pkt);
}

Tick
WLEngine::RespPort::recvAtomic(PacketPtr pkt)
{
    panic("recvAtomic unimpl.");
}

void
WLEngine::RespPort::recvFunctional(PacketPtr pkt)
{
    owner->recvFunctional(pkt);
}

void
WLEngine::RespPort::recvRespRetry()
{
    panic("recvRespRetry from response port is called.");
}

void
WLEngine::recvFunctional(PacketPtr pkt)
{
    // FIXME: This needs to be fixed
    // if (pkt->cmd == MemCmd::UpdateWL) {
    //     panic("Functional requests should not be made to WL.");
    //     //TODO: Might be a good idea to implement later.
    //     // wlEngine->recvFunctional(pkt);
    // } else {
        sendMemFunctional(pkt);
    // }
}

bool
WLEngine::acquireAddress(Addr addr)
{
    return lockDir->acquire(addr, requestorId);
}

bool
WLEngine::releaseAddress(Addr addr)
{
    return lockDir->release(addr, requestorId);
}

}
