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
    applyQueue(numLines),
    evictQueue(numLines),
    nextRespondEvent([this] { processNextRespondEvent(); }, name()),
    nextApplyEvent([this] { processNextApplyEvent(); }, name()),
    nextEvictEvent([this] { processNextEvictEvent(); }, name()),
    stats(*this)
{
    assert(isPowerOf2(numLines) && isPowerOf2(numElementsPerLine));
    cacheBlocks = new Block [numLines];
    for (int i = 0; i < numLines; i++) {
        cacheBlocks[i] = Block(numElementsPerLine);
    }

    peerPushEngine->registerCoalesceEngine(this, numElementsPerLine);

    needsApply.reset();
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
    while(!found) {
        for (auto range: vertex_ranges) {
            if (range.contains(first_match_addr)) {
                found = true;
                break;
            }
        }
        first_match_addr += peerMemoryAtomSize;
    }

    found = false;
    Addr second_match_addr = first_match_addr + peerMemoryAtomSize;
    while(!found) {
        for (auto range: vertex_ranges) {
            if (range.contains(second_match_addr)) {
                found = true;
                break;
            }
        }
        second_match_addr += peerMemoryAtomSize;
    }

    nmpu = (int) ((second_match_addr - first_match_addr) / peerMemoryAtomSize);
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
            __func__, addr, block_index, wl_offset,
            cacheBlocks[block_index].items[wl_offset].to_string(),
            responseQueue.size());
        // TODO: Add a stat to count the number of WLItems that have been touched.
        cacheBlocks[block_index].busyMask |= (1 << wl_offset);
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
                                "line[%d].\n", __func__, addr, block_index);
                    stats.readMisses++;
                    stats.numVertexReads++;
                    if (!cacheBlocks[block_index].busyMask) {
                        applyQueue.push_back(block_index);
                        DPRINTF(MPU, "%s: Added %d to applyQueue. "
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
                    cacheBlocks[block_index].busyMask = 0;
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
CoalesceEngine::respondToMemAlarm()
{
    assert(!nextEvictEvent.scheduled());
    schedule(nextEvictEvent, nextCycle());
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
        Addr aligned_miss_addr = roundDown<Addr, Addr>(miss_addr, peerMemoryAtomSize);
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
            cacheBlocks[block_index].busyMask |= (1 << wl_offset);
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
    Addr aligned_addr = roundDown<Addr, Addr>(addr, peerMemoryAtomSize);
    int block_index = (aligned_addr / peerMemoryAtomSize) % numLines;
    int wl_offset = (addr - aligned_addr) / sizeof(WorkListItem);

    DPRINTF(MPU, "%s: Received a write for WorkListItem: %s with Addr: %lu.\n",
                __func__, wl.to_string(), addr);
    assert((cacheBlocks[block_index].busyMask & (1 << wl_offset)) ==
            (1 << wl_offset));

    if (cacheBlocks[block_index].items[wl_offset].tempProp != wl.tempProp) {
        cacheBlocks[block_index].dirty = true;
        stats.numVertexWrites++;
    }

    cacheBlocks[block_index].items[wl_offset] = wl;
    cacheBlocks[block_index].busyMask &= ~(1 << wl_offset);
    DPRINTF(MPU, "%s: Wrote to cache line[%d][%d] = %s.\n",
                __func__, block_index, wl_offset,
                cacheBlocks[block_index].items[wl_offset].to_string());

    // TODO: Make this more general and programmable.
    if ((cacheBlocks[block_index].busyMask == 0)) {(aligned_addr / peerMemoryAtomSize) % numLines;
        DPRINTF(MPU, "%s: Received all the expected writes for cache line[%d]."
                    " It does not have any taken items anymore.\n",
                    __func__, block_index);
        applyQueue.push_back(block_index);
        DPRINTF(MPU, "%s: Added %d to applyQueue. applyQueue.size = %u.\n",
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

    if (cacheBlocks[block_index].busyMask) {
        DPRINTF(MPU, "%s: cache line [%d] has been taken amid apply process. "
                    "Therefore, ignoring the apply schedule.\n",
                    __func__, block_index);
        stats.falseApplySchedules++;
    } else if (!cacheBlocks[block_index].dirty) {
        DPRINTF(MPU, "%s: cache line [%d] has no change. Therefore, no apply "
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

                Addr block_addr = cacheBlocks[block_index].addr;
                int atom_index = (int) (block_addr / (peerMemoryAtomSize * nmpu));
                int block_bits = (int) (peerMemoryAtomSize / sizeof(WorkListItem));
                int bit_index = atom_index * block_bits + i;

                if (needsApply[bit_index] == 1) {
                    DPRINTF(MPU, "%s: WorkListItem[%lu] already set in bit-vector."
                                " Not doing anything further.\n", __func__,
                                block_addr + (i * sizeof(WorkListItem)));
                } else {
                    if (peerPushEngine->allocatePushSpace()) {
                        peerPushEngine->recvWLItem(
                            cacheBlocks[block_index].items[i]);
                    } else {
                        needsApply[bit_index] = 1;
                    }
                }
            }
        }
    }

    // TODO: This is where eviction policy goes
    if (cacheBlocks[block_index].hasConflict){
        evictQueue.push_back(block_index);
        DPRINTF(MPU, "%s: Added %d to evictQueue. evictQueue.size = %u.\n",
                __func__, block_index, evictQueue.size());
    }

    applyQueue.pop_front();

    if ((!evictQueue.empty()) &&
        (!pendingMemAlarm()) &&
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

    if ((cacheBlocks[block_index].busyMask) ||
        (applyQueue.find(block_index))) {
        DPRINTF(MPU, "%s: cache line [%d] has been taken amid evict process. "
                    "Therefore, ignoring the apply schedule.\n",
                    __func__, block_index);
        stats.falseEvictSchedules++;
    } else {
        int space_needed = cacheBlocks[block_index].dirty ?
                        (cacheBlocks[block_index].hasConflict ? 2 : 1) :
                        (cacheBlocks[block_index].hasConflict ? 1 : 0);
        if (!allocateMemReqSpace(space_needed)) {
            DPRINTF(MPU, "%s: There is not enough space in memReqQueue to "
                    "procees the eviction of cache line [%d]. dirty: %d, "
                    "hasConflict: %d.\n", __func__, block_index,
                    cacheBlocks[block_index].dirty,
                    cacheBlocks[block_index].hasConflict);
            requestMemAlarm(space_needed);
            return;
        } else {
            if (cacheBlocks[block_index].dirty) {
                DPRINTF(MPU, "%s: Change observed on cache line [%d].\n",
                            __func__, block_index);
                PacketPtr write_pkt = createWritePacket(
                    cacheBlocks[block_index].addr, peerMemoryAtomSize,
                    (uint8_t*) cacheBlocks[block_index].items);
                DPRINTF(MPU, "%s: Created a write packet to Addr: %lu, "
                            "size = %d.\n", __func__,
                            write_pkt->getAddr(), write_pkt->getSize());
                enqueueMemReq(write_pkt);
            }

            if (cacheBlocks[block_index].hasConflict) {
                assert(!MSHRMap[block_index].empty());
                Addr miss_addr = MSHRMap[block_index].front();
                DPRINTF(MPU, "%s: First conflicting address for cache line[%d]"
                        " is Addr: %lu.\n", __func__, block_index, miss_addr);

                Addr aligned_miss_addr =
                    roundDown<Addr, Addr>(miss_addr, peerMemoryAtomSize);

                PacketPtr read_pkt = createReadPacket(aligned_miss_addr,
                                                        peerMemoryAtomSize);
                DPRINTF(MPU, "%s: Created a read packet for Addr: %lu."
                            " req addr (aligned_addr) = %lu, size = %d.\n",
                            __func__, miss_addr,
                            read_pkt->getAddr(), read_pkt->getSize());
                enqueueMemReq(read_pkt);

                cacheBlocks[block_index].addr = aligned_miss_addr;
                cacheBlocks[block_index].busyMask = 0;
                cacheBlocks[block_index].allocated = true;
                cacheBlocks[block_index].valid = false;
                cacheBlocks[block_index].hasConflict = true;
                cacheBlocks[block_index].dirty = false;
                DPRINTF(MPU, "%s: Allocated cache line [%d] for Addr: %lu.\n",
                            __func__, block_index, aligned_miss_addr);
            } else {

                // Since allocated is false, does not matter what the address is.
                cacheBlocks[block_index].busyMask = 0;
                cacheBlocks[block_index].allocated = false;
                cacheBlocks[block_index].valid = false;
                cacheBlocks[block_index].hasConflict = false;
                cacheBlocks[block_index].dirty = false;
                DPRINTF(MPU, "%s: Deallocated cache line [%d].\n",
                            __func__, block_index);
            }
        }
    }

    evictQueue.pop_front();

    if ((!evictQueue.empty()) &&
        (!nextEvictEvent.scheduled())) {
        schedule(nextEvictEvent, nextCycle());
    }
}

void
CoalesceEngine::respondToPushAlarm()
{
    DPRINTF(MPU, "%s: Received a Push alarm.\n", __func__);
    int it;
    for (it = 0; it < MAX_BITVECTOR_SIZE; it += numElementsPerLine) {
        uint32_t slice = 0;
        for (int i = 0; i < numElementsPerLine; i++) {
            slice <<= 1;
            slice |= needsApply[it + i];
        }
        if (slice) {
            break;
        }
    }
    DPRINTF(MPU, "%s: Found slice %u at %d position in needsApply.\n",
                __func__, slice, it);

    Addr block_addr = (nmpu * peerMemoryAtomSize) *
                ((int)(it / (peerMemoryAtomSize / sizeof(WorkListItem))));
    int block_index = ((int) (block_addr / peerMemoryAtomSize)) % numLines;

    if ((cacheBlocks[block_index].addr == block_addr) &&
        (cacheBlocks[block_index].valid)) {
        // hit in cache
        bool do_push = cacheBlocks[block_index].busyMask == 0 ? true : false;
        for (int i = 0; i < numElementsPerLine; i++) {
            peerPushEngine->recvWLItemRetry(
                        cacheBlocks[block_index].items[i], do_push);
        }

        // TODO: Should we add block_index to evict_queue?
        if (do_push && cacheBlocks[block_index].hasConflict) {
            evictQueue.push_back(block_index);
        }
    } else {
        PacketPtr pkt = createReadPacket(block_addr, peerMemoryAtomSize);

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
