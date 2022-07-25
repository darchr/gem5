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

#include "debug/SEGAStructureSize.hh"
#include "debug/WLEngine.hh"
#include "mem/packet_access.hh"

namespace gem5
{

WLEngine::WLEngine(const WLEngineParams &params):
    BaseReduceEngine(params),
    respPort(name() + ".resp_port", this),
    coalesceEngine(params.coalesce_engine),
    updateQueueSize(params.update_queue_size),
    registerFileSize(params.on_the_fly_update_map_size),
    nextReadEvent([this]{ processNextReadEvent(); }, name()),
    nextReduceEvent([this]{ processNextReduceEvent(); }, name()),
    stats(*this)
{
    coalesceEngine->registerWLEngine(this);
}

Port&
WLEngine::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "resp_port") {
        return respPort;
    } else {
        return BaseReduceEngine::getPort(if_name, idx);
    }
}

void
WLEngine::init()
{
    respPort.sendRangeChange();
}

AddrRangeList
WLEngine::RespPort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

void
WLEngine::RespPort::checkRetryReq()
{
    if (needSendRetryReq) {
        DPRINTF(WLEngine,  "%s: Sending a RetryReq.\n", __func__);
        sendRetryReq();
        needSendRetryReq = false;
    }
}

bool
WLEngine::RespPort::recvTimingReq(PacketPtr pkt)
{
    if (!owner->handleIncomingUpdate(pkt)) {
        needSendRetryReq = true;
        return false;
    }

    return true;
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
    coalesceEngine->recvFunctional(pkt);
}

AddrRangeList
WLEngine::getAddrRanges() const
{
    return coalesceEngine->getAddrRanges();
}

// TODO: Parameterize the number of pops WLEngine can do at a time.
// TODO: Add a histogram stats of the size of the updateQueue. Sample here.
void
WLEngine::processNextReadEvent()
{
    Addr update_addr;
    uint32_t update_value;
    std::tie(update_addr, update_value) = updateQueue.front();

    DPRINTF(WLEngine,  "%s: Looking at the front of the updateQueue. "
            "(addr: %lu, value: %u).\n", __func__, update_addr, update_value);

    if ((registerFile.find(update_addr) == registerFile.end())) {
        DPRINTF(WLEngine,  "%s: No register already allocated for addr: %lu "
                            "in registerFile.\n", __func__, update_addr);
        if (registerFile.size() < registerFileSize) {
            DPRINTF(WLEngine, "%s: There are free registers available in the "
                                            "registerFile.\n", __func__);
            // TODO: It might be a good idea for WLEngine to act differently
            // on cache rejects. As a first step the cache should not just
            // return a boolean value. It should return an integer/enum
            // to tell WLEngine why it rejected the read request. Their might
            // be things that WLEngine can do to fix head of the line blocking.
            if (coalesceEngine->recvWLRead(update_addr)) {
                DPRINTF(WLEngine, "%s: CoalesceEngine returned true for read "
                            "request to addr: %lu.\n", __func__, update_addr);
                registerFile[update_addr] = update_value;
                DPRINTF(SEGAStructureSize, "%s: Added (addr: %lu, value: %u) "
                        "to registerFile. registerFile.size = %d, "
                        "registerFileSize = %d.\n", __func__, update_addr,
                        update_value, registerFile.size(), registerFileSize);
                DPRINTF(WLEngine, "%s: Added (addr: %lu, value: %u) "
                        "to registerFile. registerFile.size = %d, "
                        "registerFileSize = %d.\n", __func__, update_addr,
                        update_value, registerFile.size(), registerFileSize);
                updateQueue.pop_front();
                DPRINTF(SEGAStructureSize, "%s: Popped (addr: %lu, value: %u) "
                            "from updateQueue. updateQueue.size = %d. "
                            "updateQueueSize = %d.\n", __func__, update_addr,
                            update_value, updateQueue.size(), updateQueueSize);
                DPRINTF(WLEngine, "%s: Popped (addr: %lu, value: %u) "
                            "from updateQueue. updateQueue.size = %d. "
                            "updateQueueSize = %d.\n", __func__, update_addr,
                            update_value, updateQueue.size(), updateQueueSize);
                respPort.checkRetryReq();
            }
        }
    } else {
        // TODO: Generalize this to reduce function rather than just min
        DPRINTF(WLEngine,  "%s: A register has already been allocated for "
                    "addr: %lu in registerFile. registerFile[%lu] = %u.\n",
                __func__, update_addr, update_addr, registerFile[update_addr]);
        registerFile[update_addr] =
                std::min(update_value, registerFile[update_addr]);
        DPRINTF(WLEngine,  "%s: Reduced the update_value: %u with the entry in"
                    " registerFile. registerFile[%lu] = %u.\n", __func__,
                    update_value, update_addr, registerFile[update_addr]);
        stats.registerFileCoalesce++;
        updateQueue.pop_front();
        DPRINTF(SEGAStructureSize, "%s: Popped (addr: %lu, value: %u) "
                            "from updateQueue. updateQueue.size = %d. "
                            "updateQueueSize = %d.\n", __func__, update_addr,
                            update_value, updateQueue.size(), updateQueueSize);
        DPRINTF(WLEngine, "%s: Popped (addr: %lu, value: %u) "
                    "from updateQueue. updateQueue.size = %d. "
                    "updateQueueSize = %d.\n", __func__, update_addr,
                    update_value, updateQueue.size(), updateQueueSize);
        respPort.checkRetryReq();
    }

    if (!updateQueue.empty() && (!nextReadEvent.scheduled())) {
        schedule(nextReadEvent, nextCycle());
    }
}

void
WLEngine::handleIncomingWL(Addr addr, WorkListItem wl)
{
    assert(workListFile.size() <= registerFileSize);

    workListFile[addr] = wl;
    DPRINTF(SEGAStructureSize, "%s: Added (addr: %lu, wl: %s) to "
                "workListFile. workListFile.size = %d.\n", __func__, addr,
                                    wl.to_string(), workListFile.size());
    DPRINTF(WLEngine, "%s: Added (addr: %lu, wl: %s) to "
                "workListFile. workListFile.size = %d.\n", __func__, addr,
                                    wl.to_string(), workListFile.size());
    assert(!workListFile.empty());
    if (!nextReduceEvent.scheduled()) {
        schedule(nextReduceEvent, nextCycle());
    }
}

void
WLEngine::processNextReduceEvent()
{
    for (auto &it : workListFile) {
        Addr addr = it.first;
        assert(registerFile.find(addr) != registerFile.end());
        uint32_t update_value = registerFile[addr];
        DPRINTF(WLEngine,  "%s: Reducing between registerFile and workListFile"
                    ". registerFile[%lu] = %u, workListFile[%lu] = %s.\n",
                                        __func__, addr, registerFile[addr],
                                        addr, workListFile[addr].to_string());
        // TODO: Generalize this to reduce function rather than just min
        workListFile[addr].tempProp =
                    std::min(update_value, workListFile[addr].tempProp);
        DPRINTF(WLEngine,  "%s: Reduction done. workListFile[%lu] = %s.\n",
                            __func__, addr, workListFile[addr].to_string());
        stats.numReduce++;

        coalesceEngine->recvWLWrite(addr, workListFile[addr]);
        registerFile.erase(addr);
        DPRINTF(SEGAStructureSize, "%s: Removed addr: %lu from registerFile. "
                    "registerFile.size = %d, registerFileSize = %d\n",
                    __func__, addr, registerFile.size(), registerFileSize);
        DPRINTF(WLEngine, "%s: Removed addr: %lu from registerFile. "
                    "registerFile.size = %d, registerFileSize = %d\n",
                    __func__, addr, registerFile.size(), registerFileSize);
    }
    workListFile.clear();
}

bool
WLEngine::handleIncomingUpdate(PacketPtr pkt)
{
    assert(updateQueue.size() <= updateQueueSize);
    if ((updateQueueSize != 0) && (updateQueue.size() == updateQueueSize)) {
        return false;
    }

    updateQueue.emplace_back(pkt->getAddr(), pkt->getLE<uint32_t>());
    DPRINTF(SEGAStructureSize, "%s: Emplaced (addr: %lu, value: %u) in the "
                "updateQueue. updateQueue.size = %d, updateQueueSize = %d.\n",
                __func__, pkt->getAddr(), pkt->getLE<uint32_t>(),
                updateQueue.size(), updateQueueSize);
    DPRINTF(WLEngine, "%s: Emplaced (addr: %lu, value: %u) in the "
                "updateQueue. updateQueue.size = %d, updateQueueSize = %d.\n",
                __func__, pkt->getAddr(), pkt->getLE<uint32_t>(),
                updateQueue.size(), updateQueueSize);


    // delete the packet since it's not needed anymore.
    delete pkt;

    if (!nextReadEvent.scheduled()) {
        schedule(nextReadEvent, nextCycle());
    }
    return true;
}

WLEngine::WorkListStats::WorkListStats(WLEngine &_wl)
    : statistics::Group(&_wl),
    wl(_wl),

    ADD_STAT(numReduce, statistics::units::Count::get(),
             "Number of memory blocks read for vertecies"),
    ADD_STAT(registerFileCoalesce, statistics::units::Count::get(),
             "Number of memory blocks read for vertecies")
{
}

void
WLEngine::WorkListStats::regStats()
{
    using namespace statistics;
}

}
