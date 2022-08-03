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

#include <bitset>

#include "accl/graph/sega/wl_engine.hh"
#include "base/intmath.hh"
#include "debug/ApplyUpdates.hh"
#include "debug/BitVector.hh"
#include "debug/CacheBlockState.hh"
#include "debug/CoalesceEngine.hh"
#include "debug/SEGAStructureSize.hh"
#include "mem/packet_access.hh"

namespace gem5
{

CoalesceEngine::CoalesceEngine(const Params &params):
    BaseMemoryEngine(params),
    peerPushEngine(params.peer_push_engine),
    numLines((int) (params.cache_size / peerMemoryAtomSize)),
    numElementsPerLine((int) (peerMemoryAtomSize / sizeof(WorkListItem))),
    numMSHREntries(params.num_mshr_entry), numTgtsPerMSHR(params.num_tgts_per_mshr),
    numRetriesReceived(0),
    nextMemoryEvent([this] {
        processNextMemoryEvent();
        }, name() + ".nextMemoryEvent"),
    nextResponseEvent([this] {
        processNextResponseEvent();
        }, name() + ".nextResponseEvent"),
    nextApplyEvent([this] {
        processNextApplyEvent();
        }, name() + ".nextApplyEvent"),
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
CoalesceEngine::registerWLEngine(WLEngine* wl_engine)
{
    peerWLEngine = wl_engine;
}

DrainState
CoalesceEngine::drain()
{
    DPRINTF(CoalesceEngine, "%s: drain called.\n");
    return DrainState::Drained;
}

// addr should be aligned to peerMemoryAtomSize
int
CoalesceEngine::getBlockIndex(Addr addr)
{
    assert((addr % peerMemoryAtomSize) == 0);
    Addr trimmed_addr = peerMemoryRange.removeIntlvBits(addr);
    return ((int) (trimmed_addr / peerMemoryAtomSize)) % numLines;
}

// addr should be aligned to peerMemoryAtomSize
int
CoalesceEngine::getBitIndexBase(Addr addr)
{
    assert((addr % peerMemoryAtomSize) == 0);
    Addr trimmed_addr = peerMemoryRange.removeIntlvBits(addr);
    int atom_index = (int) (trimmed_addr / peerMemoryAtomSize);
    int block_bits = (int) (peerMemoryAtomSize / sizeof(WorkListItem));
    return atom_index * block_bits;
}

// index should be aligned to (peerMemoryAtomSize / sizeof(WorkListItem))
Addr
CoalesceEngine::getBlockAddrFromBitIndex(int index)
{
    assert((index % ((int) (peerMemoryAtomSize / sizeof(WorkListItem)))) == 0);
    Addr trimmed_addr = index * sizeof(WorkListItem);
    return peerMemoryRange.addIntlvBits(trimmed_addr);
}

bool
CoalesceEngine::recvWLRead(Addr addr)
{
    Addr aligned_addr = roundDown<Addr, size_t>(addr, peerMemoryAtomSize);
    assert(aligned_addr % peerMemoryAtomSize == 0);
    int block_index = getBlockIndex(aligned_addr);
    assert(block_index < numLines);
    int wl_offset = (addr - aligned_addr) / sizeof(WorkListItem);
    assert(wl_offset < numElementsPerLine);
    DPRINTF(CoalesceEngine,  "%s: Received a read request for addr: %lu. "
                        "This request maps to cacheBlocks[%d], aligned_addr: "
                        "%lu, and wl_offset: %d.\n", __func__, addr,
                        block_index, aligned_addr, wl_offset);
    DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                block_index, cacheBlocks[block_index].to_string());

    if ((cacheBlocks[block_index].addr == aligned_addr) &&
        (cacheBlocks[block_index].valid)) {
        DPRINTF(CoalesceEngine,  "%s: Addr: %lu is a hit.\n", __func__, addr);
        stats.readHits++;
        assert(!cacheBlocks[block_index].pendingData);
        // No cache block could be in pendingApply and pendingWB at the
        // same time.
        assert(!(cacheBlocks[block_index].pendingApply &&
                cacheBlocks[block_index].pendingWB));
        // Hit
        // TODO: Add a hit latency as a param for this object.
        // Can't just schedule the nextResponseEvent for latency cycles in
        // the future.
        responseQueue.push_back(std::make_tuple(addr,
                    cacheBlocks[block_index].items[wl_offset]));
        DPRINTF(SEGAStructureSize, "%s: Added (addr: %lu, wl: %s) "
                        "to responseQueue. responseQueue.size = %d, "
                        "responseQueueSize = %d.\n", __func__, addr,
                        cacheBlocks[block_index].items[wl_offset].to_string(),
                        responseQueue.size(),
                        peerWLEngine->getRegisterFileSize());
        DPRINTF(CoalesceEngine, "%s: Added (addr: %lu, wl: %s) "
                        "to responseQueue. responseQueue.size = %d, "
                        "responseQueueSize = %d.\n", __func__, addr,
                        cacheBlocks[block_index].items[wl_offset].to_string(),
                        responseQueue.size(),
                        peerWLEngine->getRegisterFileSize());
        // TODO: Stat to count the number of WLItems that have been touched.
        cacheBlocks[block_index].busyMask |= (1 << wl_offset);
        // If they are scheduled for apply and WB those schedules should be
        // discarded. Since there is no easy way to take items out of the
        // function queue. Those functions check for their respective bits
        // and skip the process if the respective bit is set to false.
        cacheBlocks[block_index].pendingApply = false;
        cacheBlocks[block_index].pendingWB = false;
        cacheBlocks[block_index].lastChangedTick = curTick();
        DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                    block_index, cacheBlocks[block_index].to_string());

        if (!nextResponseEvent.scheduled()) {
            schedule(nextResponseEvent, nextCycle());
        }
        stats.numVertexReads++;
        return true;
    } else if ((cacheBlocks[block_index].addr == aligned_addr) &&
                (cacheBlocks[block_index].pendingData)) {
        // Hit under miss
        DPRINTF(CoalesceEngine,  "%s: Addr: %lu is a hit under miss.\n",
                                                        __func__, addr);
        stats.readHitUnderMisses++;
        assert(!cacheBlocks[block_index].valid);
        assert(cacheBlocks[block_index].busyMask == 0);
        assert(!cacheBlocks[block_index].needsWB);
        assert(!cacheBlocks[block_index].needsApply);
        assert(!cacheBlocks[block_index].pendingApply);
        assert(!cacheBlocks[block_index].pendingWB);

        assert(MSHR.size() <= numMSHREntries);
        assert(MSHR.find(block_index) != MSHR.end());
        assert(MSHR[block_index].size() <= numTgtsPerMSHR);
        if (MSHR[block_index].size() == numTgtsPerMSHR) {
            DPRINTF(CoalesceEngine,  "%s: Out of targets for "
                        "cacheBlocks[%d]. Rejecting request.\n",
                                        __func__, block_index);
            stats.readRejections++;
            return false;
        } else {
            DPRINTF(CoalesceEngine,  "%s: MSHR entries are available for "
                            "cacheBlocks[%d].\n", __func__, block_index);
        }
        MSHR[block_index].push_back(addr);
        DPRINTF(CoalesceEngine,  "%s: Added Addr: %lu to targets "
                "for cacheBlocks[%d].\n", __func__, addr, block_index);
        DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                    block_index, cacheBlocks[block_index].to_string());
        return true;
    } else {
        // miss
        // FIXME: Make this assert work. It will break if the cache block
        // is cold and addr or aligned_addr is 0. It fails because cache block
        // addr field is initialized to 0. Unfortunately Addr type is unsigned.
        // So you can not initialized addr to -1.
        // assert(cacheBlocks[block_index].addr != aligned_addr);
        assert(MSHR.size() <= numMSHREntries);
        DPRINTF(CoalesceEngine,  "%s: Addr: %lu is a miss.\n", __func__, addr);
        if (MSHR.find(block_index) == MSHR.end()) {
            DPRINTF(CoalesceEngine,  "%s: Respective cacheBlocks[%d] for Addr:"
                    " %lu not found in MSHRs.\n", __func__, block_index, addr);
            if (MSHR.size() == numMSHREntries) {
                // Out of MSHR entries
                DPRINTF(CoalesceEngine,  "%s: Out of MSHR entries. "
                                "Rejecting request.\n", __func__);
                // TODO: Break out read rejections into more than one stat
                // based on the cause of the rejection
                stats.readRejections++;
                return false;
            } else {
                DPRINTF(CoalesceEngine,  "%s: MSHR "
                    "entries available.\n", __func__);
                if ((cacheBlocks[block_index].valid) ||
                    (cacheBlocks[block_index].pendingData)) {
                    DPRINTF(CoalesceEngine,  "%s: Addr: %lu has a conflict "
                                "with Addr: %lu.\n", __func__, addr,
                                cacheBlocks[block_index].addr);
                    if ((cacheBlocks[block_index].valid) &&
                        (cacheBlocks[block_index].busyMask == 0) &&
                        (!cacheBlocks[block_index].pendingApply) &&
                        (!cacheBlocks[block_index].pendingWB)) {
                        DPRINTF(CoalesceEngine, "%s: cacheBlocks[%d] is in "
                                    "idle state.\n", __func__, block_index);
                        // We're in idle state
                        // Idle: valid && !pendingApply && !pendingWB;
                        // Note 0: needsApply has to be false. Because
                        // A cache line enters the idle state from two
                        // other states. First a busy state that does not
                        // need apply (needsApply is already false) or
                        // from pendingApplyState after being applied which
                        // clears the needsApply bit. needsApply is useful
                        // when a cache block has transitioned from
                        // pendingApply to busy without the apply happening.
                        // Note 1: pendingData does not have to be evaluated
                        // becuase pendingData is cleared when data
                        // arrives from the memory and valid does not
                        // denote cleanliness of the line. Rather it
                        // is used to differentiate between empty blocks
                        // and the blocks that have data from memory.
                        // pendingData denotes the transient state between
                        // getting a miss and getting the data for that miss.
                        // valid basically means that the data in the cache
                        // could be used to respond to read/write requests.
                        assert(!cacheBlocks[block_index].needsApply);
                        assert(!cacheBlocks[block_index].pendingData);
                        // There are no conflicts in idle state.
                        assert(MSHR.find(block_index) == MSHR.end());
                        if (cacheBlocks[block_index].needsWB) {
                            DPRINTF(CoalesceEngine, "%s: cacheBlocks[%d] needs"
                            "to be written back.\n", __func__, block_index);
                            cacheBlocks[block_index].pendingWB = true;
                            cacheBlocks[block_index].lastChangedTick = curTick();
                            memoryFunctionQueue.emplace_back(
                                [this] (int block_index, Tick schedule_tick) {
                                processNextWriteBack(block_index, schedule_tick);
                            }, block_index, curTick());
                            DPRINTF(CoalesceEngine, "%s: Pushed "
                                        "processNextWriteBack for input "
                                        "%d to memoryFunctionQueue.\n",
                                        __func__, block_index);
                            if ((!nextMemoryEvent.pending()) &&
                                (!nextMemoryEvent.scheduled())) {
                                schedule(nextMemoryEvent, nextCycle());
                            }
                            DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: "
                                    "%s.\n", __func__, block_index,
                                    cacheBlocks[block_index].to_string());
                        } else {
                            DPRINTF(CoalesceEngine, "%s: cacheBlocks[%d] does "
                                            "not need to be written back.\n",
                                                        __func__, block_index);
                            cacheBlocks[block_index].addr = aligned_addr;
                            cacheBlocks[block_index].valid = false;
                            cacheBlocks[block_index].busyMask = 0;
                            cacheBlocks[block_index].needsWB = false;
                            cacheBlocks[block_index].needsApply = false;
                            cacheBlocks[block_index].pendingData = true;
                            cacheBlocks[block_index].pendingApply = false;
                            cacheBlocks[block_index].pendingWB = false;
                            cacheBlocks[block_index].lastChangedTick = curTick();
                            memoryFunctionQueue.emplace_back(
                                [this] (int block_index, Tick schedule_tick) {
                                    processNextRead(block_index, schedule_tick);
                                }, block_index, curTick());
                            DPRINTF(CoalesceEngine, "%s: Pushed "
                                        "processNextRead for input "
                                        "%d to memoryFunctionQueue.\n",
                                        __func__, block_index);
                            if ((!nextMemoryEvent.pending()) &&
                                (!nextMemoryEvent.scheduled())) {
                                schedule(nextMemoryEvent, nextCycle());
                            }
                            DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: "
                                    "%s.\n", __func__, block_index,
                                    cacheBlocks[block_index].to_string());
                        }
                    }
                    // cacheBlocks[block_index].hasConflict = true;
                    MSHR[block_index].push_back(addr);
                    DPRINTF(CoalesceEngine,  "%s: Added Addr: %lu to targets "
                        "for cacheBlocks[%d].\n", __func__, addr, block_index);
                    stats.readMisses++;
                    // TODO: Add readConflicts here.
                    stats.numVertexReads++;
                    return true;
                } else {
                    // MSHR available and no conflict
                    DPRINTF(CoalesceEngine,  "%s: Addr: %lu has no conflict. "
                                            "Allocating a cache line for it.\n"
                                                            , __func__, addr);
                    assert(!cacheBlocks[block_index].valid);
                    assert(cacheBlocks[block_index].busyMask == 0);
                    assert(!cacheBlocks[block_index].needsWB);
                    assert(!cacheBlocks[block_index].needsApply);
                    assert(!cacheBlocks[block_index].pendingData);
                    assert(!cacheBlocks[block_index].pendingApply);
                    assert(!cacheBlocks[block_index].pendingWB);
                    assert(MSHR[block_index].size() == 0);

                    cacheBlocks[block_index].addr = aligned_addr;
                    cacheBlocks[block_index].busyMask = 0;
                    cacheBlocks[block_index].valid = false;
                    cacheBlocks[block_index].needsWB = false;
                    cacheBlocks[block_index].needsApply = false;
                    cacheBlocks[block_index].pendingData = true;
                    cacheBlocks[block_index].pendingApply = false;
                    cacheBlocks[block_index].pendingWB = false;
                    cacheBlocks[block_index].lastChangedTick = curTick();
                    DPRINTF(CoalesceEngine, "%s: Allocated cacheBlocks[%d] for"
                                " Addr: %lu.\n", __func__, block_index, addr);
                    MSHR[block_index].push_back(addr);
                    DPRINTF(CoalesceEngine, "%s: Added Addr: %lu to targets "
                        "for cacheBlocks[%d].\n", __func__, addr, block_index);
                    memoryFunctionQueue.emplace_back(
                        [this] (int block_index, Tick schedule_tick) {
                            processNextRead(block_index, schedule_tick);
                        }, block_index, curTick());
                    DPRINTF(CoalesceEngine, "%s: Pushed processNextRead for "
                                        "input %d to memoryFunctionQueue.\n",
                                                    __func__, block_index);
                    if ((!nextMemoryEvent.pending()) &&
                        (!nextMemoryEvent.scheduled())) {
                        schedule(nextMemoryEvent, nextCycle());
                    }
                    DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n",
                                    __func__, block_index,
                                    cacheBlocks[block_index].to_string());
                    stats.readMisses++;
                    stats.numVertexReads++;
                    return true;
                }
            }
        } else {
            DPRINTF(CoalesceEngine,  "%s: Respective cacheBlocks[%d] for "
                "Addr: %lu already in MSHRs. It has a conflict "
                "with addr: %lu.\n", __func__, block_index, addr,
                                cacheBlocks[block_index].addr);
            assert(MSHR[block_index].size() <= numTgtsPerMSHR);
            assert(MSHR[block_index].size() > 0);
            if (MSHR[block_index].size() == numTgtsPerMSHR) {
                DPRINTF(CoalesceEngine,  "%s: Out of targets for "
                            "cacheBlocks[%d]. Rejecting request.\n",
                                            __func__, block_index);
                stats.readRejections++;
                return false;
            }
            DPRINTF(CoalesceEngine, "%s: There is room for another target "
                            "for cacheBlocks[%d].\n", __func__, block_index);

            // cacheBlocks[block_index].hasConflict = true;
            // TODO: Might want to differentiate between different misses.
            stats.readMisses++;

            MSHR[block_index].push_back(addr);
            DPRINTF(CoalesceEngine,  "%s: Added Addr: %lu to targets for "
                            "cacheBlocks[%d].\n", __func__, addr, block_index);
            stats.numVertexReads++;
            return true;
        }
    }
}

bool
CoalesceEngine::handleMemResp(PacketPtr pkt)
{
    assert(pkt->isResponse());
    DPRINTF(CoalesceEngine,  "%s: Received packet: %s from memory.\n",
                                                __func__, pkt->print());
    if (pkt->isWrite()) {
        DPRINTF(CoalesceEngine, "%s: Dropped the write response.\n", __func__);
        delete pkt;
        return true;
    }

    Addr addr = pkt->getAddr();
    int block_index = getBlockIndex(addr);

    if (pkt->findNextSenderState<SenderState>()) {
        assert(!((cacheBlocks[block_index].addr == addr) &&
                (cacheBlocks[block_index].valid)));
        // We have read the address to send the wl and it is not in the
        // cache. Simply send the items to the PushEngine.
        int it = getBitIndexBase(addr);
        DPRINTF(CoalesceEngine, "%s: Received read response for retry "
                    "for addr %lu. It was not found in the cache.\n",
                    __func__, addr);
        WorkListItem* items = pkt->getPtr<WorkListItem>();
        int push_needed = 0;
        // No applying of the line needed.
        DPRINTF(BitVector, "%s: needsPush.count: %d.\n",
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
        DPRINTF(BitVector, "%s: needsPush.count: %d.\n",
                            __func__, needsPush.count());
        peerPushEngine->deallocatePushSpace(
                                numElementsPerLine - push_needed);
        assert(peerPushEngine->getNumRetries() == needsPush.count());
        // }
        delete pkt;
        return true;
    }

    if (cacheBlocks[block_index].addr == addr) {
        DPRINTF(CoalesceEngine, "%s: Received read response to "
                "fill cacheBlocks[%d].\n", __func__, block_index);
        DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                        block_index, cacheBlocks[block_index].to_string());
        assert(!cacheBlocks[block_index].valid);
        assert(cacheBlocks[block_index].busyMask == 0);
        assert(!cacheBlocks[block_index].needsWB);
        assert(!cacheBlocks[block_index].needsApply);
        assert(cacheBlocks[block_index].pendingData);
        assert(!cacheBlocks[block_index].pendingApply);
        assert(!cacheBlocks[block_index].pendingWB);
        assert(MSHR.find(block_index) != MSHR.end());
        pkt->writeDataToBlock((uint8_t*) cacheBlocks[block_index].items,
                                                peerMemoryAtomSize);
        for (int i = 0; i < numElementsPerLine; i++) {
        DPRINTF(CoalesceEngine,  "%s: Wrote cacheBlocks[%d][%d] = %s.\n",
                            __func__, block_index, i,
                            cacheBlocks[block_index].items[i].to_string());
        }
        cacheBlocks[block_index].valid = true;
        cacheBlocks[block_index].pendingData = false;
        cacheBlocks[block_index].lastChangedTick = curTick();
        DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                        block_index, cacheBlocks[block_index].to_string());
        delete pkt;
    }

    // FIXME: Get rid of servicedIndices (maybe use an iterator)
    std::vector<int> servicedIndices;
    for (int i = 0; i < MSHR[block_index].size(); i++) {
        Addr miss_addr = MSHR[block_index][i];
        Addr aligned_miss_addr =
            roundDown<Addr, size_t>(miss_addr, peerMemoryAtomSize);
        if (aligned_miss_addr == addr) {
            int wl_offset = (miss_addr - aligned_miss_addr) / sizeof(WorkListItem);
            DPRINTF(CoalesceEngine,  "%s: Addr: %lu in the MSHR for "
                        "cacheBlocks[%d] can be serviced with the received "
                        "packet.\n",__func__, miss_addr, block_index);
            // TODO: Make this block of code into a function
            responseQueue.push_back(std::make_tuple(miss_addr,
                    cacheBlocks[block_index].items[wl_offset]));
            DPRINTF(SEGAStructureSize, "%s: Added (addr: %lu, wl: %s) "
                        "to responseQueue. responseQueue.size = %d, "
                        "responseQueueSize = %d.\n", __func__, miss_addr,
                        cacheBlocks[block_index].items[wl_offset].to_string(),
                        responseQueue.size(),
                        peerWLEngine->getRegisterFileSize());
            DPRINTF(CoalesceEngine, "%s: Added (addr: %lu, wl: %s) "
                        "to responseQueue. responseQueue.size = %d, "
                        "responseQueueSize = %d.\n", __func__, addr,
                        cacheBlocks[block_index].items[wl_offset].to_string(),
                        responseQueue.size(),
                        peerWLEngine->getRegisterFileSize());
            // TODO: Add a stat to count the number of WLItems that have been touched.
            cacheBlocks[block_index].busyMask |= (1 << wl_offset);
            cacheBlocks[block_index].lastChangedTick = curTick();
            DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                        block_index, cacheBlocks[block_index].to_string());
            // End of the said block
            servicedIndices.push_back(i);
            // DPRINTF(CoalesceEngine,  "%s: Added index: %d of MSHR for cacheBlocks[%d] for "
            //             "removal.\n", __func__, i, block_index);
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
        DPRINTF(CoalesceEngine,  "%s: Addr: %lu has been serviced "
                        "and is removed.\n", __func__, print_addr);
    }

    if (MSHR[block_index].empty()) {
        MSHR.erase(block_index);
        // cacheBlocks[block_index].hasConflict = false;
    }

    if ((!nextResponseEvent.scheduled()) &&
        (!responseQueue.empty())) {
        schedule(nextResponseEvent, nextCycle());
    }

    return true;
}

// TODO: For loop to empty the entire responseQueue.
void
CoalesceEngine::processNextResponseEvent()
{
    Addr addr_response;
    WorkListItem worklist_response;

    std::tie(addr_response, worklist_response) = responseQueue.front();
    peerWLEngine->handleIncomingWL(addr_response, worklist_response);
    DPRINTF(CoalesceEngine,
                "%s: Sent WorkListItem: %s with addr: %lu to WLEngine.\n",
                __func__, worklist_response.to_string(), addr_response);

    responseQueue.pop_front();
    DPRINTF(SEGAStructureSize,  "%s: Popped a response from responseQueue. "
                "responseQueue.size = %d, responseQueueSize = %d.\n", __func__,
                responseQueue.size(), peerWLEngine->getRegisterFileSize());
    DPRINTF(CoalesceEngine,  "%s: Popped a response from responseQueue. "
                "responseQueue.size = %d, responseQueueSize = %d.\n", __func__,
                responseQueue.size(), peerWLEngine->getRegisterFileSize());

    if ((!nextResponseEvent.scheduled()) &&
        (!responseQueue.empty())) {
        schedule(nextResponseEvent, nextCycle());
    }
}

void
CoalesceEngine::recvWLWrite(Addr addr, WorkListItem wl)
{
    Addr aligned_addr = roundDown<Addr, size_t>(addr, peerMemoryAtomSize);
    int block_index = getBlockIndex(aligned_addr);
    int wl_offset = (addr - aligned_addr) / sizeof(WorkListItem);
    DPRINTF(CoalesceEngine,  "%s: Received a write request for addr: %lu with "
                        "wl: %s. This request maps to cacheBlocks[%d], "
                        "aligned_addr: %lu, and wl_offset: %d.\n",
                        __func__, addr, wl.to_string(),
                        block_index, aligned_addr, wl_offset);
    DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                block_index, cacheBlocks[block_index].to_string());
    DPRINTF(CoalesceEngine,  "%s: Received a write for WorkListItem: %s "
                "with Addr: %lu.\n", __func__, wl.to_string(), addr);
    // Desing does not allow for write misses for now.
    assert(cacheBlocks[block_index].addr == aligned_addr);
    // cache state asserts
    assert(cacheBlocks[block_index].valid);
    assert(cacheBlocks[block_index].busyMask != 0);
    assert(!cacheBlocks[block_index].pendingData);
    assert(!cacheBlocks[block_index].pendingApply);
    assert(!cacheBlocks[block_index].pendingWB);

    // respective bit in busyMask for wl is set.
    assert((cacheBlocks[block_index].busyMask & (1 << wl_offset)) ==
            (1 << wl_offset));

    if (cacheBlocks[block_index].items[wl_offset].tempProp != wl.tempProp) {
        cacheBlocks[block_index].items[wl_offset] = wl;
        cacheBlocks[block_index].needsApply |= true;
        // NOTE: We don't set needsWB and rely on processNextApplyEvent to
        // set that bit.
        stats.numVertexWrites++;
    }

    cacheBlocks[block_index].busyMask &= ~(1 << wl_offset);
    cacheBlocks[block_index].lastChangedTick = curTick();
    DPRINTF(CoalesceEngine,  "%s: Wrote to cacheBlocks[%d][%d] = %s.\n",
                __func__, block_index, wl_offset,
                cacheBlocks[block_index].items[wl_offset].to_string());
    DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                        block_index, cacheBlocks[block_index].to_string());

    // TODO: Make this more general and programmable.
    if ((cacheBlocks[block_index].busyMask == 0)) {
        if (cacheBlocks[block_index].needsApply) {
            cacheBlocks[block_index].pendingApply = true;
            cacheBlocks[block_index].lastChangedTick = curTick();
            applyQueue.push_back(block_index);
            DPRINTF(CoalesceEngine, "%s: Added cacheBlocks[%d] to "
                            "applyQueue.\n", __func__, block_index);
            if ((!applyQueue.empty()) &&
                (!nextApplyEvent.scheduled())) {
                schedule(nextApplyEvent, nextCycle());
            }
        } else {
            assert(MSHR.size() <= numMSHREntries);
            // cache line has conflict.
            if (MSHR.find(block_index) != MSHR.end()) {
                DPRINTF(CoalesceEngine, "%s: cacheBlocks[%d] has pending "
                                    "conflict.\n", __func__, block_index);
                if (cacheBlocks[block_index].needsWB) {
                    DPRINTF(CoalesceEngine, "%s: cacheBlocks[%d] needs a write"
                                            " back.\n", __func__, block_index);
                    cacheBlocks[block_index].pendingWB = true;
                    cacheBlocks[block_index].lastChangedTick = curTick();
                    memoryFunctionQueue.emplace_back(
                        [this] (int block_index, Tick schedule_tick) {
                            processNextWriteBack(block_index, schedule_tick);
                        }, block_index, curTick());
                    DPRINTF(CoalesceEngine, "%s: Pushed processNextWriteBack "
                                    "for input %d to memoryFunctionQueue.\n",
                                                    __func__, block_index);
                    if ((!nextMemoryEvent.pending()) &&
                        (!nextMemoryEvent.scheduled())) {
                        schedule(nextMemoryEvent, nextCycle());
                    }
                } else {
                    DPRINTF(CoalesceEngine, "%s: cacheBlocks[%d] does not need"
                                    " a write back.\n", __func__, block_index);
                    Addr miss_addr = MSHR[block_index].front();
                    Addr aligned_miss_addr =
                        roundDown<Addr, size_t>(miss_addr, peerMemoryAtomSize);
                    DPRINTF(CoalesceEngine, "%s: First conflicting address for"
                        " cacheBlocks[%d] is addr: %lu, aligned_addr: %lu.\n",
                        __func__, block_index, miss_addr, aligned_miss_addr);
                    cacheBlocks[block_index].addr = aligned_miss_addr;
                    cacheBlocks[block_index].valid = false;
                    cacheBlocks[block_index].busyMask = 0;
                    cacheBlocks[block_index].needsWB = false;
                    cacheBlocks[block_index].needsApply = false;
                    cacheBlocks[block_index].pendingData = true;
                    cacheBlocks[block_index].pendingApply = false;
                    cacheBlocks[block_index].pendingWB = false;
                    cacheBlocks[block_index].lastChangedTick = curTick();
                    memoryFunctionQueue.emplace_back(
                        [this] (int block_index, Tick schedule_tick) {
                            processNextRead(block_index, schedule_tick);
                        }, block_index, curTick());
                    DPRINTF(CoalesceEngine, "%s: Pushed processNextRead "
                                    "for input %d to memoryFunctionQueue.\n",
                                                    __func__, block_index);
                    if ((!nextMemoryEvent.pending()) &&
                        (!nextMemoryEvent.scheduled())) {
                        schedule(nextMemoryEvent, nextCycle());
                    }
                }
            } else {
                DPRINTF(CoalesceEngine, "%s: cacheBlocks[%d] is in "
                        "idle state now.\n", __func__, block_index);
            }
        }
    }
    DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                block_index, cacheBlocks[block_index].to_string());

}

void
CoalesceEngine::processNextApplyEvent()
{
    int block_index = applyQueue.front();
    DPRINTF(CoalesceEngine, "%s: Looking at the front of the applyQueue. "
                "cacheBlock[%d] to be applied.\n", __func__, block_index);
    DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n",
            __func__, cacheBlocks[block_index].to_string());
    assert(cacheBlocks[block_index].valid);
    assert(cacheBlocks[block_index].needsApply);
    assert(!cacheBlocks[block_index].pendingData);
    assert(!cacheBlocks[block_index].pendingWB);

    if (cacheBlocks[block_index].pendingApply) {
        assert(cacheBlocks[block_index].busyMask == 0);
        for (int index = 0; index < numElementsPerLine; index++) {
            uint32_t current_prop = cacheBlocks[block_index].items[index].prop;
            uint32_t new_prop = std::min(current_prop,
                            cacheBlocks[block_index].items[index].tempProp);
            if (new_prop != current_prop) {
                cacheBlocks[block_index].items[index].tempProp = new_prop;
                cacheBlocks[block_index].items[index].prop = new_prop;
                DPRINTF(ApplyUpdates, "%s: WorkListItem[%lu][%d]: %s.\n",
                    __func__, cacheBlocks[block_index].addr, index,
                    cacheBlocks[block_index].items[index].to_string());

                int bit_index_base =
                            getBitIndexBase(cacheBlocks[block_index].addr);
                if ((needsPush[bit_index_base + index] == 0) &&
                    (cacheBlocks[block_index].items[index].degree != 0)) {
                    if (peerPushEngine->allocatePushSpace()) {
                        peerPushEngine->recvWLItem(
                            cacheBlocks[block_index].items[index]);
                    } else {
                        needsPush[bit_index_base + index] = 1;
                    }
                }
            }
        }
        cacheBlocks[block_index].needsWB = true;
        cacheBlocks[block_index].needsApply = false;
        cacheBlocks[block_index].pendingApply = false;
        cacheBlocks[block_index].lastChangedTick = curTick();

        assert(MSHR.size() <= numMSHREntries);
        if (MSHR.find(block_index) != MSHR.end()) {
            DPRINTF(CoalesceEngine, "%s: cacheBlocks[%d] has pending "
                                "conflicts.\n", __func__, block_index);
            cacheBlocks[block_index].pendingWB = true;
            cacheBlocks[block_index].lastChangedTick = curTick();
            memoryFunctionQueue.emplace_back(
                [this] (int block_index, Tick schedule_tick) {
                processNextWriteBack(block_index, schedule_tick);
            }, block_index, curTick());
            DPRINTF(CoalesceEngine, "%s: Pushed processNextWriteBack for input"
                    " %d to memoryFunctionQueue.\n", __func__, block_index);
            if ((!nextMemoryEvent.pending()) &&
                (!nextMemoryEvent.scheduled())) {
                schedule(nextMemoryEvent, nextCycle());
            }
        } else {
            DPRINTF(CoalesceEngine, "%s: cacheBlocks[%d] is in "
                    "idle state now.\n", __func__, block_index);
        }
        DPRINTF(CacheBlockState, "%s: cacheBlock[%d]: %s.\n", __func__,
                    block_index, cacheBlocks[block_index].to_string());
    }

    applyQueue.pop_front();
    if ((!applyQueue.empty()) &&
        (!nextApplyEvent.scheduled())) {
        schedule(nextApplyEvent, nextCycle());
    }
}

void
CoalesceEngine::processNextMemoryEvent()
{
    if (memPort.blocked()) {
        nextMemoryEvent.sleep();
        return;
    }

    DPRINTF(CoalesceEngine, "%s: Processing another "
                        "memory function.\n", __func__);
    std::function<void(int, Tick)> next_memory_function;
    int next_memory_function_input;
    Tick next_memory_function_tick;
    std::tie(
        next_memory_function,
        next_memory_function_input,
        next_memory_function_tick) = memoryFunctionQueue.front();
    next_memory_function(next_memory_function_input, next_memory_function_tick);
    memoryFunctionQueue.pop_front();
    DPRINTF(CoalesceEngine, "%s: Popped a function from memoryFunctionQueue. "
                                "memoryFunctionQueue.size = %d.\n", __func__,
                                memoryFunctionQueue.size());

    assert(!nextMemoryEvent.pending());
    assert(!nextMemoryEvent.scheduled());
    if ((!memoryFunctionQueue.empty())) {
        schedule(nextMemoryEvent, nextCycle());
    }
}

void
CoalesceEngine::processNextRead(int block_index, Tick schedule_tick)
{
    DPRINTF(CoalesceEngine, "%s: cacheBlocks[%d] to be filled.\n",
                                            __func__, block_index);
    DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n",
        __func__, block_index, cacheBlocks[block_index].to_string());
    // A cache block should not be touched while it's waiting for data.
    assert(schedule_tick == cacheBlocks[block_index].lastChangedTick);
    //

    assert(!cacheBlocks[block_index].valid);
    assert(cacheBlocks[block_index].busyMask == 0);
    assert(!cacheBlocks[block_index].needsWB);
    assert(!cacheBlocks[block_index].needsApply);
    assert(cacheBlocks[block_index].pendingData);
    assert(!cacheBlocks[block_index].pendingApply);
    assert(!cacheBlocks[block_index].pendingWB);
    PacketPtr pkt = createReadPacket(cacheBlocks[block_index].addr,
                                    peerMemoryAtomSize);
    DPRINTF(CoalesceEngine,  "%s: Created a read packet. addr = %lu, "
            "size = %d.\n", __func__, pkt->getAddr(), pkt->getSize());

    memPort.sendPacket(pkt);
}

void
CoalesceEngine::processNextWriteBack(int block_index, Tick schedule_tick)
{
    DPRINTF(CoalesceEngine, "%s: cacheBlocks[%d] to be written back.\n",
                                                __func__, block_index);
    DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                block_index, cacheBlocks[block_index].to_string());
    if (schedule_tick == cacheBlocks[block_index].lastChangedTick) {
        assert(cacheBlocks[block_index].valid);
        assert(cacheBlocks[block_index].busyMask == 0);
        assert(cacheBlocks[block_index].needsWB);
        assert(!cacheBlocks[block_index].needsApply);
        assert(!cacheBlocks[block_index].pendingData);
        assert(!cacheBlocks[block_index].pendingApply);
        assert(cacheBlocks[block_index].pendingWB);

        // Why would we write it back if it does not have a conflict.
        assert(MSHR.size() <= numMSHREntries);
        assert(MSHR.find(block_index) != MSHR.end());

        PacketPtr pkt = createWritePacket(
                cacheBlocks[block_index].addr, peerMemoryAtomSize,
                (uint8_t*) cacheBlocks[block_index].items);
        DPRINTF(CoalesceEngine,  "%s: Created a write packet to "
                        "Addr: %lu, size = %d.\n", __func__,
                        pkt->getAddr(), pkt->getSize());
        memPort.sendPacket(pkt);
        cacheBlocks[block_index].needsWB = false;
        cacheBlocks[block_index].pendingWB = false;

        Addr miss_addr = MSHR[block_index].front();
        Addr aligned_miss_addr =
                        roundDown<Addr, size_t>(miss_addr, peerMemoryAtomSize);
        DPRINTF(CoalesceEngine, "%s: First conflicting address for"
                    " cacheBlocks[%d] is addr: %lu, aligned_addr: %lu.\n",
                    __func__, block_index, miss_addr, aligned_miss_addr);

        cacheBlocks[block_index].addr = aligned_miss_addr;
        cacheBlocks[block_index].valid = false;
        cacheBlocks[block_index].busyMask = 0;
        cacheBlocks[block_index].needsWB = false;
        cacheBlocks[block_index].needsApply = false;
        cacheBlocks[block_index].pendingData = true;
        cacheBlocks[block_index].pendingApply = false;
        cacheBlocks[block_index].pendingWB = false;
        cacheBlocks[block_index].lastChangedTick = curTick();
        memoryFunctionQueue.emplace_back(
            [this] (int block_index, Tick schedule_tick) {
            processNextRead(block_index, schedule_tick);
        }, block_index, curTick());
        DPRINTF(CoalesceEngine, "%s: Pushed processNextRead for input"
                " %d to memoryFunctionQueue.\n", __func__, block_index);
        DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                    block_index, cacheBlocks[block_index].to_string());
    } else {
        DPRINTF(CoalesceEngine, "%s: cacheBlocks[%d] has been touched since a "
                            "write back has been scheduled for it. Ignoring "
                            "the current write back scheduled at tick %lu for "
                            "the right function scheduled later.\n",
                            __func__, block_index, schedule_tick);
    }
}

std::tuple<bool, int>
CoalesceEngine::getOptimalBitVectorSlice()
{
    bool hit_in_cache = false;
    int slice_base = -1;

    // int score = 0;
    // int max_score_possible = 3 * numElementsPerLine;
    for (int it = 0; it < MAX_BITVECTOR_SIZE; it += numElementsPerLine) {
        // int current_score = 0;
        uint32_t current_popcount = 0;
        for (int i = 0; i < numElementsPerLine; i++) {
            current_popcount += needsPush[it + i];
        }
        if (current_popcount == 0) {
            continue;
        }
        // current_score += current_popcount;
        Addr addr = getBlockAddrFromBitIndex(it);
        int block_index = getBlockIndex(addr);
        // Idle state: valid && !pendingApply && !pendingWB
        if ((cacheBlocks[block_index].addr == addr) &&
            (cacheBlocks[block_index].valid) &&
            (cacheBlocks[block_index].busyMask == 0) &&
            (!cacheBlocks[block_index].pendingApply) &&
            (!cacheBlocks[block_index].pendingWB)) {
            assert(!cacheBlocks[block_index].needsApply);
            assert(!cacheBlocks[block_index].pendingData);
            // current_score += numElementsPerLine * 2;
            // if (current_score > score) {
            //     score = current_score;
            //     slice_base = it;
            //     hit_in_cache = true;
            //     if (score == max_score_possible) {
            //         break;
            //     }
            // }
            return std::make_tuple(true, it);
        } else if (cacheBlocks[block_index].addr != addr) {
            // score += numElementsPerLine;
            // if (current_score > score) {
            //     score = current_score;
            //     slice_base = it;
            //     hit_in_cache = false;
            //     assert(score < max_score_possible);
            // }
            return std::make_tuple(false, it);
        }
    }

    return std::make_tuple(hit_in_cache, slice_base);
}

void
CoalesceEngine::processNextPushRetry(int prev_slice_base, Tick schedule_tick)
{
    bool hit_in_cache;
    int slice_base;
    std::tie(hit_in_cache, slice_base) = getOptimalBitVectorSlice();

    if (slice_base != -1) {
        Addr addr = getBlockAddrFromBitIndex(slice_base);
        int block_index = getBlockIndex(addr);
        if (hit_in_cache) {
            assert(cacheBlocks[block_index].valid);
            assert(cacheBlocks[block_index].busyMask == 0);

            int push_needed = 0;
            DPRINTF(BitVector, "%s: needsPush.count: %d.\n",
                                    __func__, needsPush.count());
            assert(peerPushEngine->getNumRetries() == needsPush.count());

            for (int i = 0; i < numElementsPerLine; i++) {
                if (needsPush[slice_base + i] == 1) {
                    peerPushEngine->recvWLItemRetry(
                        cacheBlocks[block_index].items[i]);
                }
                push_needed +=  needsPush[slice_base + i];
                needsPush[slice_base + i] = 0;
            }
            DPRINTF(BitVector, "%s: needsPush.count: %d.\n",
                                    __func__, needsPush.count());
            peerPushEngine->deallocatePushSpace(
                                            numElementsPerLine - push_needed);
            assert(peerPushEngine->getNumRetries() == needsPush.count());
        } else {
            PacketPtr pkt = createReadPacket(addr, peerMemoryAtomSize);
            SenderState* sender_state = new SenderState(true);
            pkt->pushSenderState(sender_state);
            memPort.sendPacket(pkt);
            // TODO: Set a tracking structure so that nextMemoryReadEvent knows
            // It does not have to read this address anymore. It can simply set
            // a flag to true (maybe not even needed just look if the cache has a
            // line allocated for it in the cacheBlocks).
        }
        numRetriesReceived--;
        assert(numRetriesReceived == 0);
    }

    if (numRetriesReceived > 0) {
        memoryFunctionQueue.emplace_back(
            [this] (int slice_base, Tick schedule_tick) {
            processNextPushRetry(slice_base, schedule_tick);
        }, 0, curTick());
        DPRINTF(CoalesceEngine, "%s: Pushed processNextPushRetry with input "
                                    "0 to memoryFunctionQueue.\n", __func__);
    }
}

void
CoalesceEngine::recvMemRetry()
{
    DPRINTF(CoalesceEngine, "%s: Received a MemRetry.\n", __func__);

    if (!nextMemoryEvent.pending()) {
        DPRINTF(CoalesceEngine, "%s: Not pending MemRerty.\n", __func__);
        return;
    }
    assert(!nextMemoryEvent.scheduled());
    nextMemoryEvent.wake();
    schedule(nextMemoryEvent, nextCycle());
}

void
CoalesceEngine::recvPushRetry()
{
    numRetriesReceived++;
    DPRINTF(CoalesceEngine,  "%s: Received a push retry.\n", __func__);
    // For now since we do only one retry at a time, we should not receive
    // a retry while this nextSendingRetryEvent is scheduled or is pending.
    assert(numRetriesReceived == 1);

    // TODO: Pass slice_base to getOptimalBitVectorSlice
    memoryFunctionQueue.emplace_back(
        [this] (int slice_base, Tick schedule_tick) {
        processNextPushRetry(slice_base, schedule_tick);
    }, 0, curTick());
    DPRINTF(CoalesceEngine, "%s: Pushed processNextPushRetry with input 0 to "
                                        "memoryFunctionQueue.\n", __func__);
    if ((!nextMemoryEvent.pending()) &&
        (!nextMemoryEvent.scheduled())) {
        schedule(nextMemoryEvent, nextCycle());
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
