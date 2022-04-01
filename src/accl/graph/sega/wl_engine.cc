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
    BaseReduceEngine(params),
    respPort(name() + ".resp_port", this),
    blockedByCoalescer(false),
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

AddrRangeList
WLEngine::RespPort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

bool
WLEngine::RespPort::recvTimingReq(PacketPtr pkt)
{
    return owner->handleIncomingUpdate(pkt);
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

void
WLEngine::processNextReadEvent()
{
    PacketPtr update = updateQueue.front();
    Addr update_addr = update->getAddr();
    uint32_t* update_value = update->getPtr<uint32_t>();

    // FIXME: else logic is wrong
    if ((onTheFlyUpdateMap.find(update_addr) == onTheFlyUpdateMap.end()) &&
        (onTheFlyUpdateMap.size() < onTheFlyUpdateMapSize)) {
        if (coalesceEngine->recvReadAddr(update_addr)) {
            DPRINTF(MPU, "%s: Received an update and it's not been pulled in. "
                            "update_addr: %lu, update_value: %u.\n",
                            __func__, update_addr, *update_value);
            onTheFlyUpdateMap[update_addr] = *update_value;
            DPRINTF(MPU, "%s: onTheFlyUpdateMap[%lu] = %d.\n",
                __func__, update_addr, onTheFlyUpdateMap[update_addr]);
            updateQueue.pop_front();
            DPRINTF(MPU, "%s: updateQueue.size: %d.\n", __func__, updateQueue.size());
        }
    } else {
        // TODO: Generalize this to reduce function rather than just min
        DPRINTF(MPU, "%s: Hitting in the onTheFlyUpdateMap."
                            "update_addr: %lu, update_value: %u, old_value: %u.\n",
                            __func__, update_addr, *update_value,
                            onTheFlyUpdateMap[update_addr]);
        onTheFlyUpdateMap[update_addr] =
                std::min(*update_value, onTheFlyUpdateMap[update_addr]);
        stats.onTheFlyCoalesce++;
        updateQueue.pop_front();
        DPRINTF(MPU, "%s: updateQueue.size: %d.\n", __func__, updateQueue.size());
        // TODO: Add a stat to count the number of coalescions
    }

    // TODO: Only schedule nextReadEvent only when it has to be scheduled
    if ((!nextReadEvent.scheduled()) &&
        (!updateQueue.empty())) {
        schedule(nextReadEvent, nextCycle());
    }
}

void
WLEngine::handleIncomingWL(Addr addr, WorkListItem wl)
{
    assert(addrWorkListMap.size() <= onTheFlyUpdateMapSize);
    addrWorkListMap[addr] = wl;
    // TODO: Add checks to see if scheduling is necessary or correct.
    if ((!nextReduceEvent.scheduled()) && (!addrWorkListMap.empty())) {
        schedule(nextReduceEvent, nextCycle());
    }
}

void
WLEngine::processNextReduceEvent()
{

    std::unordered_map<Addr, WorkListItem>::iterator it =
                    addrWorkListMap.begin();

    std::vector<Addr> servicedAddresses;
    while (it != addrWorkListMap.end()) {
        Addr addr = it->first;
        WorkListItem wl = it->second;
        uint32_t update_value = onTheFlyUpdateMap[addr];
        DPRINTF(MPU, "%s: updating WorkList[%lu] with the current temp_prop: "
                    "%d, with new update: %d.\n", __func__, addr, wl.tempProp,
                    onTheFlyUpdateMap[addr]);
        // TODO: Generalize this to reduce function rather than just min
        wl.tempProp = std::min(update_value, wl.tempProp);
        stats.numReduce++;

        coalesceEngine->recvWLWrite(addr, wl);
        servicedAddresses.push_back(addr);
        it++;
    }

    addrWorkListMap.clear();
    for (int i = 0; i < servicedAddresses.size(); i++) {
        onTheFlyUpdateMap.erase(servicedAddresses[i]);
    }
}

bool
WLEngine::handleIncomingUpdate(PacketPtr pkt)
{
    assert(updateQueue.size() <= updateQueueSize);
    if ((updateQueueSize != 0) && (updateQueue.size() == updateQueueSize)) {
        return false;
    }

    updateQueue.push_back(pkt);
    assert(!updateQueue.empty());
    DPRINTF(MPU, "%s: updateQueue.size: %d.\n", __func__, updateQueue.size());
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
