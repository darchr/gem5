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

#include "accl/graph/sega/mpu.hh"
#include "base/intmath.hh"
#include "debug/CacheBlockState.hh"
#include "debug/CoalesceEngine.hh"
#include "debug/MSDebug.hh"
#include "debug/SEGAStructureSize.hh"
#include "mem/packet_access.hh"
#include "sim/sim_exit.hh"

namespace gem5
{

CoalesceEngine::CoalesceEngine(const Params &params):
    BaseMemoryEngine(params), lastAtomAddr(0),
    numLines((int) (params.cache_size / peerMemoryAtomSize)),
    numElementsPerLine((int) (peerMemoryAtomSize / sizeof(WorkListItem))),
    onTheFlyReqs(0),
    maxRespPerCycle(params.max_resp_per_cycle),
    pullsReceived(0), pullsScheduled(0), pendingPullReads(0),
    activeBufferSize(params.active_buffer_size),
    postPushWBQueueSize(params.post_push_wb_queue_size),
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
    activeBuffer.clear();
    postPushWBQueue.clear();
}

void
CoalesceEngine::registerMPU(MPU* mpu)
{
    owner = mpu;
}

void
CoalesceEngine::recvFunctional(PacketPtr pkt)
{
    if (pkt->isRead()) {
        assert(pkt->getSize() == peerMemoryAtomSize);
        Addr addr = pkt->getAddr();
        int block_index = getBlockIndex(addr);

        // FIXME: Check postPushWBQueue for hits
        // Is it really the case though. I don't think at this time
        // beacuse we check done after handleMemResp and make sure all
        // the writes to memory are done before scheduling an exit event
        if ((cacheBlocks[block_index].addr == addr) &&
            (cacheBlocks[block_index].valid)) {
            assert(cacheBlocks[block_index].state == CacheState::IDLE);

            pkt->makeResponse();
            pkt->setDataFromBlock(
                (uint8_t*) cacheBlocks[block_index].items, peerMemoryAtomSize);
        } else {
            memPort.sendFunctional(pkt);
        }
    } else {
        graphWorkload->init(pkt, directory);
        if (pkt->getAddr() > lastAtomAddr) {
            lastAtomAddr = pkt->getAddr();
        }
        memPort.sendFunctional(pkt);
    }
}

void
CoalesceEngine::postMemInitSetup()
{
    directory->setLastAtomAddr(lastAtomAddr);
}

void
CoalesceEngine::createPopCountDirectory(int atoms_per_block)
{
    directory = new PopCountDirectory(
                        peerMemoryRange, atoms_per_block, peerMemoryAtomSize);
}

bool
CoalesceEngine::done()
{
    return memoryFunctionQueue.empty() && activeCacheBlocks.empty() &&
        activeBuffer.empty() && directory->empty() && (onTheFlyReqs == 0);
}

bool
CoalesceEngine::timeToPull()
{
    return (activeBuffer.size() + pendingPullReads) < activeBufferSize;
}

bool
CoalesceEngine::canSchedulePull()
{
    // TODO: Maybe a good idea to change this to
    // activeBuffer.size() + pendingPullReads + pullsScheduled < activeBufferSize
    return pullsScheduled < 1;
}

bool
CoalesceEngine::workLeftInMem()
{
    return !directory->empty();
}

bool
CoalesceEngine::pullCondition()
{
    return ((activeBuffer.size() + pendingPullReads + pullsScheduled) < activeBufferSize);
}

// addr should be aligned to peerMemoryAtomSize
int
CoalesceEngine::getBlockIndex(Addr addr)
{
    assert((addr % peerMemoryAtomSize) == 0);
    Addr trimmed_addr = peerMemoryRange.removeIntlvBits(addr);
    return ((int) (trimmed_addr / peerMemoryAtomSize)) % numLines;
}

ReadReturnStatus
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
        // Hit
        DPRINTF(CoalesceEngine,  "%s: Addr: %lu is a hit.\n", __func__, addr);
        stats.readHits++;
        assert(cacheBlocks[block_index].state != CacheState::INVALID);
        responseQueue.push_back(std::make_tuple(
            addr, cacheBlocks[block_index].items[wl_offset], curTick()));

        DPRINTF(SEGAStructureSize, "%s: Added (addr: %lu, wl: %s) "
                "to responseQueue. responseQueue.size = %d.\n",
                __func__, addr,
                graphWorkload->printWorkListItem(
                        cacheBlocks[block_index].items[wl_offset]),
                responseQueue.size());
        DPRINTF(CoalesceEngine, "%s: Added (addr: %lu, wl: %s) "
                "to responseQueue. responseQueue.size = %d.\n",
                __func__, addr,
                graphWorkload->printWorkListItem(
                    cacheBlocks[block_index].items[wl_offset]),
                responseQueue.size());
        // TODO: Stat to count the number of WLItems that have been touched.
        cacheBlocks[block_index].busyMask |= (1 << wl_offset);
        cacheBlocks[block_index].state = CacheState::BUSY;
        // HACK: If a read happens on the same cycle as another operation such
        // as apply set lastChangedTick to half a cycle later so that operation
        // scheduled by the original operation (apply in this example) are
        // invalidated. For more details refer to "accl/graph/sega/busyMaskErr"
        cacheBlocks[block_index].lastChangedTick =
                                    curTick() + (Tick) (clockPeriod() / 2);
        DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                    block_index, cacheBlocks[block_index].to_string());

        if (!nextResponseEvent.scheduled()) {
            schedule(nextResponseEvent, nextCycle());
        }
        stats.numVertexReads++;
        return ReadReturnStatus::ACCEPT;
    } else if ((cacheBlocks[block_index].addr == aligned_addr) &&
                (cacheBlocks[block_index].state == CacheState::PENDING_DATA)) {
        // Hit under miss
        DPRINTF(CoalesceEngine,  "%s: Addr: %lu is a hit under miss.\n",
                                                        __func__, addr);
        stats.readHitUnderMisses++;
        assert(!cacheBlocks[block_index].valid);
        assert(cacheBlocks[block_index].busyMask == 0);
        assert(!cacheBlocks[block_index].dirty);

        assert(MSHR.find(block_index) != MSHR.end());
        MSHR[block_index].push_back(addr);
        DPRINTF(CoalesceEngine,  "%s: Added Addr: %lu to MSHR "
                "for cacheBlocks[%d].\n", __func__, addr, block_index);
        DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                    block_index, cacheBlocks[block_index].to_string());
        stats.numVertexReads++;
        return ReadReturnStatus::ACCEPT;
    } else {
        // miss
        assert(cacheBlocks[block_index].addr != aligned_addr);
        DPRINTF(CoalesceEngine,  "%s: Addr: %lu is a miss.\n", __func__, addr);
        stats.readMisses++;
        if (cacheBlocks[block_index].state != CacheState::INVALID) {
            // conflict miss
            DPRINTF(CoalesceEngine, "%s: Addr: %lu has conflict with "
                "Addr: %lu.\n", __func__, addr, cacheBlocks[block_index].addr);
            cacheBlocks[block_index].hasConflict = true;
            if (cacheBlocks[block_index].state == CacheState::IDLE) {
                if (cacheBlocks[block_index].dirty) {
                    cacheBlocks[block_index].state = CacheState::PENDING_WB;
                    cacheBlocks[block_index].lastChangedTick = curTick();
                    memoryFunctionQueue.emplace_back(
                        [this] (int block_index, Tick schedule_tick) {
                            processNextWriteBack(block_index, schedule_tick);
                        }, block_index, curTick());
                    if ((!nextMemoryEvent.pending()) &&
                        (!nextMemoryEvent.scheduled())) {
                        schedule(nextMemoryEvent, nextCycle());
                    }
                } else {
                    // NOTE: The cache block could still be active but
                    // not dirty. If active we only have to active tracking
                    // but can throw the data away.
                    bool atom_active = false;
                    for (int index = 0; index < numElementsPerLine; index++) {
                        atom_active |= graphWorkload->activeCondition(
                                        cacheBlocks[block_index].items[index]);
                    }
                    if (atom_active) {
                        activeCacheBlocks.erase(block_index);
                        int count = directory->activate(cacheBlocks[block_index].addr);
                        stats.blockActiveCount.sample(count);
                        stats.frontierSize.sample(directory->workCount());
                    }
                    // NOTE: Bring the cache line to invalid state.
                    // NOTE: Above line where we set hasConflict to true
                    // does not matter anymore since we reset the cache line.
                    cacheBlocks[block_index].reset();
                }
                return ReadReturnStatus::REJECT_NO_ROLL;
            } else {
                stats.numConflicts++;
                return ReadReturnStatus::REJECT_ROLL;
            }
        } else {
            // cold miss
            assert(MSHR.find(block_index) == MSHR.end());
            cacheBlocks[block_index].addr = aligned_addr;
            cacheBlocks[block_index].busyMask = 0;
            cacheBlocks[block_index].valid = false;
            cacheBlocks[block_index].dirty = false;
            cacheBlocks[block_index].hasConflict = false;
            cacheBlocks[block_index].state = CacheState::PENDING_DATA;
            cacheBlocks[block_index].lastChangedTick = curTick();

            MSHR[block_index].push_back(addr);
            memoryFunctionQueue.emplace_back(
                [this] (int block_index, Tick schedule_tick) {
                    processNextRead(block_index, schedule_tick);
                }, block_index, curTick());
            if ((!nextMemoryEvent.pending()) &&
                (!nextMemoryEvent.scheduled())) {
                schedule(nextMemoryEvent, nextCycle());
            }
            return ReadReturnStatus::ACCEPT;
        }
    }
}

bool
CoalesceEngine::handleMemResp(PacketPtr pkt)
{
    assert(pkt->isResponse());
    DPRINTF(CoalesceEngine,  "%s: Received packet: %s from memory.\n",
                                                __func__, pkt->print());

    onTheFlyReqs--;
    if (pkt->isWrite()) {
        DPRINTF(CoalesceEngine, "%s: Dropped the write response.\n", __func__);
        delete pkt;
    } else {
        assert(pkt->isRead());
        Addr addr = pkt->getAddr();
        int block_index = getBlockIndex(addr);
        ReadPurpose* purpose = pkt->findNextSenderState<ReadPurpose>();

        // NOTE: Regardless of where the pkt will go we have to release the
        // reserved space for this pkt in the activeBuffer in case
        // it was read from memory for placement in the activeBuffer.
        // NOTE: Also we have to stop tracking the address for pullAddrs
        if (purpose->dest() == ReadDestination::READ_FOR_PUSH) {
            pendingPullReads--;
            pendingPullAddrs.erase(addr);
        }
        if (cacheBlocks[block_index].addr == addr) {
            // If it is in the cache, line should be in PENDING_DATA state.
            // Regardless of the purpose for which it was read, it should
            // be placed in the cache array.
            assert(cacheBlocks[block_index].busyMask == 0);
            assert(!cacheBlocks[block_index].valid);
            assert(!cacheBlocks[block_index].dirty);
            assert(cacheBlocks[block_index].state == CacheState::PENDING_DATA);

            // NOTE: Since it is in PENDING_DATA state it
            // should have an entry in the MSHR.
            assert(MSHR.find(block_index) != MSHR.end());

            pkt->writeDataToBlock((uint8_t*) cacheBlocks[block_index].items,
                                                            peerMemoryAtomSize);

            cacheBlocks[block_index].valid = true;
            // HACK: In case the pkt was read for push but it was allocated
            // for in the cache later on, we should cancel the future
            // processNextRead for this block. We could set lastChangedTick
            // to curTick() like usual. However, there is no way to ensure
            // that processNextRead will be not be called on the same tick
            // as the pkt arrives from the memory. Therefore, we will set
            // the lastChangedTick to half a cycle before the actual time.
            // We move that back in time because it would be fine if
            // processNextRead happened before pkt arriveed. processNextRead
            // actually will check if there is a pending read for push for
            // the address it's trying to populate.
            if (purpose->dest() == ReadDestination::READ_FOR_PUSH) {
                cacheBlocks[block_index].lastChangedTick =
                                    curTick() - (Tick) (clockPeriod() / 2);
            } else {
                cacheBlocks[block_index].lastChangedTick = curTick();
            }

            // NOTE: If the atom is active we have to deactivate the tracking
            // of this atom in the memory since it's not in memory anymore.
            // Since it is going to the cache, cache will be responsible for
            // tracking this. Push to activeCacheBlocks for simulator speed
            // instead of having to search for active blocks in the cache.
            bool atom_active = false;
            for (int index = 0; index < numElementsPerLine; index++) {
                atom_active |= graphWorkload->activeCondition(
                                            cacheBlocks[block_index].items[index]);
            }
            if (atom_active) {
                int count = directory->deactivate(addr);
                activeCacheBlocks.push_back(block_index);
                stats.blockActiveCount.sample(count);
                stats.frontierSize.sample(directory->workCount());
            }

            assert(MSHR.find(block_index) != MSHR.end());
            for (auto it = MSHR[block_index].begin();
                                            it != MSHR[block_index].end();) {
                Addr miss_addr = *it;
                Addr aligned_miss_addr =
                            roundDown<Addr, size_t>(miss_addr, peerMemoryAtomSize);

                assert(aligned_miss_addr == cacheBlocks[block_index].addr);
                int wl_offset = (miss_addr - aligned_miss_addr) / sizeof(WorkListItem);
                DPRINTF(CoalesceEngine,  "%s: Addr: %lu in the MSHR for "
                            "cacheBlocks[%d] can be serviced with the received "
                            "packet.\n",__func__, miss_addr, block_index);
                responseQueue.push_back(std::make_tuple(miss_addr,
                        cacheBlocks[block_index].items[wl_offset], curTick()));
                DPRINTF(SEGAStructureSize, "%s: Added (addr: %lu, wl: %s) "
                            "to responseQueue. responseQueue.size = %d.\n",
                            __func__, miss_addr,
                            graphWorkload->printWorkListItem(
                                cacheBlocks[block_index].items[wl_offset]),
                            responseQueue.size());
                DPRINTF(CoalesceEngine, "%s: Added (addr: %lu, wl: %s) "
                            "to responseQueue. responseQueue.size = %d.\n",
                            __func__, addr,
                            graphWorkload->printWorkListItem(
                                cacheBlocks[block_index].items[wl_offset]),
                            responseQueue.size());
                cacheBlocks[block_index].busyMask |= (1 << wl_offset);
                DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                            block_index, cacheBlocks[block_index].to_string());
                it = MSHR[block_index].erase(it);
            }
            MSHR.erase(block_index);

            cacheBlocks[block_index].state = CacheState::BUSY;
            if ((!nextResponseEvent.scheduled()) && (!responseQueue.empty())) {
                schedule(nextResponseEvent, nextCycle());
            }
            delete pkt;
        } else {
            assert(purpose->dest() == ReadDestination::READ_FOR_PUSH);
            // There should be enough room in activeBuffer to place this pkt.
            // REMEMBER: If dest == READ_FOR_PUSH we release the reserved space.
            // So at this point in code we should have at least one free entry
            // in the active buffer which is reserved for this pkt.
            assert(activeBuffer.size() + pendingPullReads < activeBufferSize);

            WorkListItem items[numElementsPerLine];
            pkt->writeDataToBlock((uint8_t*) items, peerMemoryAtomSize);
            bool atom_active = false;
            for (int index = 0; index < numElementsPerLine; index++) {
                atom_active |= graphWorkload->activeCondition(items[index]);
            }
            if (atom_active) {
                int count = directory->deactivate(addr);
                activeBuffer.emplace_back(pkt, curTick());
                stats.blockActiveCount.sample(count);
                stats.frontierSize.sample(directory->workCount());
            } else {
                delete pkt;
            }

            if (pullCondition()) {
                memoryFunctionQueue.emplace_back(
                    [this] (int ignore, Tick schedule_tick) {
                        processNextVertexPull(ignore, schedule_tick);
                    }, 0, curTick());
                if ((!nextMemoryEvent.pending()) &&
                    (!nextMemoryEvent.scheduled())) {
                    schedule(nextMemoryEvent, nextCycle());
                }
                pullsScheduled++;
            }
        }
    }

    if (done()) {
        owner->recvDoneSignal();
    }
    return true;
}

void
CoalesceEngine::processNextResponseEvent()
{
    int num_responses_sent = 0;

    Addr addr_response;
    WorkListItem worklist_response;
    Tick response_queueing_tick;
    while(true) {
        std::tie(addr_response, worklist_response, response_queueing_tick) =
                                                        responseQueue.front();
        Tick waiting_ticks = curTick() - response_queueing_tick;
        if (ticksToCycles(waiting_ticks) < 1) {
            break;
        }
        owner->handleIncomingWL(addr_response, worklist_response);
        num_responses_sent++;
        DPRINTF(CoalesceEngine,
                    "%s: Sent WorkListItem: %s with addr: %lu to WLEngine.\n",
                    __func__,
                    graphWorkload->printWorkListItem(worklist_response),
                    addr_response);

        responseQueue.pop_front();
        DPRINTF(SEGAStructureSize,  "%s: Popped a response from responseQueue."
                    " responseQueue.size = %d.\n", __func__,
                    responseQueue.size());
        DPRINTF(CoalesceEngine,  "%s: Popped a response from responseQueue. "
                    "responseQueue.size = %d.\n", __func__,
                    responseQueue.size());
        stats.responseQueueLatency.sample(
                                    waiting_ticks * 1e9 / getClockFrequency());
        if (num_responses_sent >= maxRespPerCycle) {
            if (!responseQueue.empty()) {
                stats.responsePortShortage++;
            }
            break;
        }
        if (responseQueue.empty()) {
            break;
        }
    }

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
                        __func__, addr, graphWorkload->printWorkListItem(wl),
                        block_index, aligned_addr, wl_offset);
    DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                block_index, cacheBlocks[block_index].to_string());
    DPRINTF(CoalesceEngine,  "%s: Received a write for WorkListItem: %s "
                "with Addr: %lu.\n", __func__,
                graphWorkload->printWorkListItem(wl), addr);

    // NOTE: Design does not allow for write misses.
    assert(cacheBlocks[block_index].addr == aligned_addr);
    // cache state asserts
    assert(cacheBlocks[block_index].busyMask != 0);
    assert(cacheBlocks[block_index].valid);
    assert(cacheBlocks[block_index].state == CacheState::BUSY);

    // respective bit in busyMask for wl is set.
    assert((cacheBlocks[block_index].busyMask & (1 << wl_offset)) ==
            (1 << wl_offset));

    if (wl.tempProp != cacheBlocks[block_index].items[wl_offset].tempProp) {
        cacheBlocks[block_index].dirty |= true;
    }
    cacheBlocks[block_index].items[wl_offset] = wl;
    if ((graphWorkload->activeCondition(cacheBlocks[block_index].items[wl_offset])) &&
        (!activeCacheBlocks.find(block_index))) {
        activeCacheBlocks.push_back(block_index);
        if (!owner->running()) {
            owner->start();
        }
    }

    cacheBlocks[block_index].busyMask &= ~(1 << wl_offset);
    cacheBlocks[block_index].lastChangedTick = curTick();
    DPRINTF(CoalesceEngine,  "%s: Wrote to cacheBlocks[%d][%d] = %s.\n",
                __func__, block_index, wl_offset,
                graphWorkload->printWorkListItem(
                    cacheBlocks[block_index].items[wl_offset]));
    DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                        block_index, cacheBlocks[block_index].to_string());

    if (cacheBlocks[block_index].busyMask == 0) {
        if (cacheBlocks[block_index].hasConflict) {
            if (cacheBlocks[block_index].dirty) {
                cacheBlocks[block_index].state = CacheState::PENDING_WB;
                cacheBlocks[block_index].lastChangedTick = curTick();
                memoryFunctionQueue.emplace_back(
                    [this] (int block_index, Tick schedule_tick) {
                        processNextWriteBack(block_index, schedule_tick);
                    }, block_index, curTick());
                if ((!nextMemoryEvent.pending()) &&
                    (!nextMemoryEvent.scheduled())) {
                    schedule(nextMemoryEvent, nextCycle());
                }
            } else {
                bool atom_active = false;
                for (int index = 0; index < numElementsPerLine; index++) {
                    atom_active |= graphWorkload->activeCondition(
                                        cacheBlocks[block_index].items[index]);
                }
                if (atom_active) {
                    activeCacheBlocks.erase(block_index);
                    int count = directory->activate(cacheBlocks[block_index].addr);
                    stats.blockActiveCount.sample(count);
                    stats.frontierSize.sample(directory->workCount());
                }
                cacheBlocks[block_index].reset();
            }
        } else {
            cacheBlocks[block_index].state = CacheState::IDLE;
            cacheBlocks[block_index].lastChangedTick = curTick();
        }
    }
    DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                block_index, cacheBlocks[block_index].to_string());
    stats.numVertexWrites++;
    if ((cacheBlocks[block_index].state == CacheState::IDLE) && done()) {
        owner->recvDoneSignal();
    }
}

void
CoalesceEngine::processNextMemoryEvent()
{
    if (memPort.blocked()) {
        stats.numMemoryBlocks++;
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
    stats.memoryFunctionLatency.sample((curTick() - next_memory_function_tick)
                                                * 1e9 / getClockFrequency());
    DPRINTF(CoalesceEngine, "%s: Popped a function from memoryFunctionQueue. "
                                "memoryFunctionQueue.size = %d.\n", __func__,
                                memoryFunctionQueue.size());

    assert(!nextMemoryEvent.pending());
    assert(!nextMemoryEvent.scheduled());
    if ((!memoryFunctionQueue.empty())) {
        schedule(nextMemoryEvent, nextCycle());
    }

    if (done()) {
        owner->recvDoneSignal();
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
    // assert(schedule_tick == cacheBlocks[block_index].lastChangedTick);
    // TODO: Figure out if this is still necessary.
    if (cacheBlocks[block_index].lastChangedTick != schedule_tick) {
        return;
    }

    assert(cacheBlocks[block_index].busyMask == 0);
    assert(!cacheBlocks[block_index].valid);
    assert(!cacheBlocks[block_index].dirty);
    assert(cacheBlocks[block_index].state == CacheState::PENDING_DATA);

    bool need_send_pkt = true;

    // NOTE: Search postPushWBQueue
    for (auto wb = postPushWBQueue.begin(); wb != postPushWBQueue.end();)
    {
        PacketPtr wb_pkt = std::get<0>(*wb);
        if (cacheBlocks[block_index].addr == wb_pkt->getAddr()) {
            wb_pkt->writeDataToBlock(
                (uint8_t*) cacheBlocks[block_index].items, peerMemoryAtomSize);
            cacheBlocks[block_index].valid = true;
            cacheBlocks[block_index].dirty = true;
            cacheBlocks[block_index].lastChangedTick = curTick();

            need_send_pkt = false;
            wb = postPushWBQueue.erase(wb);
            delete wb_pkt;
        } else {
            wb++;
        }
    }
    // NOTE: Search activeBuffer
    for (auto ab = activeBuffer.begin(); ab != activeBuffer.end();) {
        PacketPtr ab_pkt = std::get<0>(*ab);
        if (cacheBlocks[block_index].addr == ab_pkt->getAddr()) {
            ab_pkt->writeDataToBlock(
                (uint8_t*) cacheBlocks[block_index].items, peerMemoryAtomSize);

            cacheBlocks[block_index].valid = true;
            cacheBlocks[block_index].dirty = true;
            cacheBlocks[block_index].lastChangedTick = curTick();
            activeCacheBlocks.push_back(block_index);

            need_send_pkt = false;
            ab = activeBuffer.erase(ab);
            delete ab_pkt;
            if (pullCondition()) {
                memoryFunctionQueue.emplace_back(
                    [this] (int ignore, Tick schedule_tick) {
                        processNextVertexPull(ignore, schedule_tick);
                    }, 0, curTick());
                pullsScheduled++;
            }
        } else {
            ab++;
        }
    }
    if (!need_send_pkt) {
        for (auto it = MSHR[block_index].begin(); it != MSHR[block_index].end();) {
            Addr miss_addr = *it;
            Addr aligned_miss_addr =
                roundDown<Addr, size_t>(miss_addr, peerMemoryAtomSize);
            assert(aligned_miss_addr == cacheBlocks[block_index].addr);
            int wl_offset = (miss_addr - aligned_miss_addr) / sizeof(WorkListItem);
            DPRINTF(CoalesceEngine,  "%s: Addr: %lu in the MSHR for "
                        "cacheBlocks[%d] can be serviced with the received "
                        "packet.\n",__func__, miss_addr, block_index);
            // TODO: Make this block of code into a function
            responseQueue.push_back(std::make_tuple(miss_addr,
                    cacheBlocks[block_index].items[wl_offset], curTick()));
            DPRINTF(SEGAStructureSize, "%s: Added (addr: %lu, wl: %s) "
                        "to responseQueue. responseQueue.size = %d.\n",
                        __func__, miss_addr,
                        graphWorkload->printWorkListItem(
                            cacheBlocks[block_index].items[wl_offset]),
                        responseQueue.size());
            DPRINTF(CoalesceEngine, "%s: Added (addr: %lu, wl: %s) "
                        "to responseQueue. responseQueue.size = %d.\n",
                        __func__, miss_addr,
                        graphWorkload->printWorkListItem(
                            cacheBlocks[block_index].items[wl_offset]),
                        responseQueue.size());
            cacheBlocks[block_index].busyMask |= (1 << wl_offset);
            DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n",
                    __func__, block_index,
                    cacheBlocks[block_index].to_string());
            it = MSHR[block_index].erase(it);
        }
        assert(MSHR[block_index].empty());
        MSHR.erase(block_index);
        if ((!nextResponseEvent.scheduled()) &&
            (!responseQueue.empty())) {
            schedule(nextResponseEvent, nextCycle());
        }
        cacheBlocks[block_index].state = CacheState::BUSY;
    }

    if (pendingPullAddrs.find(cacheBlocks[block_index].addr) !=
                                            pendingPullAddrs.end()) {
        need_send_pkt = false;
    }

    if (need_send_pkt) {
        PacketPtr pkt = createReadPacket(cacheBlocks[block_index].addr,
                                        peerMemoryAtomSize);
        ReadPurpose* purpose = new ReadPurpose(ReadDestination::READ_FOR_CACHE);
        pkt->pushSenderState(purpose);
        DPRINTF(CoalesceEngine,  "%s: Created a read packet. addr = %lu, "
                "size = %d.\n", __func__, pkt->getAddr(), pkt->getSize());
        memPort.sendPacket(pkt);
        onTheFlyReqs++;
    }
}

void
CoalesceEngine::processNextWriteBack(int block_index, Tick schedule_tick)
{
    DPRINTF(CoalesceEngine, "%s: cacheBlocks[%d] to be written back.\n",
                                                __func__, block_index);
    DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                block_index, cacheBlocks[block_index].to_string());

    if (schedule_tick == cacheBlocks[block_index].lastChangedTick) {
        assert(cacheBlocks[block_index].busyMask == 0);
        assert(cacheBlocks[block_index].valid);
        assert(cacheBlocks[block_index].dirty);
        assert(cacheBlocks[block_index].hasConflict);
        assert(cacheBlocks[block_index].state == CacheState::PENDING_WB);

        // NOTE: If the atom we're writing back is active, we have to
        // stop tracking it in the cache and start tracking it in the memory.
        bool atom_active = false;
        for (int index = 0; index < numElementsPerLine; index++) {
            atom_active |= graphWorkload->activeCondition(
                                        cacheBlocks[block_index].items[index]);
        }
        if (atom_active) {
            activeCacheBlocks.erase(block_index);
            int count = directory->activate(cacheBlocks[block_index].addr);
            stats.blockActiveCount.sample(count);
            stats.frontierSize.sample(directory->workCount());
        }

        PacketPtr pkt = createWritePacket(
                cacheBlocks[block_index].addr, peerMemoryAtomSize,
                (uint8_t*) cacheBlocks[block_index].items);
        DPRINTF(CoalesceEngine,  "%s: Created a write packet to "
                        "Addr: %lu, size = %d.\n", __func__,
                        pkt->getAddr(), pkt->getSize());
        memPort.sendPacket(pkt);
        onTheFlyReqs++;
        cacheBlocks[block_index].reset();
        DPRINTF(CacheBlockState, "%s: cacheBlocks[%d]: %s.\n", __func__,
                    block_index, cacheBlocks[block_index].to_string());
    } else {
        DPRINTF(CoalesceEngine, "%s: cacheBlocks[%d] has been touched since a "
                            "write back has been scheduled for it. Ignoring "
                            "the current write back scheduled at tick %lu for "
                            "the right function scheduled later.\n",
                            __func__, block_index, schedule_tick);
        stats.numInvalidWriteBacks++;
    }
}

void
CoalesceEngine::processNextPostPushWB(int ignore, Tick schedule_tick)
{
    if (postPushWBQueue.empty()) {
        return;
    }

    PacketPtr wb_pkt;
    Tick pkt_tick;
    std::tie(wb_pkt, pkt_tick) = postPushWBQueue.front();
    if (schedule_tick == pkt_tick) {
        memPort.sendPacket(wb_pkt);
        onTheFlyReqs++;
        postPushWBQueue.pop_front();
    }
}

void
CoalesceEngine::processNextVertexPull(int ignore, Tick schedule_tick)
{
    pullsScheduled--;
    if (!directory->empty()) {
        Addr addr = directory->getNextWork();
        int block_index = getBlockIndex(addr);

        bool in_cache = cacheBlocks[block_index].addr == addr;
        bool in_active_buffer = false;
        for (auto ab = activeBuffer.begin(); ab != activeBuffer.end(); ab++) {
            PacketPtr pkt = std::get<0>(*ab);
            in_active_buffer |= (pkt->getAddr() == addr);
        }
        bool in_write_buffer = false;
        for (auto wb = postPushWBQueue.begin(); wb != postPushWBQueue.end(); wb++)
        {
            PacketPtr pkt = std::get<0>(*wb);
            in_write_buffer |= (pkt->getAddr() == addr);
        }
        bool repeat_work = pendingPullAddrs.find(addr) != pendingPullAddrs.end();

        if (!in_cache && !in_active_buffer && !in_write_buffer && !repeat_work) {
            PacketPtr pkt = createReadPacket(addr, peerMemoryAtomSize);
            ReadPurpose* purpose = new ReadPurpose(ReadDestination::READ_FOR_PUSH);
            pkt->pushSenderState(purpose);
            memPort.sendPacket(pkt);
            onTheFlyReqs++;
            pendingPullReads++;
            pendingPullAddrs.insert(addr);
        }
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

int
CoalesceEngine::workCount()
{
    return activeCacheBlocks.size() +
            directory->workCount() + activeBuffer.size();
}

void
CoalesceEngine::recvVertexPull()
{
    pullsReceived++;
    DPRINTF(CoalesceEngine, "%s: Received a vertex pull. pullsReceived: %d.\n", __func__, pullsReceived);

    stats.verticesPulled++;
    stats.lastVertexPullTime = curTick() - stats.lastResetTick;
    if (!nextApplyEvent.scheduled()) {
        schedule(nextApplyEvent, nextCycle());
    }
}

void
CoalesceEngine::processNextApplyEvent()
{
    if ((!activeBuffer.empty()) &&
        (postPushWBQueue.size() < postPushWBQueueSize)) {
        PacketPtr pkt;
        Tick entrance_tick;
        WorkListItem items[numElementsPerLine];

        std::tie(pkt, entrance_tick) = activeBuffer.front();
        pkt->writeDataToBlock((uint8_t*) items, peerMemoryAtomSize);

        for (int index = 0; (index < numElementsPerLine) && (pullsReceived > 0); index++) {
            if (graphWorkload->activeCondition(items[index])) {
                Addr addr = pkt->getAddr() + index * sizeof(WorkListItem);
                uint32_t delta = graphWorkload->apply(items[index]);
                owner->recvVertexPush(addr, delta, items[index].edgeIndex,
                                                    items[index].degree);
                pullsReceived--;
                stats.verticesPushed++;
                stats.lastVertexPushTime = curTick() - stats.lastResetTick;
            }
        }
        pkt->deleteData();
        pkt->allocate();
        pkt->setDataFromBlock((uint8_t*) items, peerMemoryAtomSize);

        bool atom_active = false;
        for (int index = 0; index < numElementsPerLine; index++) {
            atom_active |= graphWorkload->activeCondition(items[index]);
        }
        // NOTE: If the atom is not active anymore.
        if (!atom_active) {
            PacketPtr wb_pkt = createWritePacket(pkt->getAddr(),
                                        peerMemoryAtomSize, (uint8_t*) items);
            postPushWBQueue.emplace_back(wb_pkt, curTick());
            activeBuffer.pop_front();
            memoryFunctionQueue.emplace_back(
                [this] (int ignore, Tick schedule_tick) {
                    processNextPostPushWB(ignore, schedule_tick);
                }, 0, curTick());
            if ((!nextMemoryEvent.pending()) &&
                (!nextMemoryEvent.scheduled())) {
                schedule(nextMemoryEvent, nextCycle());
            }
            delete pkt;
        }
    } else if (!activeCacheBlocks.empty()) {
        int num_visited_indices = 0;
        int initial_fifo_length = activeCacheBlocks.size();
        while (true) {
            int block_index = activeCacheBlocks.front();
            if (cacheBlocks[block_index].state == CacheState::IDLE) {
                for (int index = 0; (index < numElementsPerLine) && (pullsReceived > 0); index++) {
                    if (graphWorkload->activeCondition(cacheBlocks[block_index].items[index])) {
                        Addr addr = cacheBlocks[block_index].addr + index * sizeof(WorkListItem);
                        uint32_t delta = graphWorkload->apply(cacheBlocks[block_index].items[index]);
                        cacheBlocks[block_index].dirty = true;
                        owner->recvVertexPush(addr, delta,
                            cacheBlocks[block_index].items[index].edgeIndex,
                            cacheBlocks[block_index].items[index].degree);
                        pullsReceived--;
                        stats.verticesPushed++;
                        stats.lastVertexPushTime = curTick() - stats.lastResetTick;
                    }
                }

                bool atom_active = false;
                for (int index = 0; index < numElementsPerLine; index++) {
                    atom_active |= graphWorkload->activeCondition(cacheBlocks[block_index].items[index]);
                }
                // NOTE: If we have reached the last item in the cache block
                if (!atom_active) {
                    activeCacheBlocks.erase(block_index);
                }
                break;
            }
            // NOTE: If the block with index at the front of activeCacheBlocks
            // is not in IDLE state, then roll the that index to the back
            activeCacheBlocks.pop_front();
            activeCacheBlocks.push_back(block_index);
            // NOTE: If we have visited all the items initially in the FIFO.
            num_visited_indices++;
            if (num_visited_indices == initial_fifo_length) {
                break;
            }
        }
    } else {
        DPRINTF(CoalesceEngine, "%s: Could not find "
                        "work to apply.\n", __func__);
    }

    if (pullCondition()) {
        memoryFunctionQueue.emplace_back(
            [this] (int ignore, Tick schedule_tick) {
                processNextVertexPull(ignore, schedule_tick);
            }, 0, curTick());
        if ((!nextMemoryEvent.pending()) &&
            (!nextMemoryEvent.scheduled())) {
            schedule(nextMemoryEvent, nextCycle());
        }
        pullsScheduled++;
    }

    if ((pullsReceived > 0) && (!nextApplyEvent.scheduled())) {
        schedule(nextApplyEvent, nextCycle());
    }
}


CoalesceEngine::CoalesceStats::CoalesceStats(CoalesceEngine &_coalesce)
    : statistics::Group(&_coalesce),
    coalesce(_coalesce),
    lastResetTick(0),
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
    ADD_STAT(numConflicts, statistics::units::Count::get(),
             "Number of conflicts raised by reads in the cache."),
    ADD_STAT(responsePortShortage, statistics::units::Count::get(),
             "Number of times a response has been "
             "delayed because of port shortage. "),
    ADD_STAT(numMemoryBlocks, statistics::units::Count::get(),
             "Number of times memory bandwidth was not available."),
    ADD_STAT(verticesPulled, statistics::units::Count::get(),
             "Number of times a pull request has been sent by PushEngine."),
    ADD_STAT(verticesPushed, statistics::units::Count::get(),
             "Number of times a vertex has been pushed to the PushEngine"),
    ADD_STAT(lastVertexPullTime, statistics::units::Tick::get(),
             "Time of the last pull request. (Relative to reset_stats)"),
    ADD_STAT(lastVertexPushTime, statistics::units::Tick::get(),
             "Time of the last vertex push. (Relative to reset_stats)"),
    ADD_STAT(numInvalidWriteBacks, statistics::units::Count::get(),
             "Number of times a scheduled memory function has been invalid."),
    ADD_STAT(hitRate, statistics::units::Ratio::get(),
             "Hit rate in the cache."),
    ADD_STAT(vertexPullBW, statistics::units::Rate<statistics::units::Count,
                                            statistics::units::Second>::get(),
             "Rate at which pull requests arrive."),
    ADD_STAT(vertexPushBW, statistics::units::Rate<statistics::units::Count,
                                            statistics::units::Second>::get(),
             "Rate at which vertices are pushed."),
    ADD_STAT(frontierSize, statistics::units::Count::get(),
             "Histogram of the length of the bitvector."),
    ADD_STAT(blockActiveCount, statistics::units::Count::get(),
             "Histogram of the popCount values in the directory"),
    ADD_STAT(responseQueueLatency, statistics::units::Second::get(),
             "Histogram of the response latency to WLEngine. (ns)"),
    ADD_STAT(memoryFunctionLatency, statistics::units::Second::get(),
             "Histogram of the latency of processing a memory function.")
{
}

void
CoalesceEngine::CoalesceStats::regStats()
{
    using namespace statistics;

    hitRate = (readHits + readHitUnderMisses) /
                (readHits + readHitUnderMisses + readMisses);

    vertexPullBW = (verticesPulled * getClockFrequency()) / lastVertexPullTime;

    vertexPushBW = (verticesPushed * getClockFrequency()) / lastVertexPushTime;

    frontierSize.init(64);
    blockActiveCount.init(64);
    responseQueueLatency.init(64);
    memoryFunctionLatency.init(64);
}

void
CoalesceEngine::CoalesceStats::resetStats()
{
    statistics::Group::resetStats();

    lastResetTick = curTick();
}

} // namespace gem5
