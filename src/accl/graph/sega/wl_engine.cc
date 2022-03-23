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
    nextReduceEvent([this]{ processNextReduceEvent(); }, name())
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
WLEngine::startup()
{
    //FIXME: This is the current version of our initializer.
    // This should be updated in the future.
    //FIXME: The WLEngine no longer has a MemPort. Update this to
    // work with the CoalesceEngine instead.
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
        coalesceEngine->recvFunctional(pkt);
    }

    for (int i = 0; i < 7; i++) {
        uint8_t* data = edgeToMemory(edges[i]);
        PacketPtr pkt = getWritePacket(1048576 + i * sizeof(Edge),
                                        16, data, 0);
        coalesceEngine->recvFunctional(pkt);
    }

    uint8_t* first_update_data = new uint8_t [4];
    uint32_t* tempPtr = (uint32_t*) first_update_data;
    *tempPtr = 0;

    PacketPtr first_update = getUpdatePacket(
        0, 4, first_update_data, _requestorId);

    handleIncomingUpdate(first_update);
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

    if ((onTheFlyUpdateMap.find(update_addr) == onTheFlyUpdateMap.end()) &&
        (onTheFlyUpdateMap.size() < onTheFlyUpdateMapSize)) {
        if (coalesceEngine->recvReadAddr(update_addr)) {
            onTheFlyUpdateMap[update_addr] = *update_value;
            updateQueue.pop();
        }
    } else {
        // TODO: Generalize this to reduce function rather than just min
        onTheFlyUpdateMap[update_addr] =
                std::min(*update_value, onTheFlyUpdateMap[update_addr]);
        updateQueue.pop();
        // TODO: Add a stat to count the number of coalescions
    }

    if ((!nextReadEvent.scheduled()) &&
        ((!updateQueue.empty()) ||
        (onTheFlyUpdateMap.size() < onTheFlyUpdateMapSize))) {
        schedule(nextReadEvent, nextCycle());
    }
}

void
WLEngine::processNextReduceEvent()
{
    // TODO: Generalize this to reduce function rather than just min
    currentWorkList.temp_prop = std::min(
                                onTheFlyUpdateMap[currentWorkListAddress],
                                currentWorkList.temp_prop);
    // TODO: Add a delay here
    coalesceEngine->recvWLWrite(currentWorkListAddress, currentWorkList);

    onTheFlyUpdateMap.erase(currentWorkListAddress);
    currentWorkListAddress = 0;
    currentWorkList = {0, 0, 0, 0};
}

void
WLEngine::scheduleReduceEvent()
{
    // TODO: Add checks to see if scheduling is necessary or correct.
    if (!nextReduceEvent.scheduled()) {
        schedule(nextReduceEvent, nextCycle());
    }
}

bool
WLEngine::handleIncomingUpdate(PacketPtr pkt)
{
    // TODO: Coalesce updates here too
    assert(updateQueue.size() <= updateQueueSize);
    if ((updateQueueSize != 0) && (updateQueue.size() == updateQueueSize)) {
        return false;
    }

    updateQueue.push(pkt);
    if ((!nextReadEvent.scheduled()) &&
        (!updateQueue.empty())) {
        schedule(nextReadEvent, nextCycle());
    }
    return true;
}

}
