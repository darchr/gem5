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
#include "base/intmath.hh"
#include "debug/PushEngine.hh"
#include "mem/packet_access.hh"
#include "sim/sim_exit.hh"

namespace gem5
{

PushEngine::PushEngine(const Params& params):
    BaseMemoryEngine(params),
    _running(false),
    lastIdleEntranceTick(0),
    numPendingPulls(0), edgePointerQueueSize(params.push_req_queue_size),
    onTheFlyMemReqs(0), edgeQueueSize(params.resp_queue_size),
    examineWindow(params.examine_window),
    maxPropagatesPerCycle(params.max_propagates_per_cycle),
    updateQueueSize(params.update_queue_size),
    nextVertexPullEvent([this] { processNextVertexPullEvent(); }, name()),
    nextMemoryReadEvent([this] { processNextMemoryReadEvent(); }, name()),
    nextPropagateEvent([this] { processNextPropagateEvent(); }, name()),
    nextUpdatePushEvent([this] { processNextUpdatePushEvent(); }, name()),
    stats(*this)
{
    destinationQueues.clear();
    for (int i = 0; i < params.port_out_ports_connection_count; ++i) {
        outPorts.emplace_back(name() + ".out_ports" + std::to_string(i), this, i);
        destinationQueues.emplace_back();
        destinationQueues[i].clear();
        sourceAndValueMaps.emplace_back();
        sourceAndValueMaps[i].clear();
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
        AddrRangeList range_list = outPorts[i].getAddrRanges();
        assert(range_list.size() == 1);
        AddrRange range = outPorts[i].getAddrRanges().front();
        portAddrMap.insert(range, i);
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

    DPRINTF(PushEngine, "%s: ReqPort %d received a reqRetry. "
            "blockedPacket: %s.\n", __func__, _id, blockedPacket->print());
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
        empty_update_queues &= destinationQueues[i].empty();
    }
    return empty_update_queues && metaEdgeQueue.empty() &&
        (onTheFlyMemReqs == 0) && edgePointerQueue.empty();
}

void
PushEngine::start()
{
    assert(!_running);
    // assert(!nextVertexPullEvent.scheduled());

    _running = true;
    // stats.numIdleCycles += ticksToCycles(curTick() - lastIdleEntranceTick);
    // NOTE: We might have to check for size availability here.
    assert(workLeft());
    if (vertexSpace() && !nextVertexPullEvent.scheduled()) {
        schedule(nextVertexPullEvent, nextCycle());
    }
}

void
PushEngine::processNextVertexPullEvent()
{
    if (workLeft()) {
        numPendingPulls++;
        owner->recvVertexPull();
        if (vertexSpace() && (!nextVertexPullEvent.scheduled())) {
            schedule(nextVertexPullEvent, nextCycle());
        }
    } else {
        _running = false;
        lastIdleEntranceTick = curTick();
        DPRINTF(PushEngine, "%s: In idle state now.\n", __func__);
    }
}

void
PushEngine::recvVertexPush(Addr addr, uint32_t delta,
                            uint32_t edge_index, uint32_t degree)
{
    assert(degree > 0);
    assert((edgePointerQueueSize == 0) ||
            ((edgePointerQueue.size() + numPendingPulls) <= edgePointerQueueSize));

    Addr start_addr = edge_index * sizeof(Edge);
    Addr end_addr = start_addr + (degree * sizeof(Edge));
    EdgeReadInfoGen info_gen(addr, delta, start_addr, end_addr,
                            sizeof(Edge), peerMemoryAtomSize);

    edgePointerQueue.emplace_back(info_gen, curTick());
    stats.edgePointerQueueLength.sample(edgePointerQueue.size());
    numPendingPulls--;

    if (vertexSpace() && (!nextVertexPullEvent.scheduled())) {
        schedule(nextVertexPullEvent, nextCycle());
    }

    if ((!nextMemoryReadEvent.pending()) &&
        (!nextMemoryReadEvent.scheduled())) {
        schedule(nextMemoryReadEvent, nextCycle());
    }
}

void
PushEngine::recvMirrorPush(Addr addr, uint32_t delta,
                            uint32_t edge_index, uint32_t degree)
{
    Addr start_addr = edge_index * sizeof(Edge);
    Addr end_addr = start_addr + (degree * sizeof(Edge));
    EdgeReadInfoGen info_gen(addr, delta, start_addr, end_addr,
                            sizeof(Edge), peerMemoryAtomSize);

    edgePointerQueue.emplace_back(info_gen, curTick());
    stats.edgePointerQueueLength.sample(edgePointerQueue.size());
}

void
PushEngine::startProcessingMirrors(Tick time_to_wait)
{
    assert(!nextMemoryReadEvent.pending());
    assert(!nextMemoryReadEvent.scheduled());
    Cycles wait = ticksToCycles(time_to_wait);
    if (!edgePointerQueue.empty()) {
        schedule(nextMemoryReadEvent, clockEdge(wait));
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

    EdgeReadInfoGen& curr_info = std::get<0>(edgePointerQueue.front());
    Tick entrance_tick = std::get<1>(edgePointerQueue.front());
    std::tie(aligned_addr, offset, num_edges) = curr_info.nextReadPacketInfo();
    if (metaEdgeQueue.size() < (edgeQueueSize - (onTheFlyMemReqs + num_edges)))
    {
        DPRINTF(PushEngine, "%s: Current packet information generated by "
                    "EdgeReadInfoGen. aligned_addr: %lu, offset: %lu, "
                    "num_edges: %d.\n", __func__, aligned_addr, offset, num_edges);

        PacketPtr pkt = createReadPacket(aligned_addr, peerMemoryAtomSize);
        PushInfo push_info = {curr_info.src(), curr_info.delta(), offset, num_edges};
        reqInfoMap[pkt->req] = push_info;
        memPort.sendPacket(pkt);
        onTheFlyMemReqs += num_edges;

        curr_info.iterate();
        if (curr_info.done()) {
            DPRINTF(PushEngine, "%s: Current EdgeReadInfoGen is done.\n", __func__);
            stats.edgePointerQueueLatency.sample(
                    (curTick() - entrance_tick) * 1e9 / getClockFrequency());
            edgePointerQueue.pop_front();
            stats.edgePointerQueueLength.sample(edgePointerQueue.size());
            DPRINTF(PushEngine, "%s: Popped curr_info from edgePointerQueue. "
            "edgePointerQueue.size() = %u.\n", __func__, edgePointerQueue.size());
        }
    }

    if (vertexSpace() && (!nextVertexPullEvent.scheduled())) {
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

    uint8_t pkt_data [peerMemoryAtomSize];
    PushInfo push_info = reqInfoMap[pkt->req];
    pkt->writeDataToBlock(pkt_data, peerMemoryAtomSize);

    for (int i = 0; i < push_info.numElements; i++) {
        Edge* edge = (Edge*) (pkt_data + push_info.offset + i * sizeof(Edge));
        Addr edge_dst = edge->neighbor;
        uint32_t edge_weight = edge->weight;
        MetaEdge meta_edge(
                    push_info.src, edge_dst, edge_weight, push_info.value);
        metaEdgeQueue.emplace_back(meta_edge, curTick());
        stats.edgeQueueLength.sample(metaEdgeQueue.size());
    }
    stats.edgeQueueLength.sample(metaEdgeQueue.size());
    stats.numWastefulEdgesRead +=
                (peerMemoryAtomSize / sizeof(Edge)) - push_info.numElements;

    onTheFlyMemReqs -= push_info.numElements;
    reqInfoMap.erase(pkt->req);

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
    int num_tries = 0;
    int num_reads = 0;
    std::deque<std::tuple<MetaEdge, Tick>> temp_edge;
    for (int i = 0; i < examineWindow; i++) {
        if (metaEdgeQueue.empty()) {
            break;
        }
        temp_edge.push_back(metaEdgeQueue.front());
        metaEdgeQueue.pop_front();
    }
    int max_visits = temp_edge.size();

    while(true) {
        MetaEdge meta_edge;
        Tick entrance_tick;
        std::tie(meta_edge, entrance_tick) = temp_edge.front();

        DPRINTF(PushEngine, "%s: The edge to process is %s.\n",
                                __func__, meta_edge.to_string());

        uint32_t update_value =
                graphWorkload->propagate(meta_edge.value, meta_edge.weight);
        temp_edge.pop_front();
        num_tries++;

        if (enqueueUpdate(meta_edge.src, meta_edge.dst, update_value)) {
            DPRINTF(PushEngine, "%s: Sent %s to port queues.\n",
                                            __func__, meta_edge.to_string());
            num_reads++;
            stats.numPropagates++;
            stats.edgeQueueLatency.sample(
                    (curTick() - entrance_tick) * 1e9 / getClockFrequency());
        } else {
            temp_edge.emplace_back(meta_edge, entrance_tick);
            stats.updateQueueFull++;
        }
        num_propagates++;

        if (temp_edge.empty()) {
            break;
        }
        if (num_tries >= max_visits) {
            break;
        }
    }

    while (!temp_edge.empty()) {
        metaEdgeQueue.push_front(temp_edge.back());
        temp_edge.pop_back();
    }

    stats.numPropagatesHist.sample(num_propagates);

    assert(!nextPropagateEvent.scheduled());
    if (!metaEdgeQueue.empty()) {
        schedule(nextPropagateEvent, nextCycle());
    }
}

bool
PushEngine::enqueueUpdate(Addr src, Addr dst, uint32_t value)
{
    Addr aligned_dst = roundDown<Addr, size_t>(dst, owner->vertexAtomSize());
    AddrRange update_range(aligned_dst, aligned_dst + owner->vertexAtomSize());
    auto entry = portAddrMap.contains(update_range);
    PortID port_id = entry->second;

    DPRINTF(PushEngine, "%s: Update{src: %lu, dst:%lu, value: %u} "
                        "belongs to port %d.\n",
                        __func__, src, dst, value, port_id);
    DPRINTF(PushEngine, "%s: There are %d updates already "
                        "in queue for port %d.\n", __func__,
                        destinationQueues[port_id].size(), port_id);

    assert(destinationQueues[port_id].size() == sourceAndValueMaps[port_id].size());

    int num_updates = 0;
    for (auto queue: destinationQueues) {
        num_updates += queue.size();
    }

    if (sourceAndValueMaps[port_id].find(dst) != sourceAndValueMaps[port_id].end()) {
        DPRINTF(PushEngine, "%s: Found an existing update "
                            "for dst: %lu.\n", __func__, dst);
        Addr prev_src;
        uint32_t prev_val;
        std::tie(prev_src, prev_val) = sourceAndValueMaps[port_id][dst];
        uint32_t new_val = graphWorkload->reduce(value, prev_val);
        sourceAndValueMaps[port_id][dst] = std::make_tuple(prev_src, new_val);
        DPRINTF(PushEngine, "%s: Coalesced Update{src: %lu, dst:%lu, value: %u} "
                            "with Update{src: %lu, dst:%lu, value: %u} to"
                            "Update{src: %lu, dst:%lu, value: %u}.\n", __func__,
                            src, dst, value, prev_src, dst, prev_val,
                            prev_src, dst, new_val);
        stats.updateQueueCoalescions++;
        return true;
    } else if (num_updates < (updateQueueSize * destinationQueues.size())) {
        DPRINTF(PushEngine, "%s: There is a free entry available "
                            "in queue for port %d.\n", __func__, port_id);
        destinationQueues[port_id].emplace_back(dst, curTick());
        sourceAndValueMaps[port_id][dst] = std::make_tuple(src, value);
        DPRINTF(PushEngine, "%s: Emplaced Update{src: %lu, dst:%lu, value: %u} "
                            "at the back of queue for port %d. "
                            "Size of queue for port %d is %d.\n", __func__,
                            src, dst, value, port_id, port_id,
                            destinationQueues[port_id].size());
        stats.updateQueueLength.sample(destinationQueues[port_id].size());
        if (!nextUpdatePushEvent.scheduled()) {
            schedule(nextUpdatePushEvent, nextCycle());
        }
        return true;
    }
    DPRINTF(PushEngine, "%s: DestinationQueue for pot %d is blocked.\n",
                            __func__, port_id);
    return false;
}

template<typename T> PacketPtr
PushEngine::createUpdatePacket(Addr addr, T value)
{
    RequestPtr req = std::make_shared<Request>(addr, sizeof(T), 0, 0);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr) 1) << 2);

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
            DPRINTF(PushEngine, "%s: Port %d blocked.\n", __func__, i);
            continue;
        }
        DPRINTF(PushEngine, "%s: Port %d available.\n", __func__, i);
        if (destinationQueues[i].empty()) {
            DPRINTF(PushEngine, "%s: Respective queue for "
                                "port %d is empty.\n", __func__, i);
            continue;
        }
        Addr dst;
        Tick entrance_tick;
        std::tie(dst, entrance_tick) = destinationQueues[i].front();
        Addr src;
        uint32_t value;
        std::tie(src, value) = sourceAndValueMaps[i][dst];

        PacketPtr pkt = createUpdatePacket<uint32_t>(dst, value);
        outPorts[i].sendPacket(pkt);
        destinationQueues[i].pop_front();
        sourceAndValueMaps[i].erase(dst);
        DPRINTF(PushEngine, "%s: Sent Update{src: %lu, dst:%lu, value: %u} to "
                    "port %d. Respective queue size is %d.\n", __func__,
                    src, dst, value, i, destinationQueues[i].size());
        if (destinationQueues[i].size() > 0) {
            next_time_send += 1;
        }
        stats.numUpdates++;
    }

    assert(!nextUpdatePushEvent.scheduled());
    if (next_time_send > 0) {
        schedule(nextUpdatePushEvent, nextCycle());
    }
}

PushEngine::PushStats::PushStats(PushEngine& _push):
    statistics::Group(&_push), push(_push),
    ADD_STAT(numPropagates, statistics::units::Count::get(),
             "Number of propagate operations done."),
    ADD_STAT(updateQueueFull, statistics::units::Count::get(),
             "Number of times the update queue returns false."),
    ADD_STAT(numNetBlocks, statistics::units::Count::get(),
             "Number of updates blocked by network."),
    // ADD_STAT(numIdleCycles, statistics::units::Count::get(),
    //          "Number of cycles PushEngine has been idle."),
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
    ADD_STAT(edgePointerQueueLength, statistics::units::Count::get(),
             "Histogram of the size of the edgePointerQueue."),
    ADD_STAT(edgeQueueLatency, statistics::units::Second::get(),
             "Histogram of the latency of the metaEdgeQueue."),
    ADD_STAT(edgeQueueLength, statistics::units::Count::get(),
             "Histogram of the size of the metaEdgeQueue."),
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
    edgePointerQueueLength.init(64);
    edgeQueueLatency.init(64);
    edgeQueueLength.init(64);
    updateQueueLength.init(64);
    numPropagatesHist.init(1 + push.params().max_propagates_per_cycle);
}

} // namespace gem5
