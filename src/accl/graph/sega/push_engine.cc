/*
 * Copyright (c) 2021 The Regents of the University of California.
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

#include "accl/graph/sega/push_engine.hh"

#include "accl/graph/sega/mpu.hh"
#include "debug/PushEngine.hh"
#include "mem/packet_access.hh"
#include "sim/sim_exit.hh"

namespace gem5
{

PushEngine::PushEngine(const Params& params):
    BaseMemoryEngine(params),
    draining(false), fastMode(false),
    _running(false),
    lastIdleEntranceTick(0),
    numPendingPulls(0), edgePointerQueueSize(params.push_req_queue_size),
    onTheFlyMemReqs(0), edgeQueueSize(params.resp_queue_size),
    maxPropagatesPerCycle(params.max_propagates_per_cycle),
    updateQueueSize(params.update_queue_size),
    nextVertexPullEvent([this] { processNextVertexPullEvent(); }, name()),
    nextMemoryReadEvent([this] { processNextMemoryReadEvent(); }, name()),
    nextPropagateEvent([this] { processNextPropagateEvent(); }, name()),
    nextUpdatePushEvent([this] { processNextUpdatePushEvent(); }, name()),
    nextFastEvent([this] { processNextFastEvent(); }, name()),
    stats(*this)
{
    for (int i = 0; i < params.port_out_ports_connection_count; ++i) {
        outPorts.emplace_back(
                        name() + ".out_ports" + std::to_string(i), this, i);
    }
}

Port&
PushEngine::getPort(const std::string& if_name, PortID idx)
{
    if (if_name == "out_ports") {
        return outPorts[idx];
    } else if (if_name == "mem_port") {
        return BaseMemoryEngine::getPort(if_name, idx);
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

void
PushEngine::init()
{
    localAddrRange = owner->getAddrRanges();
    for (int i = 0; i < outPorts.size(); i++){
        portAddrMap[outPorts[i].id()] = outPorts[i].getAddrRanges();
    }
}

void
PushEngine::registerMPU(MPU* mpu)
{
    owner = mpu;
}

void
PushEngine::ReqPort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr,
            "Should never try to send if blocked!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt))
    {
        DPRINTF(PushEngine, "%s: Packet is blocked.\n", __func__);
        blockedPacket = pkt;
    }
}

bool
PushEngine::ReqPort::recvTimingResp(PacketPtr pkt)
{
    panic("recvTimingResp called on the request port.");
}

void
PushEngine::ReqPort::recvReqRetry()
{
    panic_if(blockedPacket == nullptr,
            "Received retry without a blockedPacket.");

    DPRINTF(PushEngine, "%s: ReqPort %d received a reqRetry. blockedPacket: %s.\n", __func__, _id, blockedPacket->print());
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);
    if (blockedPacket == nullptr) {
        DPRINTF(PushEngine, "%s: blockedPacket sent successfully.\n", __func__);
        owner->recvReqRetry();
    }
}

void
PushEngine::recvReqRetry()
{
    DPRINTF(PushEngine, "%s: Received a reqRetry.\n", __func__);
    if (!nextUpdatePushEvent.scheduled()) {
        schedule(nextUpdatePushEvent, nextCycle());
    }
}

bool
PushEngine::vertexSpace()
{
    return (edgePointerQueueSize == 0) ||
        ((edgePointerQueue.size() + numPendingPulls) < edgePointerQueueSize);
}

bool
PushEngine::workLeft()
{
    return ((owner->workCount() - numPendingPulls) > 0);
}

bool
PushEngine::done()
{
    bool empty_update_queues = true;
    for (int i = 0; i < outPorts.size(); i++) {
        empty_update_queues &= updateQueues[outPorts[i].id()].empty();
    }
    return empty_update_queues && metaEdgeQueue.empty() &&
        (onTheFlyMemReqs == 0) && edgePointerQueue.empty();
}

bool
PushEngine::doneDrain()
{
    return done();
}

void
PushEngine::resumeAfterDrain()
{
    if (workLeft() && vertexSpace()) {
        if (fastMode) {
            if (!nextFastEvent.scheduled()) {
                schedule(nextFastEvent, nextCycle());
            }
        } else {
            if (!nextVertexPullEvent.scheduled()) {
                schedule(nextVertexPullEvent, nextCycle());
            }
        }
    }
}

uint32_t
PushEngine::reduce(uint32_t update, uint32_t value)
{
    std::string workload = params().workload;
    uint32_t new_value;
    if(workload == "BFS"){
        new_value = std::min(update, value);
    } else{
        panic("Workload not implemented\n");
    }
    return new_value;
}

uint32_t
PushEngine::propagate(uint32_t value, uint32_t weight)
{
    std::string workload = params().workload;
    uint32_t update;
    if (workload == "BFS")  {
        update = value + 1;
    }
    else{
        panic("The workload %s is not supported", workload);
    }
    return update;
}

void
PushEngine::start()
{
    assert(!_running);
    assert(!nextVertexPullEvent.scheduled());

    _running = true;
    stats.numIdleCycles += ticksToCycles(curTick() - lastIdleEntranceTick);
    // NOTE: We might have to check for size availability here.
    assert(workLeft());
    if (vertexSpace()) {
        if (fastMode) {
            schedule(nextFastEvent, nextCycle());
        } else {
            schedule(nextVertexPullEvent, nextCycle());
        }
    }
}

void
PushEngine::processNextVertexPullEvent()
{
    if (draining) {
        return;
    }
    numPendingPulls++;
    owner->recvVertexPull();

    if (!workLeft()) {
        _running = false;
        lastIdleEntranceTick = curTick();
    }

    if (workLeft() && vertexSpace() && (!nextVertexPullEvent.scheduled())) {
        schedule(nextVertexPullEvent, nextCycle());
    }
}

void
PushEngine::recvVertexPush(Addr addr, WorkListItem wl)
{
    assert(wl.degree > 0);
    assert((edgePointerQueueSize == 0) ||
            ((edgePointerQueue.size() + numPendingPulls) <= edgePointerQueueSize));

    Addr start_addr = wl.edgeIndex * sizeof(Edge);
    Addr end_addr = start_addr + (wl.degree * sizeof(Edge));

    edgePointerQueue.emplace_back(
                            start_addr, end_addr, sizeof(Edge),
                            peerMemoryAtomSize, addr,
                            (uint32_t) wl.prop, curTick());
    numPendingPulls--;
    if (workLeft() && vertexSpace() && (!nextVertexPullEvent.scheduled())) {
        schedule(nextVertexPullEvent, nextCycle());
    }

    if ((!nextMemoryReadEvent.pending()) &&
        (!nextMemoryReadEvent.scheduled())) {
        schedule(nextMemoryReadEvent, nextCycle());
    }
}

void
PushEngine::processNextMemoryReadEvent()
{
    if (memPort.blocked()) {
        nextMemoryReadEvent.sleep();
        return;
    }
    Addr aligned_addr, offset;
    int num_edges;

    EdgeReadInfoGen& curr_info = edgePointerQueue.front();
    std::tie(aligned_addr, offset, num_edges) = curr_info.nextReadPacketInfo();
    if (metaEdgeQueue.size() < (edgeQueueSize - (onTheFlyMemReqs + num_edges)))
    {
        DPRINTF(PushEngine, "%s: Current packet information generated by "
                    "EdgeReadInfoGen. aligned_addr: %lu, offset: %lu, "
                    "num_edges: %d.\n", __func__, aligned_addr, offset, num_edges);

        PacketPtr pkt = createReadPacket(aligned_addr, peerMemoryAtomSize);
        PushInfo push_info = {curr_info.src(), curr_info.value(), offset, num_edges};
        reqInfoMap[pkt->req] = push_info;

        memPort.sendPacket(pkt);
        onTheFlyMemReqs += num_edges;

        curr_info.iterate();
        if (curr_info.done()) {
            DPRINTF(PushEngine, "%s: Current EdgeReadInfoGen is done.\n", __func__);
            stats.edgePointerQueueLatency.sample(
                                (curTick() - curr_info.entrance()) *
                                1e9 / getClockFrequency());
            edgePointerQueue.pop_front();
            DPRINTF(PushEngine, "%s: Popped curr_info from edgePointerQueue. "
            "edgePointerQueue.size() = %u.\n", __func__, edgePointerQueue.size());
        }
    }

    if (workLeft() && vertexSpace() && (!nextVertexPullEvent.scheduled())) {
        schedule(nextVertexPullEvent, nextCycle());
    }

    if (!edgePointerQueue.empty()) {
        assert(!nextMemoryReadEvent.pending());
        assert(!nextMemoryReadEvent.scheduled());
        schedule(nextMemoryReadEvent, nextCycle());
    }
}

void
PushEngine::recvMemRetry()
{
    if (nextMemoryReadEvent.pending()) {
        DPRINTF(PushEngine, "%s: Received a memory retry.\n", __func__);
        nextMemoryReadEvent.wake();
        schedule(nextMemoryReadEvent, nextCycle());
    }
}

bool
PushEngine::handleMemResp(PacketPtr pkt)
{
    // TODO: in case we need to edit edges, get rid of second statement.
    assert(pkt->isResponse() && (!pkt->isWrite()));

    uint8_t* pkt_data = new uint8_t [peerMemoryAtomSize];
    PushInfo push_info = reqInfoMap[pkt->req];
    pkt->writeDataToBlock(pkt_data, peerMemoryAtomSize);

    for (int i = 0; i < push_info.numElements; i++) {
        Edge* edge = (Edge*) (pkt_data + push_info.offset + i * sizeof(Edge));
        Addr edge_dst = edge->neighbor;
        uint32_t edge_weight = edge->weight;
        MetaEdge meta_edge(
                    push_info.src, edge_dst, edge_weight, push_info.value);
        metaEdgeQueue.emplace_back(meta_edge, curTick());
    }
    stats.numWastefulEdgesRead +=
                (peerMemoryAtomSize / sizeof(Edge)) - push_info.numElements;

    onTheFlyMemReqs -= push_info.numElements;
    reqInfoMap.erase(pkt->req);
    delete pkt_data;
    delete pkt;

    if (!nextPropagateEvent.scheduled()) {
        schedule(nextPropagateEvent, nextCycle());
    }
    return true;
}

void
PushEngine::processNextPropagateEvent()
{
    int num_propagates = 0;
    while(true) {
        MetaEdge meta_edge;
        Tick entrance_tick;
        std::tie(meta_edge, entrance_tick) = metaEdgeQueue.front();

        DPRINTF(PushEngine, "%s: The edge to process is %s.\n",
                                __func__, meta_edge.to_string());

        uint32_t update_value = propagate(meta_edge.value, meta_edge.weight);
        Update update(meta_edge.src, meta_edge.dst, update_value);
        metaEdgeQueue.pop_front();

        if (enqueueUpdate(update)) {
            DPRINTF(PushEngine, "%s: Sent %s to port queues.\n",
                                            __func__, meta_edge.to_string());
            stats.numPropagates++;
            stats.edgeQueueLatency.sample(
                    (curTick() - entrance_tick) * 1e9 / getClockFrequency());
        } else {
            metaEdgeQueue.emplace_back(meta_edge, entrance_tick);
        }
        num_propagates++;

        if (metaEdgeQueue.empty()) {
            break;
        }
        if (num_propagates >= maxPropagatesPerCycle) {
            break;
        }
    }

    stats.numPropagatesHist.sample(num_propagates);

    assert(!nextPropagateEvent.scheduled());
    if (!metaEdgeQueue.empty()) {
        schedule(nextPropagateEvent, nextCycle());
    }
}

bool
PushEngine::enqueueUpdate(Update update)
{
    Addr dst_addr = update.dst;
    bool found_coalescing = false;
    bool found_locally = false;
    bool accepted = false;
    for (auto range : localAddrRange) {
        found_locally |= range.contains(dst_addr);
    }
    DPRINTF(PushEngine, "%s: Received update: %s.\n", __func__, update.to_string());
    for (int i = 0; i < outPorts.size(); i++) {
        AddrRangeList addr_range_list = portAddrMap[outPorts[i].id()];
        if (contains(addr_range_list, dst_addr)) {
            DPRINTF(PushEngine, "%s: Update: %s belongs to port %d.\n",
                        __func__, update.to_string(), outPorts[i].id());
            DPRINTF(PushEngine, "%s: There are %d updates already "
                        "in queue for port %d.\n", __func__,
                        updateQueues[outPorts[i].id()].size(),
                        outPorts[i].id());
            for (auto& entry: updateQueues[outPorts[i].id()]) {
                Update& curr_update = std::get<0>(entry);
                if (curr_update.dst == update.dst) {
                    uint32_t old_value = curr_update.value;
                    curr_update.value = reduce(old_value, update.value);
                    DPRINTF(PushEngine, "%s: found a coalescing opportunity "
                            "for destination %d with new value: %d by "
                            "coalescing %d and %d. \n", __func__, update.dst,
                            curr_update.value, old_value, update.value);
                    found_coalescing = true;
                    accepted = true;
                    stats.updateQueueCoalescions++;
                }
            }
            if ((found_coalescing == false) &&
                (updateQueues[outPorts[i].id()].size() < updateQueueSize)) {
                DPRINTF(PushEngine, "%s: There is a free entry available "
                            "in queue %d.\n", __func__, outPorts[i].id());
                updateQueues[outPorts[i].id()].emplace_back(update, curTick());
                DPRINTF(PushEngine, "%s: Emplaced the update at the back "
                            "of queue for port %d is. Size of queue "
                            "for port %d is %d.\n", __func__,
                            outPorts[i].id(), outPorts[i].id(),
                            updateQueues[outPorts[i].id()].size());
                accepted = true;
                stats.updateQueueLength.sample(
                                        updateQueues[outPorts[i].id()].size());
            }
        }
    }

    if (accepted && (!nextUpdatePushEvent.scheduled())) {
        schedule(nextUpdatePushEvent, nextCycle());
    }

    return accepted;
}

template<typename T> PacketPtr
PushEngine::createUpdatePacket(Addr addr, T value)
{
    RequestPtr req = std::make_shared<Request>(addr, sizeof(T), 0, 0);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr) 1) << 2);

    // FIXME: MemCmd::UpdateWL
    PacketPtr pkt = new Packet(req, MemCmd::UpdateWL);

    pkt->allocate();
    // pkt->setData(data);
    pkt->setLE<T>(value);

    return pkt;
}

void
PushEngine::processNextUpdatePushEvent()
{
    int next_time_send = 0;

    for (int i = 0; i < outPorts.size(); i++) {
        if (outPorts[i].blocked()) {
            DPRINTF(PushEngine, "%s: Port %d blocked.\n",
                                __func__, outPorts[i].id());
            continue;
        }
        DPRINTF(PushEngine, "%s: Port %d available.\n",
                                __func__, outPorts[i].id());
        if (updateQueues[outPorts[i].id()].empty()) {
            DPRINTF(PushEngine, "%s: Respective queue for port "
                        "%d is empty.\n", __func__, outPorts[i].id());
            continue;
        }
        DPRINTF(PushEngine, "%s: Respective queue for port "
                        "%d not empty.\n", __func__, outPorts[i].id());
        Update update;
        Tick entrance_tick;
        std::tie(update, entrance_tick) = updateQueues[outPorts[i].id()].front();
        PacketPtr pkt = createUpdatePacket<uint32_t>(update.dst, update.value);
        outPorts[i].sendPacket(pkt);
        DPRINTF(PushEngine, "%s: Sent update: %s to port %d. "
                    "Respective queue size is %d.\n", __func__,
                    update.to_string(), outPorts[i].id(),
                    updateQueues[outPorts[i].id()].size());
        updateQueues[outPorts[i].id()].pop_front();
        if (updateQueues[outPorts[i].id()].size() > 0) {
            next_time_send += 1;
        }
        stats.numUpdates++;
    }

    assert(!nextUpdatePushEvent.scheduled());
    if (next_time_send > 0) {
        schedule(nextUpdatePushEvent, nextCycle());
    }
}

void
PushEngine::processNextFastEvent()
{
    if (atomicInfo.done()) {
        std::tie(atomicAddr, atomicWl) = owner->recvFunctionalVertexPull();
        Addr start_addr = atomicWl.edgeIndex * sizeof(Edge);
        Addr end_addr = start_addr + (atomicWl.degree * sizeof(Edge));
        atomicInfo = EdgeReadInfoGen(start_addr, end_addr, sizeof(Edge),
                                peerMemoryAtomSize, atomicAddr, atomicWl.prop, curTick());
    }

    while(true) {
        Addr aligned_addr, offset;
        int num_edges;
        std::tie(aligned_addr, offset, num_edges) =
                                            atomicInfo.nextReadPacketInfo();
        if (metaEdgeQueue.size() >=
            (edgeQueueSize - (onTheFlyMemReqs + num_edges))) {
            break;
        }
        PacketPtr pkt = createReadPacket(aligned_addr, peerMemoryAtomSize);
        memPort.sendFunctional(pkt);
        uint8_t* pkt_data = new uint8_t [peerMemoryAtomSize];
        for (int i = 0; i < num_edges; i++) {
            Edge* edge = (Edge*) (pkt_data + offset + i * sizeof(Edge));
            Addr edge_dst = edge->neighbor;
            uint32_t edge_weight = edge->weight;
            MetaEdge meta_edge(
                            atomicAddr, edge_dst, edge_weight, atomicWl.prop);
            metaEdgeQueue.emplace_back(meta_edge, curTick());
        }
        atomicInfo.iterate();
        if (atomicInfo.done()) {
            break;
        }
    }

    if (!metaEdgeQueue.empty() && !nextPropagateEvent.scheduled()) {
        schedule(nextPropagateEvent, nextCycle());
    }
    if (workLeft() || !atomicInfo.done()) {
        schedule(nextFastEvent, nextCycle());
    } else {
        _running = false;
    }
}

PushEngine::PushStats::PushStats(PushEngine &_push)
    : statistics::Group(&_push),
    push(_push),
    ADD_STAT(numPropagates, statistics::units::Count::get(),
             "Number of propagate operations done."),
    ADD_STAT(numNetBlocks, statistics::units::Count::get(),
             "Number of updates blocked by network."),
    ADD_STAT(numIdleCycles, statistics::units::Count::get(),
             "Number of cycles PushEngine has been idle."),
    ADD_STAT(updateQueueCoalescions, statistics::units::Count::get(),
             "Number of coalescions in the update queues."),
    ADD_STAT(numUpdates, statistics::units::Count::get(),
             "Number of updates sent to the network."),
    ADD_STAT(numWastefulEdgesRead, statistics::units::Count::get(),
             "Number of wasteful edges read from edge memory."),
    ADD_STAT(TEPS, statistics::units::Rate<statistics::units::Count,
                                    statistics::units::Second>::get(),
             "Traversed Edges Per Second."),
    ADD_STAT(edgePointerQueueLatency, statistics::units::Second::get(),
             "Histogram of the latency of the edgePointerQueue."),
    ADD_STAT(edgeQueueLatency, statistics::units::Second::get(),
             "Histogram of the latency of the metaEdgeQueue."),
    ADD_STAT(updateQueueLength, statistics::units::Count::get(),
             "Histogram of the length of updateQueues."),
    ADD_STAT(numPropagatesHist, statistics::units::Count::get(),
             "Histogram of number of propagates sent.")
{
}

void
PushEngine::PushStats::regStats()
{
    using namespace statistics;

    TEPS = numPropagates / simSeconds;

    edgePointerQueueLatency.init(64);
    edgeQueueLatency.init(64);
    updateQueueLength.init(64);
    numPropagatesHist.init(push.params().max_propagates_per_cycle);
}

} // namespace gem5
