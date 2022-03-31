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

#include "accl/graph/base/base_read_engine.hh"

namespace gem5
{

BaseReadEngine::BaseReadEngine(const BaseReadEngineParams &params):
    ClockedObject(params),
    system(params.system),
    memPort(name() + ".mem_port", this),
    _requestorId(system->getRequestorId(this))
{}

BaseReadEngine::~BaseReadEngine()
{}

Port&
BaseReadEngine::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "mem_port") {
        return memPort;
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

void
BaseReadEngine::MemPort::sendPacket(PacketPtr pkt)
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
BaseReadEngine::MemPort::recvTimingResp(PacketPtr pkt)
{
    //TODO: Investigate sending true all the time
    return owner->handleMemResp(pkt);
}

void
BaseReadEngine::MemPort::recvReqRetry()
{
    panic_if(!(_blocked && blockedPacket), "Received retry without a blockedPacket");

    _blocked = false;
    sendPacket(blockedPacket);

    if (!blocked()) {
        blockedPacket = nullptr;
    }
}

PacketPtr
BaseReadEngine::createReadPacket(Addr addr, unsigned int size)
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

}
