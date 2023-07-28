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
    _running(false),
    lastIdleEntranceTick(0),
    numPendingPulls(0), edgePointerQueueSize(params.push_req_queue_size),
    onTheFlyMemReqs(0), edgeQueueSize(params.resp_queue_size),
    maxPropagatesPerCycle(params.max_propagates_per_cycle),
    workload(params.workload),
    nextVertexPullEvent([this] { processNextVertexPullEvent(); }, name()),
    nextMemoryReadEvent([this] { processNextMemoryReadEvent(); }, name()),
    nextPropagateEvent([this] { processNextPropagateEvent(); }, name()),
    stats(*this)
{}

void
PushEngine::registerMPU(MPU* mpu)
{
    owner = mpu;
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
    return edgeQueue.empty() &&
            (onTheFlyMemReqs == 0) &&
            edgePointerQueue.empty();
}


uint32_t
PushEngine::propagate(uint32_t value, uint32_t weight)
{
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
        schedule(nextVertexPullEvent, nextCycle());
    }
}

void
PushEngine::processNextVertexPullEvent()
{
    // TODO: change edgePointerQueueSize
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

    if (edgeQueue.size() < (edgeQueueSize - onTheFlyMemReqs)) {
        Addr aligned_addr, offset;
        int num_edges;

        EdgeReadInfoGen &curr_info = edgePointerQueue.front();
        std::tie(aligned_addr, offset, num_edges) = curr_info.nextReadPacketInfo();
        DPRINTF(PushEngine, "%s: Current packet information generated by "
                    "EdgeReadInfoGen. aligned_addr: %lu, offset: %lu, "
                    "num_edges: %d.\n", __func__, aligned_addr, offset, num_edges);

        PacketPtr pkt = createReadPacket(aligned_addr, peerMemoryAtomSize);
        PushInfo push_info = {curr_info.src(), curr_info.value(), offset, num_edges};
        reqInfoMap[pkt->req] = push_info;

        memPort.sendPacket(pkt);
        onTheFlyMemReqs++;

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

    std::deque<std::tuple<MetaEdge, Tick>> edges;
    for (int i = 0; i < push_info.numElements; i++) {
        Edge* edge = (Edge*) (pkt_data + push_info.offset + i * sizeof(Edge));
        Addr edge_dst = edge->neighbor;
        uint32_t edge_weight = edge->weight;
        MetaEdge meta_edge(
                    push_info.src, edge_dst, edge_weight, push_info.value);
        edges.emplace_back(meta_edge, curTick());
    }
    assert(!edges.empty());
    edgeQueue.push_back(edges);

    onTheFlyMemReqs--;
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
        std::deque<std::tuple<MetaEdge, Tick>>& edge_list = edgeQueue.front();
        MetaEdge meta_edge;
        Tick entrance_tick;
        std::tie(meta_edge, entrance_tick) = edge_list.front();

        DPRINTF(PushEngine, "%s: The edge to process is %s.\n",
                                __func__, meta_edge.to_string());

        uint32_t update_value = propagate(meta_edge.value, meta_edge.weight);
        Update update(meta_edge.src, meta_edge.dst, update_value);
        edge_list.pop_front();

        if (owner->enqueueUpdate(update)) {
            DPRINTF(PushEngine, "%s: Sending %s to port queues.\n",
                                            __func__, meta_edge.to_string());
            stats.numUpdates++;
            stats.edgeQueueLatency.sample(
                    (curTick() - entrance_tick) * 1e9 / getClockFrequency());
        } else {
            edge_list.emplace_back(meta_edge, entrance_tick);
        }

        if (edge_list.empty()) {
            edgeQueue.pop_front();
        }

        if (edgeQueue.empty()) {
            break;
        }

        num_propagates++;
        if (num_propagates >= maxPropagatesPerCycle) {
            break;
        }
    }

    assert(!nextPropagateEvent.scheduled());
    if (!edgeQueue.empty()) {
        schedule(nextPropagateEvent, nextCycle());
    }
}

PushEngine::PushStats::PushStats(PushEngine &_push)
    : statistics::Group(&_push),
    push(_push),
    ADD_STAT(numUpdates, statistics::units::Count::get(),
             "Number of sent updates."),
    ADD_STAT(numNetBlocks, statistics::units::Count::get(),
             "Number of updates blocked by network."),
    ADD_STAT(numIdleCycles, statistics::units::Count::get(),
             "Number of cycles PushEngine has been idle."),
    ADD_STAT(TEPS, statistics::units::Rate<statistics::units::Count,
                                    statistics::units::Second>::get(),
             "Traversed Edges Per Second."),
    ADD_STAT(edgePointerQueueLatency, statistics::units::Second::get(),
             "Histogram of the latency of the edgePointerQueue."),
    ADD_STAT(edgeQueueLatency, statistics::units::Second::get(),
             "Histogram of the latency of the edgeQueue.")
{
}

void
PushEngine::PushStats::regStats()
{
    using namespace statistics;

    TEPS = numUpdates / simSeconds;

    edgePointerQueueLatency.init(64);
    edgeQueueLatency.init(64);
}

} // namespace gem5
