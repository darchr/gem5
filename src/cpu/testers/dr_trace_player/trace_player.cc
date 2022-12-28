/*
 * Copyright (c) 2022 The Regents of the University of California.
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

#include "cpu/testers/dr_trace_player/trace_player.hh"

#include "base/trace.hh"
#include "debug/DRTrace.hh"
#include "sim/system.hh"

namespace gem5
{

DRTracePlayer::DRTracePlayer(const Params &params) :
    ClockedObject(params),
    onClockEvent([this]{ tryExecuteInsts(); }, name()+"clock_event"),
    reader(params.reader),
    playerId(0),
    requestorId(params.system->getRequestorId(this)),
    port(name() + ".port", *this)
{

}

void
DRTracePlayer::startup()
{
    // Get the first instruction and schedule it to be run in the first cycle
    nextRef = reader->getNextTraceReference(playerId);
    schedule(onClockEvent, 0);
}

void
DRTracePlayer::tryExecuteInsts()
{
    assert(!stalled);
    DRTraceReader::TraceRef &cur_ref = nextRef;

    DPRINTF(DRTrace, "Exec reference pc: %0#x, addr: %0#x, size: %d, "
                     "%s, type: %d, valid: %d\n", cur_ref.pc, cur_ref.addr,
                      cur_ref.size, cur_ref.isInst ? "inst" : "memory",
                      cur_ref.type, cur_ref.isValid);

    if (cur_ref.isInst) {
        assert(cur_ref.pc != 0);
        curPC = cur_ref.pc;
    }

    if (cur_ref.isMemRef()) {
        assert(cur_ref.addr != 0);
        stalled = trySendMemRef(cur_ref);
    }

    if (!stalled) {
        nextRef = reader->getNextTraceReference(playerId);
        schedule(onClockEvent, nextCycle());
    }
}

bool
DRTracePlayer::trySendMemRef(DRTraceReader::TraceRef &mem_ref)
{
    PacketPtr pkt = getPacket(mem_ref);
    if (port.sendTimingReq(pkt)) {
        delete pkt;
        return true;
    } else {
        return false;
    }
}

PacketPtr
DRTracePlayer::getPacket(DRTraceReader::TraceRef &mem_ref)
{
    Request::Flags flags = Request::PHYSICAL;
    if (mem_ref.type == DRTraceReader::TraceRef::PREFETCH) {
        flags = flags | Request::PREFETCH;
    }

    // Create new request
    RequestPtr req = std::make_shared<Request>(mem_ref.addr, mem_ref.size,
                                               flags,
                                               requestorId);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(curPC);

    MemCmd cmd;
    if (mem_ref.type == DRTraceReader::TraceRef::READ ||
        mem_ref.type == DRTraceReader::TraceRef::PREFETCH) {
        cmd = MemCmd::ReadReq;
    } else {
        assert(mem_ref.type == DRTraceReader::TraceRef::WRITE);
        cmd = MemCmd::WriteReq;
    }
    // Embed it in a packet
    PacketPtr pkt = new Packet(req, cmd);

    uint8_t* pkt_data = new uint8_t[req->getSize()];
    pkt->dataDynamic(pkt_data);

    if (cmd.isWrite()) {
        std::fill_n(pkt_data, req->getSize(), (uint8_t)requestorId);
    }

    return pkt;
}


Port &
DRTracePlayer::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "port") {
        return port;
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

void
DRTracePlayer::recvReqRetry()
{
    assert(stalled);
    stalled = false;
    schedule(onClockEvent, nextCycle());
}

bool
DRTracePlayer::recvTimingResp(PacketPtr pkt)
{
    delete pkt;
    return true;
}

} // namespace gem5
