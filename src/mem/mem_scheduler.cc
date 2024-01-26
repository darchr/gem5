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
#include "mem/mem_scheduler.hh"

#include <string>

#include "base/trace.hh"
#include "debug/MemScheduler.hh"

namespace gem5
{
typedef std::pair<PacketPtr, PortID> ReqPair;
typedef std::pair<uint64_t, PortID> QueuePair;

MemScheduler::MemScheduler(const MemSchedulerParams &params):
    ClockedObject(params),
    nextReqEvent([this]{ processNextReqEventOpt(); }, name()),
    nextRespEvent([this]{ processNextRespEvent(); }, name()),
    readBufferSize(params.read_buffer_size),
    writeBufferSize(params.write_buffer_size),
    respBufferSize(params.resp_buffer_size),
    nMemPorts(params.port_mem_side_connection_count),
    nCpuPorts(params.port_cpu_side_connection_count),
    unifiedQueue(params.unified_queue),
    stats(*this)
{

    panic_if(readBufferSize == 0, "readBufferSize should be non-zero");
    panic_if(writeBufferSize == 0, "writeBufferSize "
                                    "should be non-zero");
    for (uint32_t i = 0; i < nCpuPorts; ++i){
        cpuPorts.emplace_back(name() + ".cpu_side" + std::to_string(i),
                              i, this);
        requestQueues.push_back(RequestQueue(readBufferSize, writeBufferSize,
                            params.service_write_threshold, unifiedQueue, i));
        responseQueues.push_back(ResponseQueue(respBufferSize, i));
    }
    for (uint32_t i = 0; i < nMemPorts; ++i){
        memPorts.emplace_back(name() + ".mem_side" + std::to_string(i),
                                i, this);
    }
}

Port &
MemScheduler::getPort(const std::string &if_name, PortID idx)
{

    // This is the name from the Python SimObject declaration (MemScheduler.py)
    if (if_name == "mem_side" && idx < memPorts.size()) {
        return memPorts[idx];
    } else if (if_name == "cpu_side") {
        return cpuPorts[idx];
    } else {
        // pass it along to our super class
        return SimObject::getPort(if_name, idx);
    }
}

void
MemScheduler::CPUSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(_blockedPacket != nullptr,
            "Should never try to send if blocked!");

    // If we can't send the packet across the port, store it for later.
    if (!sendTimingResp(pkt)){
        DPRINTF(MemScheduler, "sendPacket: "
            "Changing blockedPacket value, blockedPacket == nullptr: %d\n",
            _blockedPacket == nullptr);
        _blockedPacket = pkt;
        DPRINTF(MemScheduler, "sendPacket: "
            "Changed blockedPacket value, blockedPacket == nullptr: %d\n",
            _blockedPacket == nullptr);
        _blocked = true;
    }
}

AddrRangeList
MemScheduler::CPUSidePort::getAddrRanges() const
{
    return owner->getAddrRanges();
}
//sendReqretry
void
MemScheduler::CPUSidePort::trySendRetry()
{
    DPRINTF(MemScheduler, "Sending retry req for %d\n", portId());
    sendRetryReq();
}

Tick
MemScheduler::CPUSidePort::recvAtomic(PacketPtr pkt)
{
    DPRINTF(MemScheduler, "recvAtomic: Atomic request to memSched %s\n",
            pkt->print());
    return owner->handleAtomic(pkt);
}

void
MemScheduler::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    return owner->handleFunctional(pkt);
}

bool
MemScheduler::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    if (!owner->handleRequest(portId(), pkt)) {
        DPRINTF(MemScheduler,
                "recvTimingReq: handleRequest returned false.\n");
        return false;
    }
    return true;
}

void
MemScheduler::CPUSidePort::recvRespRetry()
{
    // We should have a blocked packet if this function is called.
    assert(_blockedPacket != nullptr);

    // Grab the blocked packet.
    PacketPtr pkt = _blockedPacket;
    DPRINTF(MemScheduler, "recvRespRetry: "
        "Changing blockedPacket value, blockedPacket == nullptr: %d\n",
        _blockedPacket == nullptr);
    _blockedPacket = nullptr;
    DPRINTF(MemScheduler, "recvRespRetry: "
        "Changed blockedPacket value, blockedPacket == nullptr: %d\n",
        _blockedPacket == nullptr);
    _blocked = false;
    sendPacket(pkt);
}

void
MemScheduler::MemSidePort::trySendRetry()
{
    DPRINTF(MemScheduler, "Sending retry resp for memPort[%d]\n", portId());
    sendRetryResp();
}

void
MemScheduler::MemSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(_blocked, "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
        DPRINTF(MemScheduler, "Setting blocked to true on port %s\n",
                this->name());
        _blocked = true;
    }
}

bool
MemScheduler::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    return owner->handleResponse(portId(), pkt);
}

void
MemScheduler::MemSidePort::recvReqRetry()
{
    // We should have a blocked packet if this function is called.
    assert(_blocked && blockedPacket != nullptr);
    DPRINTF(MemScheduler, "Received ReqRetry\n");
    // Try to resend it. It's possible that it fails again.
    _blocked = false;
    sendPacket(blockedPacket);
    blockedPacket = nullptr;

    owner->wakeUp();
}

void
MemScheduler::MemSidePort::recvRangeChange()
{
    owner->recvRangeChange(_portId);
}

MemScheduler::MemSchedulerStat::MemSchedulerStat(MemScheduler &_sched)
     : statistics::Group(&_sched),
     sched(_sched),
    ADD_STAT(readReqs, statistics::units::Count::get(),
        "Number of read requests received by the MemScheduler"),
    ADD_STAT(writeReqs, statistics::units::Count::get(),
        "Number of write requests received by the MemScheduler"),
    ADD_STAT(bankConflicts, statistics::units::Count::get(),
        "Number of bank conflicts"),
    ADD_STAT(failedArbitrations, statistics::units::Count::get(),
        "Number of times an arbitration failed to send a request to MemCtrl"),
    ADD_STAT(totalArbitrations,  statistics::units::Count::get(),
        "Total number of arbitrations over the request queues in the "
        "MemScheduler"),
    ADD_STAT(totalRQDelay,  statistics::units::Count::get(),
        "Total queueing delay read requests have "
        "experienced in the MemScheduler"),
    ADD_STAT(totalWQDelay,  statistics::units::Count::get(),
        "Total queueing delay write requests have "
        "experienced in the MemScheduler"),
    ADD_STAT(totalRespDelay,  statistics::units::Count::get(),
        "total queueing delay in the response queue "
        "experienced by the MemScheduler"),
    ADD_STAT(avgRQDelay,  statistics::units::Count::get(),
        "Average queueing delay read requests have "
        "experienced in the MemScheduler"),
    ADD_STAT(avgWQDelay,  statistics::units::Count::get(),
         "Average queueing delay write requests have "
        "experienced in the MemScheduler"),
    ADD_STAT(avgRespDelay,  statistics::units::Count::get(),
       "Average queueing delay in the response queue "
        "experienced by the MemScheduler")
{}

void
MemScheduler::MemSchedulerStat::regStats(){
    avgRQDelay.precision(2);
    avgWQDelay.precision(2);
    avgRespDelay.precision(2);
    avgRQDelay = totalRQDelay / readReqs;
    avgWQDelay = totalWQDelay / writeReqs;
    avgRespDelay = totalRespDelay / readReqs;
}

bool
MemScheduler::handleRequest(PortID portId, PacketPtr pkt)
{
      ///////////////////////////////////////////////////////////////////////
     /////////////////// TODO: adjust the schedule freq ////////////////////
    ///////////////////////////////////////////////////////////////////////
    DPRINTF(MemScheduler, "handleRequest: "
        "Got request from cpuPort[%d]\n", portId);
    auto queue = find_if(requestQueues.begin(), requestQueues.end(),
        [portId](RequestQueue &obj){return obj.cpuPortId == portId;});
    if (queue->blocked(pkt->isRead() && !pkt->isWrite())){
        if (pkt->isWrite() && !unifiedQueue){
            DPRINTF(MemScheduler, "handleRequest: Blocking write request\n");
            queue->sendWriteRetry = true;
        }
        if (pkt->isRead() || (unifiedQueue && pkt->isWrite())){
            DPRINTF(MemScheduler, "handleRequest: Blocking read request\n");
            queue->sendReadRetry = true;
        }
        return false;
    }
    if (pkt->isRead()){
        stats.readReqs++;
        entryTimes[pkt] = curTick();
        DPRINTF(MemScheduler, "handleRequest: "
            "Pushing read request to readQueues[%d], size = %d\n",
            portId, queue->readQueue.size());
        queue->push(pkt);
        DPRINTF(MemScheduler, "handleRequest: "
            "Pushed read request to readQueues[%d], size = %d\n",
            portId, queue->readQueue.size());
    } else{
        stats.writeReqs++;
        entryTimes[pkt] = curTick();
        DPRINTF(MemScheduler, "handleRequest: "
            "Pushing write request to writeQueues[%d], size = %d\n",
            portId, queue->writeQueue.size());
        queue->push(pkt);
        DPRINTF(MemScheduler, "handleRequest: "
            "Pushed write request to writeQueues[%d], size = %d\n",
            portId, queue->writeQueue.size());
    }
    if (!nextReqEvent.scheduled()){
        DPRINTF(MemScheduler, "handleRequest: "
            "Scheduling nextReqEvent in handleRequest\n");
        schedule(nextReqEvent, nextCycle());
    }
    return true;
}

void
MemScheduler::processNextReqEvent(){

    uint64_t minCheck = -1;
    RequestQueue *queue = NULL;
    DPRINTF(MemScheduler, "processNextReqEvent: "
        "Finding the least recently visited non-empty queue\n");
    for (auto &it : requestQueues){
        if ((it.timesChecked < minCheck) &&
            (!it.emptyRead() || it.serviceWrite())){
            minCheck = it.timesChecked;
            queue = &it;
        }
    }

    stats.totalArbitrations++;
    DPRINTF(MemScheduler, "processNextReqEvent: "
        "Least recently visited queue found, cpuPortId = %d, "
        "(timesChecked = %d) == (minChecked = %d), readQueue.size = %d, "
        "writeQueue.size = %d, emptyRead = %d, serviceWrite = %d\n",
        queue->cpuPortId, queue->timesChecked, minCheck,
        queue->readQueue.size(), queue->writeQueue.size(), queue->emptyRead(),
        queue->serviceWrite());

    PacketPtr pkt;
    if (!queue->serviceWrite()){
        DPRINTF(MemScheduler, "processNextReqEvent: "
            "Servicing read request now\n");
        pkt = queue->readQueue.front();
    }
    else{
        DPRINTF(MemScheduler, "processNextReqEvent: "
            "Servicing write request now\n");
        pkt = queue->writeQueue.front();
    }
    AddrRange addr_range = pkt->getAddrRange();
    DPRINTF(MemScheduler, "processNextReqEvent: pkt->addr_range: %s\n",
        addr_range.to_string());
    PortID memPortId = memPortMap.contains(addr_range)->second;
    DPRINTF(MemScheduler, "processNextReqEvent: "
        "Looked up outgoing routing table for MemSidePort PortID: %d\n",
        memPortId);
    auto memPort = find_if(memPorts.begin(), memPorts.end(),
        [memPortId](MemSidePort &obj){return obj.portId() == memPortId;});
    DPRINTF(MemScheduler, "processNextReqEvent: "
        "Port with proper addr range, memSidePort[%d]\n", memPort->portId());
    queue->timesChecked++;

    DPRINTF(MemScheduler, "processNextReqEvent: "
        "Sending the packet if port not blocked\n");
    if (!memPort->blocked()){
        DPRINTF(MemScheduler, "processNextReqEvent: "
            "Port not blocked! Sending the packet\n");
        memPort->sendPacket(pkt);
        PortID cpuPortId = queue->cpuPortId;
        if (pkt->needsResponse()){
            DPRINTF(MemScheduler, "processNextReqEvent: "
                "Updating incoming routing table for future, <%s, %d>\n",
                pkt->getAddrRange().to_string(), cpuPortId);
            RequestPtr req = pkt->req;
            respRoutingTable[req] = cpuPortId;
        }
        if (!queue->serviceWrite()){
            DPRINTF(MemScheduler, "processNextReqEvent: "
                "Popping read request from readQueues[%d], size = %d\n",
                cpuPortId, queue->readQueue.size());
            stats.totalRQDelay += curTick() - entryTimes[pkt];
            entryTimes.erase(pkt);
            queue->readQueue.pop();
            DPRINTF(MemScheduler, "processNextReqEvent: Popped read request "
                "from readQueues[%d], size = %d\n",
                cpuPortId, queue->readQueue.size());
        }
        else{
            DPRINTF(MemScheduler, "processNextReqEvent: "
                "Popping write request from writeQueues[%d], size = %d\n",
                cpuPortId, queue->writeQueue.size());
            stats.totalWQDelay += curTick() - entryTimes[pkt];
            entryTimes.erase(pkt);
            queue->writeQueue.pop();
            DPRINTF(MemScheduler, "processNextReqEvent: "
                "Popped write request from writeQueues[%d], size = %d\n",
                cpuPortId, queue->writeQueue.size());
        }
    }
    if (memPort->blocked()){
        for (auto port : memPorts){
            if (!port.blocked()){
                stats.failedArbitrations++;
                break;
            }
        }
    }

    if (!nextReqEvent.scheduled()){
        for (auto &queue : requestQueues){
            if (!queue.emptyRead() || queue.serviceWrite()){
                DPRINTF(MemScheduler, "processNextReqEvent: "
                    "Scheduling nextReqEvent in processNextReqEvent\n");
                schedule(nextReqEvent, nextCycle());
                break;
            }
        }
    }
    if (!unifiedQueue){
        DPRINTF(MemScheduler, "processNextReqEvent: "
            "queue->readQueueBlocked: %d, queue->writeQueueBlocked: %d\n",
            queue->blocked(1), queue->blocked(0));
        if (queue->sendReadRetry && !queue->blocked(pkt->isRead())
            && pkt->isRead()){
            PortID cpuPortId = queue->cpuPortId;
            auto cpuPort = find_if(cpuPorts.begin(), cpuPorts.end(),
                [cpuPortId](CPUSidePort &obj)
                {return obj.portId() == cpuPortId;});
            DPRINTF(MemScheduler, "processNextReqEvent: "
                "Sending read retry to ports previously blocked\n");
            cpuPort->trySendRetry();
            queue->sendReadRetry = false;
        }
        if (queue->sendWriteRetry && !queue->blocked(pkt->isRead())
            && pkt->isWrite()){
            PortID cpuPortId = queue->cpuPortId;
            auto cpuPort = find_if(cpuPorts.begin(), cpuPorts.end(),
                [cpuPortId](CPUSidePort &obj)
                {return obj.portId() == cpuPortId;});
            DPRINTF(MemScheduler, "processNextReqEvent: "
                "Sending write retry to ports previously blocked\n");
            cpuPort->trySendRetry();
            queue->sendWriteRetry = false;
        }
    }
    else{
        DPRINTF(MemScheduler, "processNextReqEvent: "
            "queue->readQueueBlocked: %d\n", queue->blocked(1));
        if (queue->sendReadRetry && !queue->blocked(pkt->isRead()
            || pkt->isWrite())){
            PortID cpuPortId = queue->cpuPortId;
            auto cpuPort = find_if(cpuPorts.begin(), cpuPorts.end(),
                [cpuPortId](CPUSidePort &obj)
                {return obj.portId() == cpuPortId;});
            DPRINTF(MemScheduler, "processNextReqEvent: "
                "Sending read retry to ports previously blocked\n");
            cpuPort->trySendRetry();
            queue->sendReadRetry = false;
        }
    }

    return;
}

MemScheduler::RequestQueue*
MemScheduler::arbitrate(std::map<PortID, bool> visited){
    uint64_t minCheck = -1;
    RequestQueue *queue = NULL;
    DPRINTF(MemScheduler, "arbitrate: "
        "Finding the least recently visited non-empty queue\n");
    for (auto &it : requestQueues){
        if ((it.timesChecked < minCheck) &&
            (!it.emptyRead() || it.serviceWrite()) &&
            (!visited[it.cpuPortId])){
            minCheck = it.timesChecked;
            queue = &it;
        }
    }
    if (queue != NULL){
        DPRINTF(MemScheduler, "arbitrate: Least recently visited queue found, "
            "cpuPortId = %d, timesChecked = %d, readQueue.size = %d, "
            "writeQueue.size = %d, emptyRead = %d, serviceWrite = %d\n",
            queue->cpuPortId, queue->timesChecked, queue->readQueue.size(),
            queue->writeQueue.size(), queue->emptyRead(),
            queue->serviceWrite());
    }
    return queue;
}

void
MemScheduler::processNextReqEventOpt(){
    std::map<PortID, bool> visited;
    for (auto queue : requestQueues){
        visited[queue.cpuPortId] = false;
    }
    auto pick = arbitrate(visited);

    if (pick == nullptr){
        stats.failedArbitrations++;
        return;
    }

    visited[pick->cpuPortId] = true;

    PacketPtr pkt;
    std::vector<MemSidePort>::iterator memPort;
    while (true){
        if (!pick->serviceWrite()){
            DPRINTF(MemScheduler, "processNextReqEvent: "
                "Servicing read request now\n");
            pkt = pick->readQueue.front();
        }
        else{
            DPRINTF(MemScheduler, "processNextReqEvent: "
                "Servicing write request now\n");
            pkt = pick->writeQueue.front();
        }
        AddrRange addr_range = pkt->getAddrRange();
        DPRINTF(MemScheduler, "processNextReqEvent: pkt->addr_range: %s\n",
            addr_range.to_string());
        PortID memPortId = memPortMap.contains(addr_range)->second;
        DPRINTF(MemScheduler, "processNextReqEvent: Looked up outgoing routing"
            " table for MemSidePort PortID: %d\n", memPortId);
        memPort = find_if(memPorts.begin(), memPorts.end(),
            [memPortId](MemSidePort &obj)
            {return obj.portId() == memPortId;});
        DPRINTF(MemScheduler, "processNextReqEvent: "
            "Port with proper addr range, memSidePort[%d]\n",
            memPort->portId());
        DPRINTF(MemScheduler, "processNextReqEvent: "
            "Checking if port is blocked!\n");
        if (!memPort->blocked()){
            DPRINTF(MemScheduler, "Port %d not blocked!\n", memPort->portId());
            break;
        }
        else {
            DPRINTF(MemScheduler, "Port %d blocked!\n", memPort->portId());
            pick = arbitrate(visited);
            if (pick == NULL){
                DPRINTF(MemScheduler, "processNextReqEvent: "
                    "arbitration failed!\n");
                for (auto queue : requestQueues){
                    if (!queue.emptyRead()){
                        stats.bankConflicts++;
                        break;
                    }
                }
                stats.failedArbitrations++;
                return;
            }
            visited[pick->cpuPortId] = true;
        }
    }

    DPRINTF(MemScheduler, "processNextReqEvent: "
        "Port not blocked! Sending the packet\n");
    PortID cpuPortId = pick->cpuPortId;
    if (pkt->needsResponse()){
        DPRINTF(MemScheduler, "processNextReqEvent: "
            "Updating incoming routing table for future, <%s, %d>\n",
            pkt->getAddrRange().to_string(), cpuPortId);
        RequestPtr req = pkt->req;
        respRoutingTable[req] = cpuPortId;
    }

    memPort->sendPacket(pkt);
    pick->timesChecked++;

    if (!pick->serviceWrite()){
        DPRINTF(MemScheduler, "processNextReqEvent: "
            "Popping read request from readQueues[%d], size = %d\n",
            cpuPortId, pick->readQueue.size());
        stats.totalRQDelay += curTick() - entryTimes[pkt];
        entryTimes.erase(pkt);
        pick->readQueue.pop();
        DPRINTF(MemScheduler, "processNextReqEvent: "
            "Popped read request from readQueues[%d], size = %d\n",
            cpuPortId, pick->readQueue.size());
    }
    else{
        DPRINTF(MemScheduler, "processNextReqEvent: "
            "Popping write request from writeQueues[%d], size = %d\n",
            cpuPortId, pick->writeQueue.size());
        stats.totalWQDelay += curTick() - entryTimes[pkt];
        entryTimes.erase(pkt);
        pick->writeQueue.pop();
        DPRINTF(MemScheduler, "processNextReqEvent: "
            "Popped write request from writeQueues[%d], size = %d\n",
            cpuPortId, pick->writeQueue.size());
    }

    if (!nextReqEvent.scheduled()){
        for (auto &queue : requestQueues){
            if (!queue.emptyRead() || queue.serviceWrite()){
                DPRINTF(MemScheduler, "processNextReqEvent: "
                    "Scheduling nextReqEvent in processNextReqEvent\n");
                schedule(nextReqEvent, nextCycle());
                break;
            }
        }
    }

    if (!unifiedQueue){
        DPRINTF(MemScheduler, "processNextReqEvent: "
            "queue->readQueueBlocked: %d, queue->writeQueueBlocked: "
            "%d\n", pick->blocked(1), pick->blocked(0));
        if (pick->sendReadRetry && !pick->blocked(pkt->isRead())
                                && pkt->isRead()){
            PortID cpuPortId = pick->cpuPortId;
            auto cpuPort = find_if(cpuPorts.begin(), cpuPorts.end(),
                [cpuPortId](CPUSidePort &obj)
                {return obj.portId() == cpuPortId;});
            DPRINTF(MemScheduler, "processNextReqEvent: "
            "Sending read retry to ports previously blocked\n");
            cpuPort->trySendRetry();
            pick->sendReadRetry = false;
        }
        if (pick->sendWriteRetry && !pick->blocked(pkt->isRead())
                                 && pkt->isWrite()){
            PortID cpuPortId = pick->cpuPortId;
            auto cpuPort = find_if(cpuPorts.begin(), cpuPorts.end(),
                [cpuPortId](CPUSidePort &obj)
                {return obj.portId() == cpuPortId;});
            DPRINTF(MemScheduler, "processNextReqEvent: "
                "Sending write retry to ports previously blocked\n");
            cpuPort->trySendRetry();
            pick->sendWriteRetry = false;
        }
    }
    else{
        DPRINTF(MemScheduler, "processNextReqEvent: "
        "queue->readQueueBlocked: %d\n", pick->blocked(1));
        if (pick->sendReadRetry && !pick->blocked(pkt->isRead()
            || pkt->isWrite())){
            PortID cpuPortId = pick->cpuPortId;
            auto cpuPort = find_if(cpuPorts.begin(), cpuPorts.end(),
                [cpuPortId](CPUSidePort &obj)
                {return obj.portId() == cpuPortId;});
            DPRINTF(MemScheduler, "processNextReqEvent: "
            "Sending read retry to ports previously blocked\n");
            cpuPort->trySendRetry();
            pick->sendReadRetry = false;
        }
    }

    return;
}

void
MemScheduler::processNextRespEvent(){

    DPRINTF(MemScheduler, "processNextRespEvent: "
    "Iterating over respQueues to send resp to cpuPorts\n");
    for (auto &queue : responseQueues){
        PortID cpuPortId = queue.cpuPortId;
        auto cpuPort = find_if(cpuPorts.begin(), cpuPorts.end(),
            [cpuPortId](CPUSidePort &obj)
            {return obj.portId() == cpuPortId;});
        DPRINTF(MemScheduler, "processNextRespEvent: "
        "cpuPort chosen, (cpuPortId = %d) == (cpuPort->portId = %d)\n"
        , cpuPortId, cpuPort->portId());
        if ((!cpuPort->blocked()) && (!queue.respQueue.empty())){
            PacketPtr pkt = queue.respQueue.front();
            cpuPort->sendPacket(pkt);
            DPRINTF(MemScheduler, "processNextRespEvent: "
                "Popping resp responseQueue[%d], size = %d\n",
                cpuPortId, queue.respQueue.size());
            stats.totalRespDelay += curTick() - respEntryTimes[pkt];
            respEntryTimes.erase(pkt);
            queue.respQueue.pop();
            DPRINTF(MemScheduler, "processNextRespEvent: "
                "Popped resp responseQueue[%d], size = %d\n",
                cpuPortId, queue.respQueue.size());
        }
    }

    if (!nextRespEvent.scheduled()){
        for (auto &queue : responseQueues){
            if (!queue.empty()){
                DPRINTF(MemScheduler, "processNextRespEvent: "
                    "Scheduling nextRespEvent in processNextRespEvent\n");
                schedule(nextRespEvent, nextCycle());
                break;
            }
        }
    }

    for (auto &queue : responseQueues){
        if ((queue.sendRetry) && (!queue.blocked())){
            PortID memPortId = queue.memPortId;
            auto memPort = find_if(memPorts.begin(), memPorts.end(),
                [memPortId](MemSidePort &obj)
                {return obj.portId() == memPortId;});
            DPRINTF(MemScheduler, "processNextRespEvent: "
                "Sending retry to ports previously blocked, "
                "<respQueue[%d], memPort[%d]>\n",
            queue.cpuPortId, memPort->portId());
            memPort->trySendRetry();
            queue.sendRetry = false;
        }
    }
    return;
}

bool
MemScheduler::handleResponse(PortID memPortId, PacketPtr pkt)
{
    RequestPtr req = pkt->req;
    PortID cpuPortId = respRoutingTable[req];
    DPRINTF(MemScheduler, "handleResponse: "
        "Received response from memSidePort[%d]\n", memPortId);
    DPRINTF(MemScheduler, "handleResponse: "
        "Looking up the incoming routing table to find proper "
        "queue for response, <%s, %d>\n",
        pkt->getAddrRange().to_string(), cpuPortId);
    auto responseQueue = find_if(responseQueues.begin(), responseQueues.end(),
        [cpuPortId](ResponseQueue &obj)
        {return obj.cpuPortId == cpuPortId;});
    DPRINTF(MemScheduler, "handleResponse: "
        "Found correct cpuPort for range, <%s, %d>\n",
        pkt->getAddrRange().to_string(), responseQueue->cpuPortId);
    DPRINTF(MemScheduler, "handleResponse: "
        "Pushing response if queue not blocked!\n");
    if (responseQueue->blocked()){
        DPRINTF(MemScheduler, "handleResponse: "
            "Queue blocked! Remembering to send retry later, memPortId = %d\n",
            memPortId);
        responseQueue->sendRetry = true;
        responseQueue->memPortId = memPortId;
        return false;
    }
    DPRINTF(MemScheduler, "handleResponse: Queue not blocked!\n");
    DPRINTF(MemScheduler, "handleResponse: "
        "Pushing response to queues, responseQueues[%d], size = %d\n",
        cpuPortId, responseQueue->respQueue.size());
    respEntryTimes[pkt] = curTick();
    responseQueue->respQueue.push(pkt);
    DPRINTF(MemScheduler, "handleResponse: "
        "Pushed response to queues, responseQueues[%d], size = %d\n",
        cpuPortId, responseQueue->respQueue.size());
    if (!nextRespEvent.scheduled()){
        DPRINTF(MemScheduler, "handleResponse: "
            "Scheduling nextRespEvent in handleResponse\n");
        schedule(nextRespEvent, nextCycle());
    }
    return true;
}

Tick
MemScheduler::handleAtomic(PacketPtr pkt)
{
    // Just pass this on to the memory side to handle for now.
    const Addr base_addr = pkt->getAddr();

    PortID memPortId = memPortMap.contains(base_addr)->second;
    DPRINTF(MemScheduler, "handleAtomic: Looked up outgoing routing"
        " table for MemSidePort PortID: %d\n", memPortId);
    auto memPort = find_if(memPorts.begin(), memPorts.end(),
        [memPortId](MemSidePort &obj)
        {return obj.portId() == memPortId;});
    return memPort->sendAtomic(pkt);
}

void
MemScheduler::handleFunctional(PacketPtr pkt)
{
    const Addr base_addr = pkt->getAddr();

    PortID memPortId = memPortMap.contains(base_addr)->second;
    DPRINTF(MemScheduler, "handleFunctional: Looked up outgoing routing"
        " table for MemSidePort PortID: %d\n", memPortId);
    auto memPort = find_if(memPorts.begin(), memPorts.end(),
        [memPortId](MemSidePort &obj)
        {return obj.portId() == memPortId;});
    memPort->sendFunctional(pkt);
}

AddrRangeList
MemScheduler::getAddrRanges() const
{
    DPRINTF(MemScheduler, "Sending new ranges\n");
    // Just use the same ranges as whatever is on the memory side.
    AddrRangeList ret;
    // Simply forward to the memory port
    for (auto &memPort : memPorts){
        // AddrRangeList addr_range = memPort->getAddrRanges();
        for (auto &addr_range : memPort.getAddrRanges()){
            ret.push_back(addr_range);
        }
    }
    return ret;
}

void
MemScheduler::sendRangeChange()
{
    for (auto &cpuPort : cpuPorts)
        cpuPort.sendRangeChange();
}

void MemScheduler::recvRangeChange(PortID portId)
{
    DPRINTF(MemScheduler, "Received range change from mem_side_port[%d].\n",
                            portId);
    for (auto &port : memPorts){
        if (port.portId() == portId){
            AddrRangeList ranges = port.getAddrRanges();
            for (auto &r : ranges){
                memPortMap.insert(r, portId);
            }
        }
    }
    sendRangeChange();
}

void
MemScheduler::wakeUp()
{
    if (!nextReqEvent.scheduled()){
        for (auto &queue : requestQueues){
            if (!queue.emptyRead()){
                schedule(nextReqEvent, nextCycle());
                return;
            }
        }
    }
}

}