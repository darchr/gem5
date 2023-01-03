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
#include "sim/sim_exit.hh"
#include "sim/system.hh"

namespace gem5
{

DRTracePlayer::DRTracePlayer(const Params &params) :
    ClockedObject(params),
    executeNextInstEvent([this]{ executeNextInst(); },
                         name()+".exec_next_event"),
    retryExecuteInstEvent([this]{ retryExecuteInst(); },
                          name()+".retry_exec_event"),
    reader(params.reader),
    playerId(0),
    requestorId(params.system->getRequestorId(this)),
    maxOutstandingMemReqs(params.max_outstanding_reqs),
    maxInstsPerCycle(params.max_ipc),
    compressAddressRange(params.compress_address_range),
    cacheLineSize(params.system->cacheLineSize()),
    port(name() + ".port", *this),
    stats(this)
{

}

void
DRTracePlayer::startup()
{
    // Get the first instruction and schedule it to be run in the first cycle
    schedule(executeNextInstEvent, 0);
}

void
DRTracePlayer::executeNextInst()
{
    assert(!stalled);
    nextRef = reader->getNextTraceReference(playerId);
    tryExecuteInst(nextRef);
}

void
DRTracePlayer::retryExecuteInst()
{
    assert(stalled);
    stalled = false;
    tryExecuteInst(nextRef);
}

void
DRTracePlayer::tryExecuteInst(DRTraceReader::TraceRef &cur_ref)
{
    assert(!stalled);
    if (!cur_ref.isValid) {
        // End of trace for this player exit the simulation
        // TODO: Move this to when the last instruction is completed
        exitSimLoopNow("End of DRTrace");
    }

    DPRINTF(DRTrace, "Exec reference pc: %0#x, addr: %0#x, size: %d, "
                     "%s, type: %d, valid: %d\n", cur_ref.pc, cur_ref.addr,
                      cur_ref.size, cur_ref.isMemRef() ? "memory" : "other",
                      cur_ref.type, cur_ref.isValid);

    stalled = executeGenericInst(cur_ref);
    if (stalled) {
        return; // Note: executeGenericInst scheduled the event
    }

    if (cur_ref.isMemRef()) {
        stalled = executeMemInst(cur_ref);
        if (stalled) {
            return; // Note: recvRetry will schedule the retry event
        }
    }

    // If we got here, then we know there are more instructions to execute
    // this cycle
    assert(!executeNextInstEvent.scheduled());
    assert(!retryExecuteInstEvent.scheduled());
    schedule(executeNextInstEvent, curTick());
}

void
DRTracePlayer::scheduleInstRetry()
{
    assert(!executeNextInstEvent.scheduled());
    if (!retryExecuteInstEvent.scheduled()) {
        schedule(retryExecuteInstEvent, nextCycle());
        numExecutingInsts = 0;
    } else {
        assert(numExecutingInsts == 0);
    }
}

bool
DRTracePlayer::executeGenericInst(DRTraceReader::TraceRef &cur_inst)
{
    if (maxInstsPerCycle && (numExecutingInsts++ > maxInstsPerCycle)) {
        DPRINTF(DRTrace, "Stalling for instruction limit\n");
        scheduleInstRetry();
        return true; // Stall for a cycle
    }

    if (cur_inst.isInstRef()) {
        assert(cur_inst.pc != 0);
        curPC = cur_inst.pc;
        stats.numInsts++;
    }

    return false;
}

bool
DRTracePlayer::executeMemInst(DRTraceReader::TraceRef &mem_ref)
{
    assert(mem_ref.addr != 0);

    if (maxOutstandingMemReqs &&
        (numOutstandingMemReqs + 1 > maxOutstandingMemReqs)) {
        DPRINTF(DRTrace, "Stalling for outstanding memory limit\n");
        stats.memStalls++;
        return true; // Will be unstalled in recvResponse
    }

    if (!trySendMemRef(mem_ref)) {
        numOutstandingMemReqs++;
        stats.outstandingMemReqs.sample(numOutstandingMemReqs);
        stats.numMemInsts++;
        return false;
    } else {
        return true;
    }
}

bool
DRTracePlayer::trySendMemRef(DRTraceReader::TraceRef &mem_ref)
{
    PacketPtr pkt = getPacket(mem_ref);

    DPRINTF(DRTrace, "Trying to send %s\n", pkt->print());

    if (!port.sendTimingReq(pkt)) {
        DPRINTF(DRTrace, "Failed to send pkt\n");
        if (stats.memStallStart == 0) {
            stats.memStallStart = curTick();
        }
        delete pkt;
        return true;
    } else {
        stats.latencyTracker[pkt] = curTick();
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

    Addr addr = mem_ref.addr;
    if (compressAddressRange.size()) {
        addr -= compressAddressRange.start();
        addr %= compressAddressRange.size();
    }

    unsigned size = mem_ref.size;
    Addr split_addr = roundDown(addr + size - 1, cacheLineSize);
    if (split_addr > addr) {
        warn("Ignoring split packet that crosses cache line boundary.");
        size = split_addr - addr;
    }

    // Create new request
    RequestPtr req = std::make_shared<Request>(addr, size, flags,
                                               requestorId);
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

    if (params().send_data) {
        uint8_t* pkt_data = new uint8_t[req->getSize()];
        pkt->dataDynamic(pkt_data);

        if (cmd.isWrite()) {
            std::fill_n(pkt_data, req->getSize(), (uint8_t)requestorId);
        }
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
    DPRINTF(DRTrace, "Received retry request\n");
    assert(stalled);

    stats.memStalledTime.sample(curTick() - stats.memStallStart);
    stats.memStallStart = 0;

    scheduleInstRetry();
}

bool
DRTracePlayer::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(DRTrace, "Received response for %s\n", pkt->print());
    numOutstandingMemReqs--;

    stats.memLatency.sample(curTick() - stats.latencyTracker[pkt]);
    stats.latencyTracker.erase(pkt);

    delete pkt;

    if (stalled) {
        scheduleInstRetry();
    }

    return true;
}

DRTracePlayer::Stats::Stats(statistics::Group *parent) :
    statistics::Group(parent),
    ADD_STAT(numInsts, statistics::units::Count::get(),
             "Number of instructions executed (not counting memory)"),
    ADD_STAT(numMemInsts, statistics::units::Count::get(),
             "Number of memory instructions executed"),
    ADD_STAT(memStalledTime, statistics::units::Tick::get(),
             "Total time stalled for memory each time stalled"),
    ADD_STAT(memLatency, statistics::units::Tick::get(),
             "Latency for each memory access"),
    ADD_STAT(memStalls, statistics::units::Count::get(),
             "Number of times stalled for outstanding memory limit"),
    ADD_STAT(instStalls, statistics::units::Count::get(),
             "Number of times stalled for IPC limit"),
    ADD_STAT(outstandingMemReqs, statistics::units::Count::get(),
             "Number of outstanding requests for each new request")
{
    memStalledTime
        .init(16)
        .flags(statistics::pdf | statistics::dist);
    memLatency
        .init(16)
        .flags(statistics::pdf | statistics::dist);
    outstandingMemReqs
        .init(16)
        .flags(statistics::pdf | statistics::dist);
}

} // namespace gem5
