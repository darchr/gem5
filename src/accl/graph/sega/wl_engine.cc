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

#include "debug/WLEngine.hh"
#include "mem/packet_access.hh"

namespace gem5
{

WLEngine::WLEngine(const WLEngineParams &params):
    BaseReduceEngine(params),
    respPort(name() + ".resp_port", this),
    coalesceEngine(params.coalesce_engine),
    updateQueueSize(params.update_queue_size),
    onTheFlyUpdateMapSize(params.on_the_fly_update_map_size),
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

    DPRINTF(WLEngine,  "%s: Looking at the front of the updateQueue. Addr: %lu, "
                "value: %u.\n", __func__, update_addr, update_value);

    if ((onTheFlyUpdateMap.find(update_addr) == onTheFlyUpdateMap.end())) {
        DPRINTF(WLEngine,  "%s: Did not find the addr: %lu in onTheFlyUpdateMap.\n",
                    __func__, update_addr);
        if (onTheFlyUpdateMap.size() < onTheFlyUpdateMapSize) {
            if (coalesceEngine->recvReadAddr(update_addr)) {
                onTheFlyUpdateMap[update_addr] = update_value;
                DPRINTF(WLEngine,  "%s: Added a new item to onTheFlyUpdateMap. "
                            "onTheFlyUpdateMap[%lu] = %u.\n", __func__,
                            update_addr, onTheFlyUpdateMap[update_addr]);
                updateQueue.pop_front();
                DPRINTF(WLEngine,  "%s: Popped an item from the front of updateQueue"
                            ". updateQueue.size = %u.\n",
                            __func__, updateQueue.size());
                respPort.checkRetryReq();
            }
        }
    } else {
        // TODO: Generalize this to reduce function rather than just min
        DPRINTF(WLEngine,  "%s: Found the addr: %lu in onTheFlyUpdateMap. "
                    "onTheFlyUpdateMap[%lu] = %u.\n", __func__, update_addr,
                    update_addr, onTheFlyUpdateMap[update_addr]);
        onTheFlyUpdateMap[update_addr] =
                std::min(update_value, onTheFlyUpdateMap[update_addr]);
        DPRINTF(WLEngine,  "%s: Reduced the update_value with the entry in "
                    "onTheFlyUpdateMap. onTheFlyUpdateMap[%lu] = %u.\n",
                    __func__, update_addr, onTheFlyUpdateMap[update_addr]);
        stats.onTheFlyCoalesce++;
        updateQueue.pop_front();
        DPRINTF(WLEngine,  "%s: Popped an item from the front of updateQueue"
                                        ". updateQueue.size = %u.\n",
                                        __func__, updateQueue.size());
        respPort.checkRetryReq();
    }

    // TODO: Only schedule nextReadEvent only when it has to be scheduled
    if ((!nextReadEvent.scheduled()) && (!updateQueue.empty())) {
        schedule(nextReadEvent, nextCycle());
    }
}

void
WLEngine::handleIncomingWL(Addr addr, WorkListItem wl)
{
    assert(addrWorkListMap.size() <= onTheFlyUpdateMapSize);

    addrWorkListMap[addr] = wl;
    DPRINTF(WLEngine,  "%s: Received a WorkListItem from the coalesceEngine. Adding"
                " it to the addrWorkListMap. addrWorkListMap[%lu] = %s.\n",
                __func__, addr, wl.to_string());

    assert(!addrWorkListMap.empty());
    if (!nextReduceEvent.scheduled()) {
        schedule(nextReduceEvent, nextCycle());
    }
}

void
WLEngine::processNextReduceEvent()
{
    for (auto &it : addrWorkListMap) {
        Addr addr = it.first;
        assert(onTheFlyUpdateMap.find(addr) != onTheFlyUpdateMap.end());
        uint32_t update_value = onTheFlyUpdateMap[addr];
        DPRINTF(WLEngine,  "%s: Reducing between onTheFlyUpdateMap and "
                    "addrWorkListMap values. onTheFlyUpdateMap[%lu] = %u, "
                    "addrWorkListMap[%lu] = %s.\n", __func__,
                                addr, onTheFlyUpdateMap[addr],
                                addr, addrWorkListMap[addr].to_string());
        // TODO: Generalize this to reduce function rather than just min
        addrWorkListMap[addr].tempProp =
                    std::min(update_value, addrWorkListMap[addr].tempProp);
        DPRINTF(WLEngine,  "%s: Reduction done. addrWorkListMap[%lu] = %s.\n",
                    __func__, addr, addrWorkListMap[addr].to_string());
        stats.numReduce++;

        coalesceEngine->recvWLWrite(addr, addrWorkListMap[addr]);
        onTheFlyUpdateMap.erase(addr);
        DPRINTF(WLEngine,  "%s: Erased addr: %lu from onTheFlyUpdateMap.\n",
                    __func__, addr);
    }
    addrWorkListMap.clear();
}

bool
WLEngine::handleIncomingUpdate(PacketPtr pkt)
{
    assert(updateQueue.size() <= updateQueueSize);
    if ((updateQueueSize != 0) && (updateQueue.size() == updateQueueSize)) {
        return false;
    }

    if (curTick() == ) {
        std
    }

    updateQueue.emplace_back(pkt->getAddr(), pkt->getLE<uint32_t>());
    DPRINTF(WLEngine,  "%s: Pushed an item to the back of updateQueue"
                                        ". updateQueue.size = %u.\n",
                                        __func__, updateQueue.size());
    delete pkt;
    assert(!updateQueue.empty());
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
    ADD_STAT(onTheFlyCoalesce, statistics::units::Count::get(),
             "Number of memory blocks read for vertecies")
{
}

void
WLEngine::WorkListStats::regStats()
{
    using namespace statistics;
}

}
