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

#include <algorithm>
#include <random>
#include <vector>

#include "accl/graph/sega/mpu.hh"
#include "debug/SEGAStructureSize.hh"
#include "debug/WLEngine.hh"
#include "mem/packet_access.hh"
#include "sim/sim_exit.hh"

namespace gem5
{

WLEngine::WLEngine(const WLEngineParams& params):
    BaseReduceEngine(params),
    updateQueueSize(params.update_queue_size),
    maxReadsPerCycle(params.rd_per_cycle),
    maxReducesPerCycle(params.reduce_per_cycle),
    maxWritesPerCycle(params.wr_per_cycle),
    maxUpdatesProcessed(params.num_updates_processed),
    registerFileSize(params.register_file_size),
    nextReadEvent([this]{ processNextReadEvent(); }, name()),
    nextReduceEvent([this]{ processNextReduceEvent(); }, name()),
    nextWriteEvent([this] { processNextWriteEvent(); }, name()),
    nextDoneSignalEvent([this] { processNextDoneSignalEvent(); }, name()),
    stats(*this)
{
    for (int i = 0; i < params.port_in_ports_connection_count; ++i) {
        inPorts.emplace_back(
                            name() + ".in_ports" + std::to_string(i), this, i);
    }
}

Port&
WLEngine::getPort(const std::string& if_name, PortID idx)
{
    if (if_name == "in_ports") {
        return inPorts[idx];
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

void
WLEngine::init()
{
    for (int i = 0; i < inPorts.size(); i++){
        inPorts[i].sendRangeChange();
    }
}

void
WLEngine::registerMPU(MPU* mpu)
{
    owner = mpu;
}

AddrRangeList
WLEngine::getAddrRanges()
{
    return owner->getAddrRanges();
}

void
WLEngine::recvFunctional(PacketPtr pkt)
{
    owner->recvFunctional(pkt);
}

AddrRangeList
WLEngine::RespPort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

void
WLEngine::RespPort::checkRetryReq()
{
    if (needSendRetryReq) {
        needSendRetryReq = false;
        sendRetryReq();
    }
}

bool
WLEngine::RespPort::recvTimingReq(PacketPtr pkt)
{
    if (!owner->handleIncomingUpdate(pkt)) {
        needSendRetryReq = true;
        return false;
    }

    return true;
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
WLEngine::checkRetryReq()
{
    std::vector<int> random_shuffle;
    for (int i = 0; i < inPorts.size(); i++) {
        random_shuffle.push_back(i);
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(random_shuffle.begin(), random_shuffle.end(), gen);

    for (int i = 0; i < inPorts.size(); i++) {
        inPorts[random_shuffle[i]].checkRetryReq();
    }
}

bool
WLEngine::done()
{
    return registerFile.empty() && updateQueue.empty();
}

bool
WLEngine::handleIncomingUpdate(PacketPtr pkt)
{
    Addr update_addr = pkt->getAddr();
    uint32_t update_value = pkt->getLE<uint32_t>();

    if (valueMap.find(update_addr) != valueMap.end()) {
        assert((updateQueueSize == 0) ||
                (updateQueue.size() <= updateQueueSize));
        DPRINTF(WLEngine, "%s: Found an already queued update to %u. ",
                            "Current value is: %u.\n", __func__,
                            update_addr, valueMap[update_addr]);
        valueMap[update_addr] =
                graphWorkload->reduce(update_value, valueMap[update_addr]);
        stats.numIncomingUpdates++;
        stats.updateQueueCoalescions++;
    } else {
        assert((updateQueueSize == 0) || (updateQueue.size() <= updateQueueSize));
        if ((updateQueueSize != 0) && (updateQueue.size() == updateQueueSize)) {
            return false;
        } else {
            updateQueue.emplace_back(update_addr, curTick());
            valueMap[update_addr] = update_value;
            stats.numIncomingUpdates++;
            DPRINTF(SEGAStructureSize, "%s: Emplaced (addr: %lu, value: %u) in the "
                        "updateQueue. updateQueue.size = %d, updateQueueSize = %d.\n",
                        __func__, update_addr, update_value,
                        updateQueue.size(), updateQueueSize);
            DPRINTF(WLEngine, "%s: Emplaced (addr: %lu, value: %u) in the "
                        "updateQueue. updateQueue.size = %d, updateQueueSize = %d.\n",
                        __func__, update_addr, update_value,
                        updateQueue.size(), updateQueueSize);
        }
    }

    // delete the packet since it's not needed anymore.
    delete pkt;

    if (!nextReadEvent.scheduled()) {
        schedule(nextReadEvent, nextCycle());
    }
    return true;
}

void
WLEngine::processNextReadEvent()
{
    std::deque<std::tuple<Addr, Tick>> temp_queue;
    for (int i = 0; i < maxUpdatesProcessed; i++) {
        if (updateQueue.empty()) {
            break;
        }
        temp_queue.push_back(updateQueue.front());
        updateQueue.pop_front();
    }

    int num_reads = 0;
    int num_popped = 0;
    int num_tries = 0;
    int max_visits = temp_queue.size();
    while (true) {
        Addr update_addr;
        Tick enter_tick;
        std::tie(update_addr, enter_tick) = temp_queue.front();

        uint32_t update_value = valueMap[update_addr];
        DPRINTF(WLEngine,  "%s: Looking at the front of the updateQueue. "
            "(addr: %lu, value: %u).\n", __func__, update_addr, update_value);
        if ((registerFile.find(update_addr) == registerFile.end())) {
            DPRINTF(WLEngine,  "%s: No register already allocated for addr: %lu "
                                "in registerFile.\n", __func__, update_addr);
            if (registerFile.size() < registerFileSize) {
                DPRINTF(WLEngine, "%s: There are free registers available in the "
                                                "registerFile.\n", __func__);
                ReadReturnStatus read_status = owner->recvWLRead(update_addr);
                if (read_status == ReadReturnStatus::ACCEPT) {
                    DPRINTF(WLEngine, "%s: CoalesceEngine returned true for read "
                                "request to addr: %lu.\n", __func__, update_addr);
                    registerFile[update_addr] = std::make_tuple(RegisterState::PENDING_READ, update_value);
                    DPRINTF(SEGAStructureSize, "%s: Added (addr: %lu, value: %u) "
                            "to registerFile. registerFile.size = %d, "
                            "registerFileSize = %d.\n", __func__, update_addr,
                            update_value, registerFile.size(), registerFileSize);
                    DPRINTF(WLEngine, "%s: Added (addr: %lu, value: %u) "
                            "to registerFile. registerFile.size = %d, "
                            "registerFileSize = %d.\n", __func__, update_addr,
                            update_value, registerFile.size(), registerFileSize);
                    temp_queue.pop_front();
                    valueMap.erase(update_addr);
                    num_reads++;
                    num_popped++;
                    stats.updateQueueLatency.sample(
                            (curTick() - enter_tick) * 1e9 / getClockFrequency());
                    DPRINTF(SEGAStructureSize, "%s: Popped (addr: %lu, value: %u) "
                                "from updateQueue. updateQueue.size = %d. "
                                "updateQueueSize = %d.\n", __func__, update_addr,
                                update_value, temp_queue.size(), updateQueueSize);
                    DPRINTF(WLEngine, "%s: Popped (addr: %lu, value: %u) "
                                "from updateQueue. updateQueue.size = %d. "
                                "updateQueueSize = %d.\n", __func__, update_addr,
                                update_value, updateQueue.size(), updateQueueSize);
                    vertexReadTime[update_addr] = curTick();
                } else {
                    if (read_status == ReadReturnStatus::REJECT_ROLL) {
                        temp_queue.pop_front();
                        temp_queue.emplace_back(update_addr, enter_tick);
                        DPRINTF(WLEngine, "%s: Received a reject from cache. "
                                            "Rolling the update.\n", __func__);
                        stats.numUpdateRolls++;
                    } else {
                        temp_queue.pop_front();
                        temp_queue.emplace_back(update_addr, enter_tick);
                        DPRINTF(WLEngine, "%s: Received a reject with no roll "
                        "from cache. Rolling the update anyway.\n", __func__);
                    }
                }
            } else {
                DPRINTF(WLEngine, "%s: There are no free registers "
                        "available in the registerFile.\n", __func__);
                temp_queue.pop_front();
                temp_queue.emplace_back(update_addr, enter_tick);
                stats.registerShortage++;
            }
        } else {
            DPRINTF(WLEngine,  "%s: A register has already been allocated for "
                "addr: %lu in registerFile. registerFile[%lu] = %u.\n", __func__,
                update_addr, update_addr, std::get<1>(registerFile[update_addr]));
            RegisterState state = std::get<0>(registerFile[update_addr]);
            if (state == RegisterState::PENDING_WRITE) {
                // NOTE: If it's pending write, let it be written.
                DPRINTF(WLEngine, "%s: Respective register for addr: "
                        "%lu is pending a write to the cache. Rolling "
                        "the update.\n", __func__, update_addr);
                temp_queue.pop_front();
                temp_queue.emplace_back(update_addr, enter_tick);
            } else {
                uint32_t curr_value = std::get<1>(registerFile[update_addr]);
                uint32_t new_value = graphWorkload->reduce(update_value, curr_value);
                registerFile[update_addr] = std::make_tuple(state, new_value);
                DPRINTF(WLEngine,  "%s: Reduced the update_value: %u with the entry in"
                            " registerFile. registerFile[%lu] = %u.\n", __func__,
                            update_value, update_addr, std::get<1>(registerFile[update_addr]));
                stats.registerFileCoalescions++;
                temp_queue.pop_front();
                valueMap.erase(update_addr);
                num_popped++;
                stats.updateQueueLatency.sample(
                                (curTick() - enter_tick) * 1e9 / getClockFrequency());
                DPRINTF(SEGAStructureSize, "%s: Popped (addr: %lu, value: %u) "
                                    "from updateQueue. updateQueue.size = %d. "
                                    "updateQueueSize = %d.\n", __func__, update_addr,
                                    update_value, updateQueue.size(), updateQueueSize);
                DPRINTF(WLEngine, "%s: Popped (addr: %lu, value: %u) "
                            "from updateQueue. updateQueue.size = %d. "
                            "updateQueueSize = %d.\n", __func__, update_addr,
                            update_value, updateQueue.size(), updateQueueSize);
            }
        }

        num_tries++;
        if (num_reads >= maxReadsPerCycle) {
            if (!temp_queue.empty()) {
                stats.numReadPortShortage++;
            }
            break;
        }
        if (num_tries >= max_visits) {
            break;
        }
        if (temp_queue.empty()) {
            break;
        }
    }

    while (!temp_queue.empty()) {
        updateQueue.push_front(temp_queue.back());
        temp_queue.pop_back();
    }
    if (num_popped > 0) {
        checkRetryReq();
    }
    if (!updateQueue.empty() && !nextReadEvent.scheduled()) {
        schedule(nextReadEvent, nextCycle());
    }
}

void
WLEngine::handleIncomingWL(Addr addr, WorkListItem wl)
{
    assert(workListFile.size() <= registerFileSize);
    assert(std::get<0>(registerFile[addr]) == RegisterState::PENDING_READ);

    workListFile[addr] = wl;
    DPRINTF(SEGAStructureSize, "%s: Added (addr: %lu, wl: %s) to "
                "workListFile. workListFile.size = %d.\n", __func__, addr,
                graphWorkload->printWorkListItem(wl), workListFile.size());
    DPRINTF(WLEngine, "%s: Added (addr: %lu, wl: %s) to "
                "workListFile. workListFile.size = %d.\n", __func__, addr,
                graphWorkload->printWorkListItem(wl), workListFile.size());

    uint32_t value = std::get<1>(registerFile[addr]);
    registerFile[addr] = std::make_tuple(RegisterState::PENDING_REDUCE, value);
    toReduce.push_back(addr);

    stats.vertexReadLatency.sample(
        ((curTick() - vertexReadTime[addr]) * 1e9) / getClockFrequency());
    vertexReadTime.erase(addr);

    if (!nextReduceEvent.scheduled()) {
        schedule(nextReduceEvent, nextCycle());
    }
}

void
WLEngine::processNextReduceEvent()
{
    int num_reduces = 0;
    while (true) {
        Addr addr = toReduce.front();
        assert(std::get<0>(registerFile[addr]) == RegisterState::PENDING_REDUCE);
        uint32_t update_value = std::get<1>(registerFile[addr]);
        workListFile[addr].tempProp =
            graphWorkload->reduce(update_value, workListFile[addr].tempProp);
        registerFile[addr] = std::make_tuple(RegisterState::PENDING_WRITE, update_value);
        num_reduces++;
        stats.numReductions++;
        toReduce.pop_front();
        toWrite.push_back(addr);

        if (num_reduces >= maxReducesPerCycle) {
            if (!toReduce.empty()) {
                stats.numReducerShortage++;
            }
            break;
        }
        if (toReduce.empty()) {
            break;
        }
    }

    if (!toWrite.empty() && !nextWriteEvent.scheduled()) {
        schedule(nextWriteEvent, nextCycle());
    }

    if (!toReduce.empty() && !nextReduceEvent.scheduled()) {
        schedule(nextReduceEvent, nextCycle());
    }
}

void
WLEngine::processNextWriteEvent()
{
    int num_writes = 0;
    while (true) {
        Addr addr = toWrite.front();
        assert(std::get<0>(registerFile[addr]) == RegisterState::PENDING_WRITE);
        owner->recvWLWrite(addr, workListFile[addr]);
        registerFile.erase(addr);
        workListFile.erase(addr);
        toWrite.pop_front();
        num_writes++;
        if (num_writes >= maxWritesPerCycle) {
            if (!toWrite.empty()) {
                stats.numWritePortShortage++;
            }
            break;
        }
        if (toWrite.empty()) {
            break;
        }
    }

    if (done() && !nextDoneSignalEvent.scheduled()) {
        schedule(nextDoneSignalEvent, nextCycle());
    }

    if (!toWrite.empty() && !nextWriteEvent.scheduled()) {
        schedule(nextWriteEvent, nextCycle());
    }
}

void
WLEngine::processNextDoneSignalEvent()
{
    if (done()) {
        owner->recvDoneSignal();
    }
}

WLEngine::WorkListStats::WorkListStats(WLEngine &_wl)
    : statistics::Group(&_wl),
    wl(_wl),
    ADD_STAT(updateQueueCoalescions, statistics::units::Count::get(),
             "Number of coalescions in the update queues."),
    ADD_STAT(registerShortage, statistics::units::Count::get(),
             "Number of times updates were "
             "stalled because of register shortage"),
    ADD_STAT(numUpdateRolls, statistics::units::Count::get(),
             "Number of times an update has been rolled back "
             "to the back of the update queue due to cache reject."),
    ADD_STAT(numReadPortShortage, statistics::units::Count::get(),
             "Number of times limited by read per cycle."),
    ADD_STAT(registerFileCoalescions, statistics::units::Count::get(),
             "Number of memory blocks read for vertecies"),
    ADD_STAT(numReductions, statistics::units::Count::get(),
             "Number of memory blocks read for vertecies"),
    ADD_STAT(numReducerShortage, statistics::units::Count::get(),
             "Number of times limited by number of reducers."),
    ADD_STAT(numWritePortShortage, statistics::units::Count::get(),
             "Number of times limited by write per cycle."),
    ADD_STAT(numIncomingUpdates, statistics::units::Count::get(),
              "Number of inocoming updates for each GPT."),
    ADD_STAT(vertexReadLatency, statistics::units::Second::get(),
             "Histogram of the latency of reading a vertex (ns)."),
    ADD_STAT(updateQueueLatency, statistics::units::Second::get(),
             "Histogram of the latency of dequeuing an update (ns).")
{
}

void
WLEngine::WorkListStats::regStats()
{
    using namespace statistics;

    vertexReadLatency.init(64);
    updateQueueLatency.init(64);

}

} // namespace gem5
