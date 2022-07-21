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

#include "accl/graph/sega/coalesce_engine.hh"
#include "debug/PushEngine.hh"
#include "mem/packet_access.hh"

namespace gem5
{

PushEngine::PushEngine(const PushEngineParams &params):
    BaseMemEngine(params),
    reqPort(name() + ".req_port", this),
    baseEdgeAddr(params.base_edge_addr),
    pushReqQueueSize(params.push_req_queue_size),
    numRetries(0), retrySpaceAllocated(0), spacesAllocatedBetweenRetries(0),
    nextAddrGenEvent([this] { processNextAddrGenEvent(); }, name()),
    nextPushEvent([this] { processNextPushEvent(); }, name()),
    stats(*this)
{}

Port&
PushEngine::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "req_port") {
        return reqPort;
    } else if (if_name == "mem_port") {
        return BaseMemEngine::getPort(if_name, idx);
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

void
PushEngine::registerCoalesceEngine(CoalesceEngine* coalesce_engine,
                                    int elements_per_line)
{
    peerCoalesceEngine = coalesce_engine;
    numElementsPerLine = elements_per_line;
}

void
PushEngine::ReqPort::sendPacket(PacketPtr pkt)
{
    panic_if(_blocked, "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt))
    {
        blockedPacket = pkt;
        _blocked = true;
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
    panic_if(!(_blocked && blockedPacket), "Received retry without a blockedPacket");

    DPRINTF(PushEngine, "%s: Received a reqRetry.\n", __func__);

    _blocked = false;
    sendPacket(blockedPacket);

    if (!_blocked) {
        blockedPacket = nullptr;
        DPRINTF(PushEngine, "%s: Sent the blockedPacket. "
                    "_blocked: %s, (blockedPacket == nullptr): %s.\n",
                    __func__, _blocked ? "true" : "false",
                    (blockedPacket == nullptr) ? "true" : "false");
    }
}

void
PushEngine::deallocatePushSpace(int space)
{
    retrySpaceAllocated -= space;
    DPRINTF(PushEngine, "%s: Deallocated %d spaces. numRetries = %d, "
            "nextAddrGenEvent.scheduled() = %s, pendingMemRetry() = %s, "
            "pushReqQueue.size() = %d, retrySpaceAllocated = %d.\n",
            __func__, space, numRetries,
            nextAddrGenEvent.scheduled() ? "true" : "false",
            pendingMemRetry() ? "true" : "false",
            pushReqQueue.size(), retrySpaceAllocated);
    /// DISCUSS: Might have to check whether the addrGenEvent is scheduled
    // and or the pushReqQueue is empty. If so we might need to
    // send retries.
    // if ((numRetries > 0)  &&
    //     ((pushReqQueue.size() + retrySpaceAllocated) == 0)) {
    //     assert((!pendingMemRetry()) && (!nextAddrGenEvent.scheduled()));
    //     int free_space =
    //         pushReqQueueSize - (pushReqQueue.size() + retrySpaceAllocated);
    //     if (free_space > numElementsPerLine) {
    //         DPRINTF(PushEngine, "%s: Found %d free spaces. "
    //                 "retrySpaceAllocated = %d.\n", __func__, free_space,
    //                 retrySpaceAllocated);
    //         retrySpaceAllocated += numElementsPerLine;
    //         peerCoalesceEngine->recvPushRetry();
    //     }
    // }

    if (numRetries > 0) {
        int free_space =
            pushReqQueueSize - (pushReqQueue.size() + retrySpaceAllocated);
        assert(free_space <= numElementsPerLine);
        retrySpaceAllocated += free_space;
        spacesAllocatedBetweenRetries += free_space;
        if (spacesAllocatedBetweenRetries >= numElementsPerLine) {
            spacesAllocatedBetweenRetries %= numElementsPerLine;
            peerCoalesceEngine->recvPushRetry();
        }
    }
}

void
PushEngine::recvWLItem(WorkListItem wl)
{
    assert(wl.degree != 0);

    assert((pushReqQueueSize == 0) ||
        (pushReqQueue.size() < pushReqQueueSize));
    panic_if((pushReqQueue.size() == pushReqQueueSize) &&
            (pushReqQueueSize != 0), "You should call this method after "
            "checking if there is enough push space. Use allocatePushSpace.\n");

    DPRINTF(PushEngine, "%s: Received %s.\n", __func__, wl.to_string());
    Addr start_addr = baseEdgeAddr + (wl.edgeIndex * sizeof(Edge));
    Addr end_addr = start_addr + (wl.degree * sizeof(Edge));
    uint32_t value = wl.prop;

    pushReqQueue.emplace_back(start_addr, end_addr, sizeof(Edge),
                                    peerMemoryAtomSize, value);

    if ((!nextAddrGenEvent.scheduled())) {
        if (memQueueFull()) {
            if (!pendingMemRetry()) {
                requestMemRetry(1);
            }
        } else {
            schedule(nextAddrGenEvent, nextCycle());
        }
    }
}

void
PushEngine::recvWLItemRetry(WorkListItem wl)
{
    assert(wl.degree != 0);
    DPRINTF(PushEngine, "%s: Received %s with retry.\n",
                                __func__, wl.to_string());

    Addr start_addr = baseEdgeAddr + (wl.edgeIndex * sizeof(Edge));
    Addr end_addr = start_addr + (wl.degree * sizeof(Edge));
    uint32_t value = wl.prop;

    pushReqQueue.emplace_back(start_addr, end_addr, sizeof(Edge),
                                    peerMemoryAtomSize, value);
    numRetries--;
    retrySpaceAllocated--;
    if ((!nextAddrGenEvent.scheduled())) {
        if (memQueueFull()) {
            if (!pendingMemRetry()) {
                requestMemRetry(1);
            }
        } else {
            schedule(nextAddrGenEvent, nextCycle());
        }
    }
}

void
PushEngine::processNextAddrGenEvent()
{
    Addr aligned_addr, offset;
    int num_edges;

    PushPacketInfoGen &curr_info = pushReqQueue.front();
    std::tie(aligned_addr, offset, num_edges) = curr_info.nextReadPacketInfo();
    DPRINTF(PushEngine, "%s: Current packet information generated by "
                "PushPacketInfoGen. aligned_addr: %lu, offset: %lu, "
                "num_edges: %d.\n", __func__, aligned_addr, offset, num_edges);

    PacketPtr pkt = createReadPacket(aligned_addr, peerMemoryAtomSize);
    reqOffsetMap[pkt->req] = offset;
    reqNumEdgeMap[pkt->req] = num_edges;
    reqValueMap[pkt->req] = curr_info.value();

    enqueueMemReq(pkt);

    if (curr_info.done()) {
        DPRINTF(PushEngine, "%s: Current PushPacketInfoGen is done.\n", __func__);
        pushReqQueue.pop_front();
        DPRINTF(PushEngine, "%s: Popped curr_info from pushReqQueue. "
                    "pushReqQueue.size() = %u.\n",
                    __func__, pushReqQueue.size());
        // if (numRetries > 0) {
        //     int free_space = pushReqQueueSize - (pushReqQueue.size() + retrySpaceAllocated);
        //     DPRINTF(PushEngine, "%s: Found %d free spaces in "
        //                     "the pushReqQueue.\n", __func__, free_space);
        //     if (free_space > numElementsPerLine) {
        //         retrySpaceAllocated += numElementsPerLine;
        //         DPRINTF(PushEngine, "%s: Allocated %d spaces for retry. "
        //                 "retrySpaceAllocated = %d.\n", __func__,
        //                 numElementsPerLine, retrySpaceAllocated);
        //         peerCoalesceEngine->recvPushRetry();
        //     }
        // }

        if (numRetries > 0) {
            retrySpaceAllocated++;
            DPRINTF(PushEngine, "%s: Allocated one space for retry. "
                "retrySpaceAllocated = %d.\n", __func__, retrySpaceAllocated);
            spacesAllocatedBetweenRetries++;
            if (spacesAllocatedBetweenRetries == numElementsPerLine) {
                spacesAllocatedBetweenRetries = 0;
                peerCoalesceEngine->recvPushRetry();
            }
        }
    }

    if (memQueueFull()) {
        if (!pushReqQueue.empty()) {
            requestMemRetry(1);
        }
        return;
    }

    if ((!nextAddrGenEvent.scheduled()) && (!pushReqQueue.empty())) {
        schedule(nextAddrGenEvent, nextCycle());
    }
}

void
PushEngine::recvMemRetry()
{
    assert(!nextAddrGenEvent.scheduled());
    DPRINTF(PushEngine, "%s: Received a memory retry.\n", __func__);
    schedule(nextAddrGenEvent, nextCycle());
}

bool
PushEngine::handleMemResp(PacketPtr pkt)
{
    // TODO: in case we need to edit edges, get rid of second statement.
    assert(pkt->isResponse() && (!pkt->isWrite()));
    memRespQueue.push_back(pkt);

    if ((!nextPushEvent.scheduled()) && (!memRespQueue.empty())) {
        schedule(nextPushEvent, nextCycle());
    }
    return true;
}

// TODO: Add a parameter to allow for doing multiple pushes at the same time.
void
PushEngine::processNextPushEvent()
{
    PacketPtr pkt = memRespQueue.front();
    uint8_t* data = pkt->getPtr<uint8_t>();

    Addr offset = reqOffsetMap[pkt->req];
    assert(offset < peerMemoryAtomSize);
    uint32_t value = reqValueMap[pkt->req];

    DPRINTF(PushEngine, "%s: Looking at the front of the queue. pkt->Addr: %lu, "
                "offset: %lu\n",
            __func__, pkt->getAddr(), offset);

    Edge* curr_edge = (Edge*) (data + offset);

    // TODO: Implement propagate function here
    uint32_t update_value = value + 1;
    PacketPtr update = createUpdatePacket<uint32_t>(
                            curr_edge->neighbor, update_value);

    if (!reqPort.blocked()) {
        reqPort.sendPacket(update);
        stats.numUpdates++;
        DPRINTF(PushEngine, "%s: Sent a push update to addr: %lu with value: %d.\n",
                                __func__, curr_edge->neighbor, update_value);
        reqOffsetMap[pkt->req] = reqOffsetMap[pkt->req] + sizeof(Edge);
        assert(reqOffsetMap[pkt->req] <= peerMemoryAtomSize);
        reqNumEdgeMap[pkt->req]--;
        assert(reqNumEdgeMap[pkt->req] >= 0);
    }

    if (reqNumEdgeMap[pkt->req] == 0) {
        reqOffsetMap.erase(pkt->req);
        reqNumEdgeMap.erase(pkt->req);
        reqValueMap.erase(pkt->req);
        memRespQueue.pop_front();
        delete pkt;
    }

    if (!nextPushEvent.scheduled() && !memRespQueue.empty()) {
        schedule(nextPushEvent, nextCycle());
    }
}

template<typename T> PacketPtr
PushEngine::createUpdatePacket(Addr addr, T value)
{
    RequestPtr req = std::make_shared<Request>(
                addr, sizeof(T), 0, _requestorId);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr) _requestorId) << 2);

    // FIXME: MemCmd::UpdateWL
    PacketPtr pkt = new Packet(req, MemCmd::UpdateWL);

    pkt->allocate();
    // pkt->setData(data);
    pkt->setLE<T>(value);

    return pkt;
}

bool
PushEngine::allocatePushSpace() {
    assert(retrySpaceAllocated >= 0);
    if ((pushReqQueueSize == 0) ||
        ((pushReqQueue.size() + retrySpaceAllocated) < pushReqQueueSize)) {
        assert(numRetries == 0);
        return true;
    } else {
        numRetries++;
        return false;
    }
}

PushEngine::PushStats::PushStats(PushEngine &_push)
    : statistics::Group(&_push),
    push(_push),
    ADD_STAT(numUpdates, statistics::units::Count::get(),
             "Number of sent updates.")
{
}

void
PushEngine::PushStats::regStats()
{
    using namespace statistics;
}

}
