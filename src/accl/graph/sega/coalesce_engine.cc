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
#include "debug/MPU.hh"
#include "mem/packet_access.hh"

namespace gem5
{

CoalesceEngine::CoalesceEngine(const CoalesceEngineParams &params):
    BaseMemEngine(params),
    peerPushEngine(params.peer_push_engine),
    numLines((int) (params.cache_size / peerMemoryAtomSize)),
    numElementsPerLine((int) (peerMemoryAtomSize / sizeof(WorkListItem))),
    numMSHREntry(params.num_mshr_entry),
    numTgtsPerMSHR(params.num_tgts_per_mshr),
    nextRespondEvent([this] { processNextRespondEvent(); }, name()),
    nextApplyAndCommitEvent([this] { processNextApplyAndCommitEvent(); }, name()),
    stats(*this)
{
    assert(isPowerOf2(numLines) && isPowerOf2(numElementsPerLine));
    cacheBlocks = new Block [numLines];
    for (int i = 0; i < numLines; i++) {
        cacheBlocks[i] = Block(numElementsPerLine);
    }
}

void
CoalesceEngine::recvFunctional(PacketPtr pkt)
{
    sendMemFunctional(pkt);
}

void
CoalesceEngine::registerWLEngine(WLEngine* wl_engine)
{
    peerWLEngine = wl_engine;
}

bool
CoalesceEngine::recvReadAddr(Addr addr)
{
    assert(MSHRMap.size() <= numMSHREntry);
    DPRINTF(MPU, "%s: Received a read request for address: %lu.\n",
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
        DPRINTF(MPU, "%s: Addr: %lu is a hit. Pushed cacheBlocks[%d][%d]: %s "
            "to responseQueue. responseQueue.size = %d.\n",
            __func__, addr, block_index, wl_offset, responseQueue.size(),
            cacheBlocks[block_index].items[wl_offset].to_string());
        // TODO: Add a stat to count the number of WLItems that have been touched.
        cacheBlocks[block_index].takenMask |= (1 << wl_offset);
        stats.readHits++;

        assert(!responseQueue.empty());
        if (!nextRespondEvent.scheduled()) {
            schedule(nextRespondEvent, nextCycle());
        }
        stats.numVertexReads++;
        return true;
    } else {
        // miss
        DPRINTF(MPU, "%s: Addr: %lu is a miss.\n", __func__, addr);
        if (MSHRMap.find(block_index) == MSHRMap.end()) {
            DPRINTF(MPU, "%s: Respective cache line[%d] for Addr: %lu not "
                        "found in MSHRs.\n", __func__, block_index, addr);
            assert(MSHRMap.size() <= numMSHREntry);
            if (MSHRMap.size() == numMSHREntry) {
                // Out of MSHR entries
                DPRINTF(MPU, "%s: Out of MSHR entries. "
                            "Rejecting request.\n", __func__);
                // TODO: Break out read rejections into more than one stat
                // based on the cause of the rejection
                stats.readRejections++;
                return false;
            } else {
                DPRINTF(MPU, "%s: MSHR entries available.\n", __func__);
                if (cacheBlocks[block_index].allocated) {
                    assert(MSHRMap[block_index].size() <= numTgtsPerMSHR);
                    DPRINTF(MPU, "%s: Addr: %lu has a conflict "
                                "with Addr: %lu.\n", __func__, addr,
                                cacheBlocks[block_index].addr);
                    if (MSHRMap[block_index].size() == numTgtsPerMSHR) {
                        DPRINTF(MPU, "%s: Out of targets for cache line[%d]. "
                                    "Rejecting request.\n",
                                    __func__, block_index);
                        stats.readRejections++;
                        return false;
                    }
                    cacheBlocks[block_index].hasConflict = true;
                    MSHRMap[block_index].push_back(addr);
                    DPRINTF(MPU, "%s: Added Addr: %lu to targets for cache "
                                "line[%d]", __func__, addr, block_index);
                    stats.readMisses++;
                    stats.numVertexReads++;
                    return true;
                } else {
                    assert(!cacheBlocks[block_index].valid);
                    // MSHR available and no conflict
                    //TODO: Fix this to work with new inheritance.
                    // assert(
                    //     outstandingMemReqQueue.size() <=
                    //     outstandingMemReqQueueSize);
                    DPRINTF(MPU, "%s: Addr: %lu has no conflict. Trying to "
                                "allocate a cache line for it.\n",
                                __func__, addr);
                    if (memReqQueueFull()) {
                        DPRINTF(MPU, "%s: No space in outstandingMemReqQueue. "
                                    "Rejecting  request.\n", __func__);
                        stats.readRejections++;
                        return false;
                    }
                    cacheBlocks[block_index].addr = aligned_addr;
                    cacheBlocks[block_index].takenMask = 0;
                    cacheBlocks[block_index].allocated = true;
                    cacheBlocks[block_index].valid = false;
                    cacheBlocks[block_index].hasConflict = false;
                    DPRINTF(MPU, "%s: Allocated cache line[%d] for "
                                "Addr: %lu.\n", __func__, block_index, addr);

                    MSHRMap[block_index].push_back(addr);
                    DPRINTF(MPU, "%s: Added Addr: %lu to targets for cache "
                                "line[%d].\n", __func__, addr, block_index);

                    PacketPtr pkt = createReadPacket(aligned_addr, peerMemoryAtomSize);
                    DPRINTF(MPU, "%s: Created a read packet for Addr: %lu."
                                " req addr (aligned_addr) = %lu, size = %d.\n",
                                __func__, addr, aligned_addr, peerMemoryAtomSize);
                    enqueueMemReq(pkt);
                    DPRINTF(MPU, "%s: Pushed pkt to outstandingMemReqQueue.\n",
                                                                    __func__);
                    stats.readMisses++;
                    stats.numVertexReads++;
                    return true;
                }
            }
        } else {
            DPRINTF(MPU, "%s: Respective cache line[%d] for Addr: %lu already "
                        "in MSHRs.\n", __func__, block_index, addr);
            if (MSHRMap[block_index].size() == numTgtsPerMSHR) {
                DPRINTF(MPU, "%s: Out of targets for cache line[%d]. "
                            "Rejecting request.\n",
                            __func__, block_index);
                stats.readRejections++;
                return false;
            }
            if ((!cacheBlocks[block_index].hasConflict) &&
                (aligned_addr != cacheBlocks[block_index].addr)) {
                DPRINTF(MPU, "%s: Addr: %lu has a conflict "
                            "with Addr: %lu.\n", __func__, addr,
                            cacheBlocks[block_index].addr);
                cacheBlocks[block_index].hasConflict = true;
            }

            if (aligned_addr != cacheBlocks[block_index].addr) {
                stats.readMisses++;
            } else {
                stats.readHitUnderMisses++;
            }

            MSHRMap[block_index].push_back(addr);
            DPRINTF(MPU, "%s: Added Addr: %lu to targets for cache "
                            "line[%d].\n", __func__, addr, block_index);
            stats.numVertexReads++;
            return true;
        }
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
    DPRINTF(MPU, "%s: Sent WorkListItem: %s with Addr: %lu to WLEngine.\n",
                __func__, worklist_response.to_string(), addr_response);

    responseQueue.pop_front();
    DPRINTF(MPU, "%s: Popped a response from responseQueue. "
                "responseQueue.size = %d.\n", __func__,
                responseQueue.size());

    if ((!nextRespondEvent.scheduled()) &&
        (!responseQueue.empty())) {
        schedule(nextRespondEvent, nextCycle());
    }
}

void
CoalesceEngine::respondToAlarm()
{
    assert(!nextApplyAndCommitEvent.scheduled());
    schedule(nextApplyAndCommitEvent, nextCycle());
}

bool
CoalesceEngine::handleMemResp(PacketPtr pkt)
{
    assert(pkt->isResponse());
    if (pkt->isWrite()) {
        delete pkt;
        DPRINTF(MPU, "%s: Received a write response for Addr: %lu. Dropping "
                    "the packet.\n", __func__, pkt->getAddr());
        return true;
    }

    Addr addr = pkt->getAddr();
    int block_index = (addr / peerMemoryAtomSize) % numLines;

    DPRINTF(MPU, "%s: Received a read resposne for Addr: %lu.\n",
                __func__, pkt->getAddr());
    assert((cacheBlocks[block_index].allocated) && // allocated cache block
            (!cacheBlocks[block_index].valid) &&    // valid is false
            (!(MSHRMap.find(block_index) == MSHRMap.end()))); // allocated MSHR
    pkt->writeDataToBlock((uint8_t*) cacheBlocks[block_index].items,
                                                peerMemoryAtomSize);

    for (int i = 0; i < numElementsPerLine; i++) {
        DPRINTF(MPU, "%s: Wrote cacheBlocks[%d][%d] = %s.\n", __func__,
                block_index, i, cacheBlocks[block_index].items[i].to_string());
    }
    cacheBlocks[block_index].valid = true;
    delete pkt;

    // FIXME: Get rid of servicedIndices (maybe use an iterator)
    std::vector<int> servicedIndices;
    for (int i = 0; i < MSHRMap[block_index].size(); i++) {
        Addr miss_addr = MSHRMap[block_index][i];
        Addr aligned_miss_addr = std::floor(miss_addr / peerMemoryAtomSize) * peerMemoryAtomSize;

        if (aligned_miss_addr == addr) {
            int wl_offset = (miss_addr - aligned_miss_addr) / sizeof(WorkListItem);
            DPRINTF(MPU, "%s: Addr: %lu in the MSHR for cache line[%d] could "
                        "be serviced with the received packet.\n",
                        __func__, miss_addr, block_index);
            // TODO: Make this block of code into a function
            responseQueue.push_back(std::make_tuple(miss_addr,
                    cacheBlocks[block_index].items[wl_offset]));
            DPRINTF(MPU, "%s: Pushed cache line[%d][%d] to "
                    "responseQueue. responseQueue.size = %u.\n"
                    , __func__, block_index, wl_offset,
                    responseQueue.size());
            // TODO: Add a stat to count the number of WLItems that have been touched.
            cacheBlocks[block_index].takenMask |= (1 << wl_offset);
            // End of the said block

            servicedIndices.push_back(i);
            DPRINTF(MPU, "%s: Added index: %d of MSHR for cache line[%d] for "
                        "removal.\n", __func__, i, block_index);
        }
    }

    // TODO: We Can use taken instead of this
    // TODO: Change the MSHRMap from map<Addr, vector> to map<Addr, list>
    int bias = 0;
    for (int i = 0; i < servicedIndices.size(); i++) {
        Addr print_addr = MSHRMap[block_index][i - bias];
        MSHRMap[block_index].erase(MSHRMap[block_index].begin() +
                                    servicedIndices[i] - bias);
        bias++;
        DPRINTF(MPU, "%s: Addr: %lu has been serviced and is removed.\n",
                    __func__, print_addr);
    }

    if (MSHRMap[block_index].empty()) {
        MSHRMap.erase(block_index);
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
    Addr aligned_addr = std::floor(addr / peerMemoryAtomSize) * peerMemoryAtomSize;
    int block_index = (aligned_addr / peerMemoryAtomSize) % numLines;
    int wl_offset = (addr - aligned_addr) / sizeof(WorkListItem);

    DPRINTF(MPU, "%s: Received a write for WorkListItem: %s with Addr: %lu.\n",
                __func__, wl.to_string(), addr);
    assert((cacheBlocks[block_index].takenMask & (1 << wl_offset)) ==
            (1 << wl_offset));

    if (cacheBlocks[block_index].items[wl_offset].tempProp != wl.tempProp) {
        cacheBlocks[block_index].hasChange = true;
        stats.numVertexWrites++;
    }

    cacheBlocks[block_index].items[wl_offset] = wl;
    cacheBlocks[block_index].takenMask &= ~(1 << wl_offset);
    DPRINTF(MPU, "%s: Wrote to cache line[%d] = %s.\n", __func__, block_index,
                cacheBlocks[block_index].items[wl_offset].to_string());

    // TODO: Make this more general and programmable.
    if ((cacheBlocks[block_index].takenMask == 0)) {
        DPRINTF(MPU, "%s: Received all the expected writes for cache line[%d]."
                    " It does not have any taken items anymore.\n",
                    __func__, block_index);
        // TODO: Fix this hack
        bool found = false;
        for (auto i : evictQueue) {
            if (i == block_index) {
                found = true;
                break;
            }
        }
        if (!found) {
            evictQueue.push_back(block_index);
        }
        DPRINTF(MPU, "%s: Added %d to evictQueue. evictQueue.size = %u.\n",
                    __func__, block_index, evictQueue.size());
    }

    if ((!nextApplyAndCommitEvent.scheduled()) &&
        (!evictQueue.empty()) &&
        (!pendingAlarm())) {
        schedule(nextApplyAndCommitEvent, nextCycle());
    }

}

void
CoalesceEngine::processNextApplyEvent()
{
    int block_index = applyQueue.front();

    if (cacheBlocks[block_index].takenMask) {
        DPRINTF(MPU, "%s: cache line [%d] has been taken amid apply process. "
                    "Therefore, ignoring the apply schedule.\n",
                    __func__, block_index);
        stats.falseApplySchedules++;
    } else if (!cacheBlocks[block_index].hasChange) {
        DPRINTF(MPU, "%s: cache line [%d] has no change. Therefore, no apply "
                    "needed. Adding the cache line to evict schedule.\n",
                    __func__, block_index);
        evictQueue.push_back(block_index);
    } else {
        for (int i = 0; i < numElementsPerLine; i++) {
            uint32_t old_prop = cacheBlocks[block_index].items[i].prop;
            cacheBlocks[block_index].items[i].prop = std::min(
                                cacheBlocks[block_index].items[i].prop,
                                cacheBlocks[block_index].items[i].tempProp);
            // TODO: Is this correct?
            cacheBlocks[block_index].items[i].tempProp = cacheBlocks[block_index].items[i].prop;

            if (cacheBlocks[block_index].items[i].prop != old_prop) {
                if (peerPushEngine->recvWLItem(
                    cacheBlocks[block_index].items[i])) {
                    DPRINTF(MPU, "%s: Sent WorkListItem [%d] to PushEngine.\n",
                    __func__,
                    cacheBlocks[block_index].addr + i * sizeof(WorkListItem));
                } else {
                    // peerPushEngine->setPushAlarm();
                    // pendingPushAlarm = true;
                    return;
                }
            }
        }
        // TODO: This is where eviction policy goes
        evictQueue.push_back(block_index);
    }

    applyQueue.pop_front();

    if ((!evictQueue.empty()) &&
        (!pendingAlarm()) &&
        (!nextEvictEvent.scheduled())) {
        schedule(nextEvictEvent, nextCycle());
    }

    if ((!applyQueue.empty()) &&
        (!nextApplyEvent.scheduled())) {
        schedule(nextApplyEvent, nextCycle());
    }
}

void
CoalesceEngine::processNextEvictEvent()
{
    int block_index = evictQueue.front();

    if (cacheBlocks[block_index].takenMask) {
        DPRINTF(MPU, "%s: cache line [%d] has been taken amid evict process. "
                    "Therefore, ignoring the apply schedule.\n",
                    __func__, block_index);
        stats.falseEvictSchedules++;
    } else {
        int space_needed = cacheBlocks
    }
}

void
CoalesceEngine::processNextApplyAndCommitEvent()
{
    // FIXME: Refactor the line below to work with the new inheritance.
    // assert((!alarmRequested) && (spaceRequested == 0));
    int block_index = evictQueue.front();
    uint8_t changedMask = 0;

    DPRINTF(MPU, "%s: Received nextApplyAndCommitEvent for cache line[%d].\n",
                __func__, block_index);
    DPRINTF(MPU, "%s: Checking to see if cache line[%d] could be applied and "
                "then commited.\n", __func__, block_index);

    if (cacheBlocks[block_index].takenMask == 0) {
        if ((cacheBlocks[block_index].hasChange) &&
            (cacheBlocks[block_index].hasConflict) &&
            (memReqQueueHasSpace(2))) {
            DPRINTF(MPU, "%s: ApplyAndCommit could be done on cache line[%d].\n",
                        __func__, block_index);
        } else if ((cacheBlocks[block_index].hasChange) &&
                    (!cacheBlocks[block_index].hasConflict) &&
                    (memReqQueueHasSpace(1))) {
            DPRINTF(MPU, "%s: ApplyAndCommit could be done on cache line[%d].\n",
                        __func__, block_index);
        } else if ((!cacheBlocks[block_index].hasChange) &&
                    (cacheBlocks[block_index].hasConflict) &&
                    (memReqQueueHasSpace(1))) {
            DPRINTF(MPU, "%s: ApplyAndCommit could be done on cache line[%d].\n",
                        __func__, block_index);
        } else if ((!cacheBlocks[block_index].hasChange) &&
                    (!cacheBlocks[block_index].hasConflict)) {
            DPRINTF(MPU, "%s: No ApplyAndCommit needed for cache line[%d].\n",
                        __func__, block_index);
        } else {
            int spaceNeeded = cacheBlocks[block_index].hasConflict ? 2 : 1;
            requestAlarm(spaceNeeded);
            DPRINTF(MPU, "%s: Not enough space in outstandingMemReqQueue. Set "
            "an alarm for nextApplyAndCommitEvent when there is %d space.\n",
            __func__, spaceNeeded);
            return;
        }

        // Reducing between tempProp and prop for each item in the cache line.
        for (int i = 0; i < numElementsPerLine; i++) {
            uint32_t old_prop = cacheBlocks[block_index].items[i].prop;
            cacheBlocks[block_index].items[i].prop = std::min(
                cacheBlocks[block_index].items[i].prop,
                cacheBlocks[block_index].items[i].tempProp);
            DPRINTF(MPU, "%s: Applied cache line[%d][%d] = %s.\n", __func__,
                        block_index, i,
                        cacheBlocks[block_index].items[i].to_string());
            if (old_prop != cacheBlocks[block_index].items[i].prop) {
                changedMask |= (1 << i);
                // TODO: Add a stat to count the number of changed props.
                DPRINTF(MPU, "%s: Change observed in cache line[%d][%d].\n",
                            __func__, block_index, i);
            }
        }

        if (cacheBlocks[block_index].hasChange) {
            DPRINTF(MPU, "%s: At least one item from cache line[%d] has changed.\n"
                        , __func__, block_index);

            PacketPtr write_pkt = createWritePacket(
                cacheBlocks[block_index].addr, peerMemoryAtomSize,
                (uint8_t*) cacheBlocks[block_index].items);
            DPRINTF(MPU, "%s: Created a write packet to Addr: %lu, size = %d.\n",
                        __func__, write_pkt->getAddr(), peerMemoryAtomSize);
            enqueueMemReq(write_pkt);
            DPRINTF(MPU, "%s: Added the evicting write back packet to "
                        "outstandingMemReqQueue.\n" , __func__);

            for (int i = 0; i < numElementsPerLine; i++) {
                if ((changedMask & (1 << i)) == (1 << i)) {
                    DPRINTF(ApplyUpdates, "%s: WorkListItem[%lu] = %s.\n",
                    __func__, cacheBlocks[block_index].addr + (i * sizeof(WorkListItem)),
                    cacheBlocks[block_index].items[i].to_string());
                    peerPushEngine->recvWLItem(cacheBlocks[block_index].items[i]);
                    DPRINTF(MPU, "%s: Sent cache line[%d][%d] to PushEngine.\n",
                                                        __func__, block_index, i);
                }
            }
        }

        if (cacheBlocks[block_index].hasConflict) {
            assert(!MSHRMap[block_index].empty());
            DPRINTF(MPU, "%s: A conflict exists for cache line[%d]. There is "
                        "enough space in outstandingMemReqQueue for a read "
                        "packet.\n", __func__, block_index);
            Addr miss_addr = MSHRMap[block_index][0];
            DPRINTF(MPU, "%s: First conflicting address for cache line[%d] is"
                        " Addr: %lu.\n", __func__, block_index, miss_addr);

            Addr aligned_miss_addr =
                std::floor(miss_addr / peerMemoryAtomSize) *
                    peerMemoryAtomSize;
            PacketPtr read_pkt = createReadPacket(aligned_miss_addr,
                                                    peerMemoryAtomSize);
            DPRINTF(MPU, "%s: Created a read packet for Addr: %lu."
                        " req addr (aligned_addr) = %lu, size = %d.\n",
                        __func__, miss_addr, aligned_miss_addr, peerMemoryAtomSize);
            enqueueMemReq(read_pkt);
            DPRINTF(MPU, "%s: Added the evicting write back packet along with "
                        "its subsequent read packet (to service the conflicts)"
                        " to outstandingMemReqQueue.\n" , __func__);

            cacheBlocks[block_index].addr = aligned_miss_addr;
            cacheBlocks[block_index].takenMask = 0;
            cacheBlocks[block_index].allocated = true;
            cacheBlocks[block_index].valid = false;
            cacheBlocks[block_index].hasConflict = true;
            cacheBlocks[block_index].hasChange = false;
        } else {
            DPRINTF(MPU, "%s: No conflict exists for cache line[%d]. There is "
                    "enough space in outstandingMemReqQueue for the write back"
                    " packet.\n", __func__, block_index);
            DPRINTF(MPU, "%s: Added the write back packet to "
                        "outstandingMemReqQueue.\n", __func__);

            // Since allocated is false, does not matter what the address is.
            cacheBlocks[block_index].takenMask = 0;
            cacheBlocks[block_index].allocated = false;
            cacheBlocks[block_index].valid = false;
            cacheBlocks[block_index].hasConflict = false;
            cacheBlocks[block_index].hasChange = false;
        }

    } else {
        DPRINTF(MPU, "%s: cache line[%d] has been read since being scheduled "
                    "for eviction. Therefore, ignoring the evict schedule.\n",
                    __func__, block_index);
    }

    evictQueue.pop_front();
    DPRINTF(MPU, "%s: Popped an item from evictQueue. evictQueue.size "
                        " = %u.\n", __func__, evictQueue.size());

    if ((!nextApplyAndCommitEvent.scheduled()) &&
        (!evictQueue.empty())) {
        schedule(nextApplyAndCommitEvent, nextCycle());
    }
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
             "Number of cache rejections.")
{
}

void
CoalesceEngine::CoalesceStats::regStats()
{
    using namespace statistics;
}

}
