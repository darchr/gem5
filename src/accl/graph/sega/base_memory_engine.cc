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

#include "accl/graph/sega/base_memory_engine.hh"

#include "debug/BaseMemoryEngine.hh"
#include "debug/SEGAStructureSize.hh"

namespace gem5
{

BaseMemoryEngine::BaseMemoryEngine(const BaseMemoryEngineParams &params):
    ClockedObject(params),
    system(params.system),
    _requestorId(system->getRequestorId(this)),
    memPort(name() + ".mem_port", this),
    peerMemoryAtomSize(params.attached_memory_atom_size)
{}

BaseMemoryEngine::~BaseMemoryEngine()
{}

Port&
BaseMemoryEngine::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "mem_port") {
        return memPort;
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

void
BaseMemoryEngine::init()
{
    AddrRangeList memory_ranges = memPort.getAddrRanges();
    // BaseMemoryEngine only supports one memory.
    assert(memory_ranges.size() == 1);

    peerMemoryRange = memory_ranges.front();
    DPRINTF(BaseMemoryEngine, "%s: The range attached to this engine is %s. "
                            "The range is %s interleaved.\n", __func__,
                            peerMemoryRange.to_string(),
                            peerMemoryRange.interleaved() ? "" : "not");
}

void
BaseMemoryEngine::MemPort::sendPacket(PacketPtr pkt)
{
    panic_if(_blocked, "Should never try to send if blocked MemSide!");
    DPRINTF(BaseMemoryEngine, "%s: Sending pakcet: %s to "
                "the memory.\n", __func__, pkt->print());
    if (!sendTimingReq(pkt))
    {
        blockedPacket = pkt;
        _blocked = true;
        DPRINTF(BaseMemoryEngine, "%s: MemPort blocked.\n", __func__);
    } else {
        DPRINTF(BaseMemoryEngine, "%s: Packet sent successfully.\n", __func__);
        owner->recvMemRetry();
    }
}

bool
BaseMemoryEngine::MemPort::recvTimingResp(PacketPtr pkt)
{
    return owner->handleMemResp(pkt);
}

void
BaseMemoryEngine::MemPort::recvReqRetry()
{
    panic_if(!(_blocked && blockedPacket),
            "Received retry without a blockedPacket");

    _blocked = false;
    sendPacket(blockedPacket);

    if (!blocked()) {
        blockedPacket = nullptr;
    }
}

PacketPtr
BaseMemoryEngine::createReadPacket(Addr addr, unsigned int size)
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
BaseMemoryEngine::createWritePacket(Addr addr, unsigned int size, uint8_t* data)
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

}
