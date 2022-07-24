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

#include "accl/graph/sega/coalesce_engine.hh"

#include "accl/graph/sega/wl_engine.hh"
#include "base/intmath.hh"
#include "debug/ApplyUpdates.hh"
#include "debug/CoalesceEngine.hh"
#include "mem/packet_access.hh"

namespace gem5
{

CoalesceEngine::CoalesceEngine(const CoalesceEngineParams &params):
    BaseMemEngine(params),
    peerPushEngine(params.peer_push_engine),
    numLines((int) (params.cache_size / peerMemoryAtomSize)),
    numElementsPerLine((int) (peerMemoryAtomSize / sizeof(WorkListItem))),
    numMSHREntries(params.num_mshr_entry),
    numTgtsPerMSHR(params.num_tgts_per_mshr),
    currentBitSliceIndex(0),
    numRetriesReceived(0),
    applyQueue(numLines),
    writeBackQueue(numLines),
    replaceQueue(numLines),
    nextMemoryReadEvent([this] { processNextMemoryReadEvent(); }, name()),
    nextRespondEvent([this] { processNextRespondEvent(); }, name()),
    nextApplyEvent([this] { processNextApplyEvent(); }, name()),
    nextWriteBackEvent([this] { processNextWriteBackEvent(); }, name()),
    nextSendRetryEvent([this] { processNextSendRetryEvent(); }, name()),
    stats(*this)
{
    assert(isPowerOf2(numLines) && isPowerOf2(numElementsPerLine));
    cacheBlocks = new Block [numLines];
    for (int i = 0; i < numLines; i++) {
        cacheBlocks[i] = Block(numElementsPerLine);
    }

    peerPushEngine->registerCoalesceEngine(this, numElementsPerLine);

    needsPush.reset();
}

void
CoalesceEngine::recvFunctional(PacketPtr pkt)
{
    sendMemFunctional(pkt);
}

void
CoalesceEngine::startup()
{
    AddrRangeList vertex_ranges = getAddrRanges();

    bool found = false;
    Addr first_match_addr = 0;
    while(true) {
        for (auto range: vertex_ranges) {
            if (range.contains(first_match_addr)) {
                found = true;
                break;
            }
        }
        if (found) {
            break;
        }
        first_match_addr += peerMemoryAtomSize;
    }

    found = false;
    Addr second_match_addr = first_match_addr + peerMemoryAtomSize;
    while(true) {
        for (auto range: vertex_ranges) {
            if (range.contains(second_match_addr)) {
                found = true;
                break;
            }
        }
        if (found) {
            break;
        }
        second_match_addr += peerMemoryAtomSize;
    }

    nmpu = (int) ((second_match_addr - first_match_addr) / peerMemoryAtomSize);
    memoryAddressOffset = first_match_addr;
}

void
CoalesceEngine::registerWLEngine(WLEngine* wl_engine)
{
    peerWLEngine = wl_engine;
}

// addr should be aligned to peerMemoryAtomSize
int
CoalesceEngine::getBlockIndex(Addr addr)
{
    assert((addr % peerMemoryAtomSize) == 0);
    return ((int) (addr / peerMemoryAtomSize)) % numLines;
}

// addr should be aligned to peerMemoryAtomSize
int
CoalesceEngine::getBitIndexBase(Addr addr)
{
    assert((addr % peerMemoryAtomSize) == 0);
    int atom_index = (int) (addr / (peerMemoryAtomSize * nmpu));
    int block_bits = (int) (peerMemoryAtomSize / sizeof(WorkListItem));
    int bit_index = atom_index * block_bits;
    return bit_index;
}

// index should be aligned to (peerMemoryAtomSize / sizeof(WorkListItem))
Addr
CoalesceEngine::getBlockAddrFromBitIndex(int index)
{
    assert((index % ((int) (peerMemoryAtomSize / sizeof(WorkListItem)))) == 0);
    Addr block_addr = (nmpu * peerMemoryAtomSize) *
        ((int)(index / (peerMemoryAtomSize / sizeof(WorkListItem))));
    return (block_addr + memoryAddressOffset);
}

bool
CoalesceEngine::recvWLRead(Addr addr)
{
    assert(MSHR.size() <= numMSHREntries);
    DPRINTF(CoalesceEngine,  "%s: Received a read request for address: %lu.\n",
                                                    __func__, addr);
    Addr aligned_addr = (addr / peerMemoryAtomSize) * peerMemoryAtomSize;
    assert(aligned_addr % peerMemoryAtomSize == 0);
    int block_index = (aligned_addr / peerMemoryAtomSize) % numLines;
    assert(block_index < numLines);
    int wl_offset = (addr - aligned_addr) / sizeof(WorkListItem);
    assert(wl_offset < numElementsPerLine);

    if ((cacheBlocks[block_index].addr == aligned_addr) &&
        (cacheBlocks[block_index].valid)) {
        // Hit
        // TODO: Add a hit latency as a param for this object.
        // Can't just schedule the nextRespondEvent for latency cycles in
        // the future.
        responseQueue.push_back(std::make_tuple(addr,
                    cacheBlocks[block_index].items[wl_offset]));
        DPRINTF(CoalesceEngine,  "%s: Addr: %lu is a hit. Pushed cacheBlocks[%d][%d]: %s "
            "to responseQueue. responseQueue.size = %d.\n",
            __func__, addr, block_index, wl_offset,
            cacheBlocks[block_index].items[wl_offset].to_string(),
            responseQueue.size());
        // TODO: Add a stat to count the number of WLItems that have been touched.
        cacheBlocks[block_index].busyMask |= (1 << wl_offset);
        stats.readHits++;

        if (!nextRespondEvent.scheduled()) {
            schedule(nextRespondEvent, nextCycle());
        }
        stats.numVertexReads++;
        return true;
    } else {
        // miss
        DPRINTF(CoalesceEngine,  "%s: Addr: %lu is a miss.\n", __func__, addr);
        if (MSHR.find(block_index) == MSHR.end()) {
            DPRINTF(CoalesceEngine,  "%s: Respective cacheBlocks[%d] for Addr: %lu not "
                        "found in MSHRs.\n", __func__, block_index, addr);
            assert(MSHR.size() <= numMSHREntries);
            if (MSHR.size() == numMSHREntries) {
                // Out of MSHR entries
                DPRINTF(CoalesceEngine,  "%s: Out of MSHR entries. "
                            "Rejecting request.\n", __func__);
                // TODO: Break out read rejections into more than one stat
                // based on the cause of the rejection
                stats.readRejections++;
                return false;
            } else {
                DPRINTF(CoalesceEngine,  "%s: MSHR entries available.\n", __func__);
                if (cacheBlocks[block_index].allocated) {
                    assert(MSHR[block_index].size() <= numTgtsPerMSHR);
                    DPRINTF(CoalesceEngine,  "%s: Addr: %lu has a conflict "
                                "with Addr: %lu.\n", __func__, addr,
                                cacheBlocks[block_index].addr);
                    if (MSHR[block_index].size() == numTgtsPerMSHR) {
                        DPRINTF(CoalesceEngine,  "%s: Out of targets for cacheBlocks[%d]. "
                                    "Rejecting request.\n",
                                    __func__, block_index);
                        stats.readRejections++;
                        return false;
                    }
                    cacheBlocks[block_index].hasConflict = true;
                    MSHR[block_index].push_back(addr);
                    DPRINTF(CoalesceEngine,  "%s: Added Addr: %lu to targets for cache "
                                "line[%d].\n", __func__, addr, block_index);
                    stats.readMisses++;
                    stats.numVertexReads++;

                    if ((cacheBlocks[block_index].busyMask == 0) &&
                        (cacheBlocks[block_index].valid)) {
                        applyQueue.push_back(block_index);
                        DPRINTF(CoalesceEngine,  "%s: Added %d to applyQueue. "
                                    "applyQueue.size = %u.\n", __func__,
                                    block_index, applyQueue.size());
                        assert(!applyQueue.empty());
                        if ((!nextApplyEvent.scheduled())) {
                            schedule(nextApplyEvent, nextCycle());
                        }
                    }
                    return true;
                } else {
                    assert(!cacheBlocks[block_index].valid);
                    // MSHR available and no conflict
                    DPRINTF(CoalesceEngine,  "%s: Addr: %lu has no conflict. "
                                            "Allocating a cache line for it.\n"
                                                            , __func__, addr);

                    cacheBlocks[block_index].addr = aligned_addr;
                    cacheBlocks[block_index].busyMask = 0;
                    cacheBlocks[block_index].allocated = true;
                    cacheBlocks[block_index].valid = false;
                    cacheBlocks[block_index].hasConflict = false;
                    DPRINTF(CoalesceEngine,  "%s: Allocated cacheBlocks[%d] for"
                                " Addr: %lu.\n", __func__, block_index, addr);

                    MSHR[block_index].push_back(addr);
                    DPRINTF(CoalesceEngine,  "%s: Added Addr: %lu to targets "
                        "for cacheBlocks[%d].\n", __func__, addr, block_index);

                    // enqueueMemReq(pkt);
                    fillQueue.push_back(block_index);
                    // FIXME: Fix this DPRINTF
                    // DPRINTF(CoalesceEngine,  "%s: Pushed pkt index  "
                    //         "lineFillBuffer. lineFillBuffer.size = %d.\n",
                    //         __func__, fillQueue.size());
                    if ((!nextMemoryReadEvent.pending()) &&
                        (!nextMemoryReadEvent.scheduled())) {
                        schedule(nextMemoryReadEvent, nextCycle());
                    }
                    stats.readMisses++;
                    stats.numVertexReads++;
                    return true;
                }
            }
        } else {
            DPRINTF(CoalesceEngine,  "%s: Respective cacheBlocks[%d] for Addr: %lu already "
                        "in MSHRs.\n", __func__, block_index, addr);
            if (MSHR[block_index].size() == numTgtsPerMSHR) {
                DPRINTF(CoalesceEngine,  "%s: Out of targets for cacheBlocks[%d]. "
                            "Rejecting request.\n",
                            __func__, block_index);
                stats.readRejections++;
                return false;
            }
            if ((!cacheBlocks[block_index].hasConflict) &&
                (aligned_addr != cacheBlocks[block_index].addr)) {
                DPRINTF(CoalesceEngine,  "%s: Addr: %lu has a conflict "
                            "with Addr: %lu.\n", __func__, addr,
                            cacheBlocks[block_index].addr);
                cacheBlocks[block_index].hasConflict = true;
            }

            if (aligned_addr != cacheBlocks[block_index].addr) {
                stats.readMisses++;
            } else {
                stats.readHitUnderMisses++;
            }

            MSHR[block_index].push_back(addr);
            DPRINTF(CoalesceEngine,  "%s: Added Addr: %lu to targets for cache "
                            "line[%d].\n", __func__, addr, block_index);
            stats.numVertexReads++;
            return true;
        }
    }
}

void
CoalesceEngine::processNextMemoryReadEvent()
{
    assert(!nextMemoryReadEvent.pending());
    if (memQueueFull()) {
        // TODO: Implement interface where events of the CoalesceEngine are
        // pushed to a fifo to be scheduled later.
        nextMemoryReadEvent.sleep();
        if (!pendingMemRetry()) {
            assert(pendingEventQueue.empty());
            requestMemRetry(1);
        }
        pendingEventQueue.push_back("nextMemoryReadEvent");
        return;
    }

    int block_index = fillQueue.front();
    PacketPtr pkt = createReadPacket(cacheBlocks[block_index].addr,
                                    peerMemoryAtomSize);
    DPRINTF(CoalesceEngine,  "%s: Created a read packet. addr = %lu, "
            "size = %d.\n", __func__, pkt->getAddr(), pkt->getSize());

    enqueueMemReq(pkt);

    fillQueue.pop_front();

    if (!fillQueue.empty()) {
        assert(!nextMemoryReadEvent.scheduled());
        assert(!nextMemoryReadEvent.pending());
        schedule(nextMemoryReadEvent, nextCycle());
    }
}

// TODO: For loop to empty the entire responseQueue.
void
CoalesceEngine::processNextRespondEvent()
{
    Addr addr_response;
    WorkListItem worklist_response;

    std::tie(addr_response, worklist_response) = responseQueue.front();
    peerWLEngine->handleIncomingWL(addr_response, worklist_response);
    DPRINTF(CoalesceEngine,  "%s: Sent WorkListItem: %s with Addr: %lu to WLEngine.\n",
                __func__, worklist_response.to_string(), addr_response);

    responseQueue.pop_front();
    DPRINTF(CoalesceEngine,  "%s: Popped a response from responseQueue. "
                "responseQueue.size = %d.\n", __func__,
                responseQueue.size());

    if ((!nextRespondEvent.scheduled()) &&
        (!responseQueue.empty())) {
        schedule(nextRespondEvent, nextCycle());
    }
}

// FIXME: Update this for implementing event retry interaction.
void
CoalesceEngine::recvMemRetry()
{
    assert(!pendingEventQueue.empty());
    std::string front = pendingEventQueue.front();

    if (front == "nextMemoryReadEvent") {
        assert(!nextMemoryReadEvent.scheduled());
        assert(nextMemoryReadEvent.pending());
        schedule(nextMemoryReadEvent, nextCycle());
        nextMemoryReadEvent.wake();
    } else if (front == "nextWriteBackEvent") {
        assert(!nextWriteBackEvent.scheduled());
        assert(nextWriteBackEvent.pending());
        schedule(nextWriteBackEvent, nextCycle());
        nextWriteBackEvent.wake();
    } else if (front == "nextSendRetryEvent") {
        assert(!nextSendRetryEvent.scheduled());
        assert(nextSendRetryEvent.pending());
        breakPointFunction();
        schedule(nextSendRetryEvent, nextCycle());
        nextSendRetryEvent.wake();
    } else {
        panic("EVENT IS NOT RECOGNIZED.\n");
    }

    pendingEventQueue.pop_front();
    if (!pendingEventQueue.empty()) {
        requestMemRetry(1);
    }
    return;
}

bool
CoalesceEngine::handleMemResp(PacketPtr pkt)
{
    assert(pkt->isResponse());
    if (pkt->isWrite()) {
        delete pkt;
        DPRINTF(CoalesceEngine,  "%s: Received a write response for Addr: %lu. Dropping "
                    "the packet.\n", __func__, pkt->getAddr());
        return true;
    }

    if (pkt->findNextSenderState<SenderState>()) {
        Addr addr = pkt->getAddr();
        int it = getBitIndexBase(addr);
        int block_index = getBlockIndex(addr);

        if ((cacheBlocks[block_index].addr == addr) &&
            (cacheBlocks[block_index].valid)) {
            // We read the address to send the wl but it is put in cache before
            // the read response arrives.
            if (cacheBlocks[block_index].busyMask == 0) {
                DPRINTF(CoalesceEngine, "%s: Received read response for retry "
                        "for addr %lu. It was found in the cache as idle.\n",
                        __func__, addr);
                int push_needed = 0;
                // It is not busy anymore, we have to send the wl from cache.
                DPRINTF(CoalesceEngine, "%s: needsPush.count: %d.\n",
                                __func__, needsPush.count());
                assert(peerPushEngine->getNumRetries() == needsPush.count());
                for (int i = 0; i < numElementsPerLine; i++) {
                    assert(!((needsPush[it + i] == 1) &&
                            (cacheBlocks[block_index].items[i].degree == 0)));
                    // TODO: Make this more programmable
                    uint32_t new_prop = std::min(
                                        cacheBlocks[block_index].items[i].prop,
                                        cacheBlocks[block_index].items[i].tempProp);
                    cacheBlocks[block_index].items[i].tempProp = new_prop;
                    cacheBlocks[block_index].items[i].prop = new_prop;
                    if (needsPush[it + i] == 1) {
                        peerPushEngine->recvWLItemRetry(
                            cacheBlocks[block_index].items[i]);
                    }
                    push_needed += needsPush[it + i];
                    needsPush[it + i] = 0;
                }
                DPRINTF(CoalesceEngine, "%s: needsPush.count: %d.\n",
                                __func__, needsPush.count());
                peerPushEngine->deallocatePushSpace(
                                        numElementsPerLine - push_needed);
                assert(peerPushEngine->getNumRetries() == needsPush.count());
                // Since we have just applied the line, we can take it out of
                // the applyQueue if it's in there. No need to do the same
                // thing for evictQueue.
                if (applyQueue.find(block_index)) {
                    applyQueue.erase(block_index);
                    if (applyQueue.empty() && nextApplyEvent.scheduled()) {
                        deschedule(nextApplyEvent);
                    }
                    if (cacheBlocks[block_index].hasConflict) {
                        writeBackQueue.push_back(block_index);
                        if ((!nextWriteBackEvent.pending()) &&
                            (!nextWriteBackEvent.scheduled())) {
                            schedule(nextWriteBackEvent, nextCycle());
                        }
                    }
                }
            } else {
                // The line is busy. Therefore, we have to disregard the data
                // we received from the memory and also tell the push engine to
                // deallocate the space it allocated for this retry. However,
                // we still have to rememeber that these items need a retry.
                // i.e. don't change needsPush, call recvWLItemRetry with
                // do_push = false
                DPRINTF(CoalesceEngine, "%s: Received read response for retry "
                        "for addr %lu. It was found in the cache as busy.\n",
                        __func__, addr);
                DPRINTF(CoalesceEngine, "%s: needsPush.count: %d.\n",
                                __func__, needsPush.count());
                assert(peerPushEngine->getNumRetries() == needsPush.count());
                peerPushEngine->deallocatePushSpace(numElementsPerLine);
                assert(peerPushEngine->getNumRetries() == needsPush.count());
                DPRINTF(CoalesceEngine, "%s: needsPush.count: %d.\n",
                                __func__, needsPush.count());
            }
        } else {
            // We have read the address to send the wl and it is not in the
            // cache. Simply send the items to the PushEngine.
            DPRINTF(CoalesceEngine, "%s: Received read response for retry "
                        "for addr %lu. It was not found in the cache.\n",
                        __func__, addr);
            WorkListItem* items = pkt->getPtr<WorkListItem>();
            int push_needed = 0;
            // No applying of the line needed.
            DPRINTF(CoalesceEngine, "%s: needsPush.count: %d.\n",
                                __func__, needsPush.count());
            assert(peerPushEngine->getNumRetries() == needsPush.count());
            for (int i = 0; i < numElementsPerLine; i++) {
                assert(!((needsPush[it + i] == 1) &&
                                (items[i].degree == 0)));
                if (needsPush[it + i] == 1) {
                    peerPushEngine->recvWLItemRetry(items[i]);
                }
                push_needed += needsPush[it + i];
                needsPush[it + i] = 0;
            }
            DPRINTF(CoalesceEngine, "%s: needsPush.count: %d.\n",
                                __func__, needsPush.count());
            peerPushEngine->deallocatePushSpace(
                                    numElementsPerLine - push_needed);
            assert(peerPushEngine->getNumRetries() == needsPush.count());
        }

        delete pkt;
        return true;
    }

    Addr addr = pkt->getAddr();
    int block_index = (addr / peerMemoryAtomSize) % numLines;

    DPRINTF(CoalesceEngine,  "%s: Received a read resposne for Addr: %lu.\n",
                __func__, pkt->getAddr());
    assert((cacheBlocks[block_index].allocated) && // allocated cache block
            (!cacheBlocks[block_index].valid) &&    // valid is false
            (!(MSHR.find(block_index) == MSHR.end()))); // allocated MSHR
    pkt->writeDataToBlock((uint8_t*) cacheBlocks[block_index].items,
                                                peerMemoryAtomSize);

    for (int i = 0; i < numElementsPerLine; i++) {
        DPRINTF(CoalesceEngine,  "%s: Wrote cacheBlocks[%d][%d] = %s.\n", __func__,
                block_index, i, cacheBlocks[block_index].items[i].to_string());
    }
    cacheBlocks[block_index].valid = true;
    delete pkt;

    // FIXME: Get rid of servicedIndices (maybe use an iterator)
    std::vector<int> servicedIndices;
    for (int i = 0; i < MSHR[block_index].size(); i++) {
        Addr miss_addr = MSHR[block_index][i];
        Addr aligned_miss_addr = roundDown<Addr, Addr>(miss_addr, peerMemoryAtomSize);
        if (aligned_miss_addr == addr) {
            int wl_offset = (miss_addr - aligned_miss_addr) / sizeof(WorkListItem);
            DPRINTF(CoalesceEngine,  "%s: Addr: %lu in the MSHR for cacheBlocks[%d] could "
                        "be serviced with the received packet.\n",
                        __func__, miss_addr, block_index);
            // TODO: Make this block of code into a function
            responseQueue.push_back(std::make_tuple(miss_addr,
                    cacheBlocks[block_index].items[wl_offset]));
            DPRINTF(CoalesceEngine,  "%s: Pushed cacheBlocks[%d][%d] to "
                    "responseQueue. responseQueue.size = %u.\n"
                    , __func__, block_index, wl_offset,
                    responseQueue.size());
            // TODO: Add a stat to count the number of WLItems that have been touched.
            cacheBlocks[block_index].busyMask |= (1 << wl_offset);
            // End of the said block

            servicedIndices.push_back(i);
            DPRINTF(CoalesceEngine,  "%s: Added index: %d of MSHR for cacheBlocks[%d] for "
                        "removal.\n", __func__, i, block_index);
        }
    }

    // TODO: We Can use taken instead of this
    // TODO: Change the MSHR from map<Addr, vector> to map<Addr, list>
    int bias = 0;
    for (int i = 0; i < servicedIndices.size(); i++) {
        Addr print_addr = MSHR[block_index][i - bias];
        MSHR[block_index].erase(MSHR[block_index].begin() +
                                    servicedIndices[i] - bias);
        bias++;
        DPRINTF(CoalesceEngine,  "%s: Addr: %lu has been serviced and is removed.\n",
                    __func__, print_addr);
    }

    if (MSHR[block_index].empty()) {
        MSHR.erase(block_index);
        cacheBlocks[block_index].hasConflict = false;
    } else {
        assert(cacheBlocks[block_index].hasConflict);
    }

    if ((!nextRespondEvent.scheduled()) &&
        (!responseQueue.empty())) {
        schedule(nextRespondEvent, nextCycle());
    }

    return true;
}

void
CoalesceEngine::recvWLWrite(Addr addr, WorkListItem wl)
{
    // TODO: Parameterize all the numbers here.
    Addr aligned_addr = roundDown<Addr, Addr>(addr, peerMemoryAtomSize);
    int block_index = (aligned_addr / peerMemoryAtomSize) % numLines;
    int wl_offset = (addr - aligned_addr) / sizeof(WorkListItem);

    DPRINTF(CoalesceEngine,  "%s: Received a write for WorkListItem: %s with Addr: %lu.\n",
                __func__, wl.to_string(), addr);
    assert((cacheBlocks[block_index].busyMask & (1 << wl_offset)) ==
            (1 << wl_offset));

    if (cacheBlocks[block_index].items[wl_offset].tempProp != wl.tempProp) {
        cacheBlocks[block_index].dirty = true;
        stats.numVertexWrites++;
    }

    cacheBlocks[block_index].items[wl_offset] = wl;
    cacheBlocks[block_index].busyMask &= ~(1 << wl_offset);
    DPRINTF(CoalesceEngine,  "%s: Wrote to cacheBlocks[%d][%d] = %s.\n",
                __func__, block_index, wl_offset,
                cacheBlocks[block_index].items[wl_offset].to_string());

    // TODO: Make this more general and programmable.
    if ((cacheBlocks[block_index].busyMask == 0)) {
        DPRINTF(CoalesceEngine,  "%s: Received all the expected writes for cacheBlocks[%d]."
                    " It does not have any taken items anymore.\n",
                    __func__, block_index);
        applyQueue.push_back(block_index);
        DPRINTF(CoalesceEngine,  "%s: Added %d to applyQueue. applyQueue.size = %u.\n",
                __func__, block_index, applyQueue.size());
    }

    if ((!applyQueue.empty()) &&
        (!nextApplyEvent.scheduled())) {
        schedule(nextApplyEvent, nextCycle());
    }

}

void
CoalesceEngine::processNextApplyEvent()
{
    int block_index = applyQueue.front();

    if (cacheBlocks[block_index].busyMask != 0) {
        DPRINTF(CoalesceEngine,  "%s: cacheBlocks[%d] has been taken amid apply process. "
                    "Therefore, ignoring the apply schedule.\n",
                    __func__, block_index);
        stats.falseApplySchedules++;
    } else if (!cacheBlocks[block_index].dirty) {
        DPRINTF(CoalesceEngine,  "%s: cacheBlocks[%d] has no change. Therefore, no apply "
                    "needed.\n", __func__, block_index);
    } else {
        for (int i = 0; i < numElementsPerLine; i++) {
            uint32_t old_prop = cacheBlocks[block_index].items[i].prop;
            uint32_t new_prop = std::min(
                                cacheBlocks[block_index].items[i].prop,
                                cacheBlocks[block_index].items[i].tempProp);

            if (new_prop != old_prop) {
                cacheBlocks[block_index].items[i].tempProp = new_prop;
                cacheBlocks[block_index].items[i].prop = new_prop;
                DPRINTF(ApplyUpdates, "%s: WorkListItem[%lu]: %s.\n", __func__,
                    cacheBlocks[block_index].addr + (i  * sizeof(WorkListItem)),
                    cacheBlocks[block_index].items[i].to_string());
                int bit_index =
                        getBitIndexBase(cacheBlocks[block_index].addr) + i;
                if ((cacheBlocks[block_index].items[i].degree != 0) &&
                    (needsPush[bit_index] == 0)) {
                    // If the respective bit in the bit vector is set
                    // there is no need to try and resend it.
                    if (peerPushEngine->allocatePushSpace()) {
                        peerPushEngine->recvWLItem(
                            cacheBlocks[block_index].items[i]);
                    } else {
                        needsPush[bit_index] = 1;
                    }
                }
            }
        }
    }

    // TODO: This is where eviction policy goes
    if (cacheBlocks[block_index].hasConflict){
        writeBackQueue.push_back(block_index);
        DPRINTF(CoalesceEngine,  "%s: Added %d to writeBackQueue. writeBackQueue.size = %u.\n",
                __func__, block_index, writeBackQueue.size());
    }

    applyQueue.pop_front();

    if ((!writeBackQueue.empty()) &&
        (!nextWriteBackEvent.pending()) &&
        (!nextWriteBackEvent.scheduled())) {
        schedule(nextWriteBackEvent, nextCycle());
    }

    if ((!applyQueue.empty()) &&
        (!nextApplyEvent.scheduled())) {
        schedule(nextApplyEvent, nextCycle());
    }
}

void
CoalesceEngine::processNextWriteBackEvent()
{
    assert(!nextWriteBackEvent.pending());
    if (memQueueFull()) {
        nextWriteBackEvent.sleep();
        // TODO: Implement interface where events of the CoalesceEngine are
        // pushed to a fifo to be scheduled later.
        if (!pendingMemRetry()) {
            assert(pendingEventQueue.empty());
            requestMemRetry(1);
        }
        pendingEventQueue.push_back("nextWriteBackEvent");
        return;
    }

    int block_index = writeBackQueue.front();

    // Why would we write it back if it does not have a conflict?
    assert(cacheBlocks[block_index].hasConflict);

    if ((cacheBlocks[block_index].busyMask != 0) ||
        (applyQueue.find(block_index))) {
        DPRINTF(CoalesceEngine,  "%s: cacheBlocks[%d] has been taken amid "
                "writeback process. Therefore, ignoring the apply schedule.\n",
                    __func__, block_index);
        // FIXME: Fix the name of this stat.
        stats.falseEvictSchedules++;
    } else {
        if (cacheBlocks[block_index].dirty) {
            DPRINTF(CoalesceEngine,  "%s: Change observed on "
                    "cacheBlocks[%d].\n", __func__, block_index);
            PacketPtr write_pkt = createWritePacket(
                cacheBlocks[block_index].addr, peerMemoryAtomSize,
                (uint8_t*) cacheBlocks[block_index].items);
            DPRINTF(CoalesceEngine,  "%s: Created a write packet to "
                        "Addr: %lu, size = %d.\n", __func__,
                        write_pkt->getAddr(), write_pkt->getSize());
            enqueueMemReq(write_pkt);
        }
        assert(!MSHR[block_index].empty());
        Addr miss_addr = MSHR[block_index].front();
        DPRINTF(CoalesceEngine,  "%s: First conflicting address for "
                                    "cacheBlocks[%d] is Addr: %lu.\n",
                                    __func__, block_index, miss_addr);
        Addr aligned_miss_addr =
            roundDown<Addr, Addr>(miss_addr, peerMemoryAtomSize);

        cacheBlocks[block_index].addr = aligned_miss_addr;
        cacheBlocks[block_index].busyMask = 0;
        cacheBlocks[block_index].allocated = true;
        cacheBlocks[block_index].valid = false;
        cacheBlocks[block_index].hasConflict = true;
        cacheBlocks[block_index].dirty = false;
        DPRINTF(CoalesceEngine,  "%s: Allocated cacheBlocks[%d] for "
                "Addr: %lu.\n", __func__, block_index, aligned_miss_addr);
        fillQueue.push_back(block_index);
    }

    writeBackQueue.pop_front();

    if (!writeBackQueue.empty()) {
        assert(!nextWriteBackEvent.pending());
        assert(!nextWriteBackEvent.scheduled());
        schedule(nextWriteBackEvent, nextCycle());
    }
}

void
CoalesceEngine::recvPushRetry()
{
    numRetriesReceived++;
    // For now since we do only one retry at a time, we should not receive
    // a retry while this nextSendingRetryEvent is scheduled or is pending.
    assert(!nextSendRetryEvent.pending());
    assert(!nextSendRetryEvent.scheduled());
    assert(numRetriesReceived == 1);
    schedule(nextSendRetryEvent, nextCycle());
}

void
CoalesceEngine::processNextSendRetryEvent()
{
    assert(!nextSendRetryEvent.pending());
    assert(needsPush.count() != 0);
    // if (needsPush.count() == 0) {
    //     DPRINTF(CoalesceEngine, "%s: Received a retry while there are no set "
    //                     "bit in needsPush. Rejecting the retry.\n", __func__);
    //     peerPushEngine->recvRetryReject();
    //     return;
    // }

    DPRINTF(CoalesceEngine,  "%s: Received a push retry.\n", __func__);
    Addr block_addr = 0;
    int block_index = 0;
    int it = 0;
    uint32_t slice = 0;
    bool hit_in_cache = false;

    for (it = currentBitSliceIndex; it < MAX_BITVECTOR_SIZE;
        it = (it + numElementsPerLine) % MAX_BITVECTOR_SIZE) {
        for (int i = 0; i < numElementsPerLine; i++) {
            slice <<= 1;
            slice |= needsPush[it + i];
        }
        if (slice) {
            block_addr = getBlockAddrFromBitIndex(it);
            block_index = getBlockIndex(block_addr);
            if ((cacheBlocks[block_index].addr == block_addr) &&
                (cacheBlocks[block_index].valid)) {
                if (cacheBlocks[block_index].busyMask == 0) {
                    hit_in_cache = true;
                    break;
                }
            } else {
                hit_in_cache = false;
                break;
            }
        }
    }

    assert(it < MAX_BITVECTOR_SIZE);
    if ((it + numElementsPerLine) > MAX_BITVECTOR_SIZE) {
        currentBitSliceIndex = 0;
    } else {
        currentBitSliceIndex = it + numElementsPerLine;
    }

    DPRINTF(CoalesceEngine, "%s: Found slice with value %d at position %d "
                        "in needsPush.\n", __func__, slice, it);

    if (hit_in_cache) {
        int push_needed = 0;
        DPRINTF(CoalesceEngine, "%s: needsPush.count: %d.\n",
                                __func__, needsPush.count());
        assert(peerPushEngine->getNumRetries() == needsPush.count());
        for (int i = 0; i < numElementsPerLine; i++) {
            // TODO: Make this more programmable
            uint32_t new_prop = std::min(
                                cacheBlocks[block_index].items[i].prop,
                                cacheBlocks[block_index].items[i].tempProp);
            cacheBlocks[block_index].items[i].tempProp = new_prop;
            cacheBlocks[block_index].items[i].prop = new_prop;
            if (needsPush[it + i] == 1) {
                peerPushEngine->recvWLItemRetry(
                    cacheBlocks[block_index].items[i]);
            }
            push_needed +=  needsPush[it + i];
            needsPush[it + i] = 0;
        }
        DPRINTF(CoalesceEngine, "%s: needsPush.count: %d.\n",
                                __func__, needsPush.count());
        peerPushEngine->deallocatePushSpace(numElementsPerLine - push_needed);
        assert(peerPushEngine->getNumRetries() == needsPush.count());
        if (applyQueue.find(block_index)) {
            applyQueue.erase(block_index);
            if (applyQueue.empty() && nextApplyEvent.scheduled()) {
                deschedule(nextApplyEvent);
            }
            if (cacheBlocks[block_index].hasConflict) {
                writeBackQueue.push_back(block_index);
                if ((!writeBackQueue.empty()) &&
                    (!nextWriteBackEvent.pending()) &&
                    (!nextWriteBackEvent.scheduled())) {
                    schedule(nextWriteBackEvent, nextCycle());
                }
            }
        }
    } else {
        if (memQueueFull()) {
            nextSendRetryEvent.sleep();
            if (!pendingMemRetry()) {
                assert(pendingEventQueue.empty());
                requestMemRetry(1);
            }
            pendingEventQueue.push_back("nextSendRetryEvent");
            return;
        }

        // FIXME: Fix the retry mechanism between memory and cache to
        // handle memory retries correctly. This probably requires scheduling
        // an event for sending the retry. For now we're enabling infinite
        // queueing in the outstandingMemReqQueue.
        // FIXME: Also do not send requests for cache lines that are already
        // read but await data. Just set a flag or sth.
        PacketPtr pkt = createReadPacket(block_addr, peerMemoryAtomSize);
        SenderState* sender_state = new SenderState(true);
        pkt->pushSenderState(sender_state);
        enqueueMemReq(pkt);
    }

    numRetriesReceived--;
    assert(numRetriesReceived == 0);
    assert(!nextSendRetryEvent.scheduled());
}

CoalesceEngine::CoalesceStats::CoalesceStats(CoalesceEngine &_coalesce)
    : statistics::Group(&_coalesce),
    coalesce(_coalesce),

    ADD_STAT(numVertexReads, statistics::units::Count::get(),
             "Number of memory vertecies read from cache."),
    ADD_STAT(numVertexWrites, statistics::units::Count::get(),
             "Number of memory vertecies written to cache."),
    ADD_STAT(readHits, statistics::units::Count::get(),
             "Number of cache hits."),
    ADD_STAT(readMisses, statistics::units::Count::get(),
             "Number of cache misses."),
    ADD_STAT(readHitUnderMisses, statistics::units::Count::get(),
             "Number of cache hit under misses."),
    ADD_STAT(readRejections, statistics::units::Count::get(),
             "Number of cache rejections."),
    ADD_STAT(falseApplySchedules, statistics::units::Count::get(),
             "Number of failed apply schedules."),
    ADD_STAT(falseEvictSchedules, statistics::units::Count::get(),
             "Number of failed evict schedules.")
{
}

void
CoalesceEngine::CoalesceStats::regStats()
{
    using namespace statistics;
}

}
