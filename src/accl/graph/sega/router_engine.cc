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

#include "accl/graph/sega/router_engine.hh"

#include "accl/graph/sega/centeral_controller.hh"
#include "base/trace.hh"
#include "sim/stats.hh"
#include "debug/RouterEngine.hh"

namespace gem5
{
RouterEngine::RouterEngine(const Params &params):
  ClockedObject(params),
  system(params.system),
  gptQSize(params.gpt_queue_size),
  gpnQSize(params.gpn_queue_size),
  emptyQueues(false),
  routerLatency(params.router_latency),
  nextGPTGPNEvent([this] { processNextGPTGPNEvent(); }, name()),
  nextInternalRequestEvent(
                        [this] { processNextInternalRequestEvent(); }, name()),
  nextGPNGPTEvent([this] { processNextGPNGPTEvent(); }, name()),
  nextExternalRequestEvent(
                        [this] { processNextExternalRequestEvent(); }, name()),
  stats(*this)
{

    for (int i = 0; i < params.port_gpt_req_side_connection_count; ++i) {
        gptReqPorts.emplace_back(
                    name() + ".gpt_req_side" + std::to_string(i), this, i);
        // m_newTraffic.emplace_back(new statistics::Histogram());
        // m_newTraffic[i]->init(10);
    }

    for (int i = 0; i < params.port_gpt_resp_side_connection_count; ++i) {
        gptRespPorts.emplace_back(
                    name() + ".gpt_resp_side" + std::to_string(i), this, i);
    }

    for (int i = 0; i < params.port_gpn_req_side_connection_count; ++i) {
        gpnReqPorts.emplace_back(
                    name() + ".gpn_req_side" + std::to_string(i), this, i);
    }

    for (int i = 0; i < params.port_gpn_resp_side_connection_count; ++i) {
        gpnRespPorts.emplace_back(
                    name() + ".gpn_resp_side" + std::to_string(i), this, i);
    }

    for (int i = 0; i <params.port_gpt_req_side_connection_count; ++i) {
        externalLatency[i] = curCycle();
    }

    for (int i = 0; i < params.port_gpn_req_side_connection_count; ++i) {
        internalLatency[i] = curCycle();
    }
}

void
RouterEngine::registerCenteralController(
                                    CenteralController* centeral_controller)
{
    centeralController = centeral_controller;
}

AddrRangeList
RouterEngine::GPTRespPort::getAddrRanges() const
{
    return owner->getGPNRanges();
}

AddrRangeList
RouterEngine::GPNRespPort::getAddrRanges() const
{
    return owner->getGPTRanges();
}

AddrRangeList
RouterEngine::getGPNRanges()
{
    AddrRangeList ret;
    for (auto &gpnPort : gpnReqPorts) {
        for (auto &addr_range : gpnPort.getAddrRanges()) {
            ret.push_back(addr_range);
        }
    }
    // for(auto i = routerAddrMap.begin(); i != routerAddrMap.end(); ++i) {
    //     ret.push_back(i->second);
    // }
    return ret;
}

AddrRangeList
RouterEngine::getGPTRanges()
{
    AddrRangeList ret;
    for (auto &gptPort : gptReqPorts) {
        for (auto &addr_range : gptPort.getAddrRanges()) {
            ret.push_back(addr_range);
        }
    }
    return ret;
}

void
RouterEngine::init()
{
    for (int i = 0; i < gptReqPorts.size(); i++) {
        gptAddrMap[gptReqPorts[i].id()] = gptReqPorts[i].getAddrRanges();
    }
    std::cout<<"gptReqPorts: "<<gptReqPorts.size()<<std::endl;
}

// void
// RouterEngine::resetStats()
// {
//     for (int i = 0; i < gptReqPorts.size(); i++) {
//         m_newTraffic[i]->reset();
//     }
// }

void
RouterEngine::startup()
{
    for (int i = 0; i < gpnReqPorts.size(); i++) {
        routerAddrMap[gpnReqPorts[i].id()] = gpnReqPorts[i].getAddrRanges();
    }
    std::cout<<"gpnReqPorts: "<<gpnReqPorts.size()<<std::endl;
}

bool
RouterEngine::done()
{
    bool emptygptReq = true;
    bool emptygpnReq = true;
    bool emptygptResp = true;
    bool emptygpnResp = true;
    bool empty = true;
    for (auto &q: gptReqQueues) {
        emptygptReq &= q.second.empty();
    }

    for (auto &q: gpnReqQueues) {
        emptygpnReq &= q.second.empty();
    }

    for (auto &q: gptRespQueues) {
        emptygptResp &= q.second.empty();
    }

    for (auto &q: gpnRespQueues) {
        emptygpnResp &= q.second.empty();
    }

    empty = emptygptReq & emptygpnReq & emptygptResp & emptygpnResp;
    DPRINTF(RouterEngine, "%s: emptygptReq: %d, emptygpnReq: %d, "
                "emptygptResp: %d, emptygpnResp: %d.\n", __func__, emptygptReq,
                                    emptygpnReq, emptygptResp, emptygpnResp);
    return empty;
}

Port&
RouterEngine::getPort(const std::string& if_name, PortID idx)
{
    if (if_name == "gpt_req_side") {
        return gptReqPorts[idx];
    } else if (if_name == "gpt_resp_side") { 
        return gptRespPorts[idx];
    } else if (if_name == "gpn_req_side") {
        return gpnReqPorts[idx];
    } else if (if_name == "gpn_resp_side") {
        return gpnRespPorts[idx];
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

bool
RouterEngine::GPTReqPort::recvTimingResp(PacketPtr pkt)
{
    panic("Not implemented yet!");
    return 0;
}

void
RouterEngine::GPTReqPort::recvReqRetry()
{
    panic_if(blockedPacket == nullptr,
            "Received retry without a blockedPacket.");

    DPRINTF(RouterEngine, "%s: ReqPort %d received a reqRetry. "
                "blockedPacket: %s.\n", __func__, _id, blockedPacket->print());
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);
    if (blockedPacket == nullptr) {
        DPRINTF(RouterEngine, "%s: blockedPacket sent successfully.\n",
                                                                    __func__);
        owner->recvReqRetry();
    }
}

bool
RouterEngine::GPNReqPort::recvTimingResp(PacketPtr pkt)
{
    panic("Not implemented yet!");
    return 0;
}

void
RouterEngine::GPNReqPort::recvReqRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    sendPacket(pkt);

    owner->wakeUpInternal();
}

void
RouterEngine::GPNReqPort::sendPacket(PacketPtr pkt) {
    panic_if(blocked(), "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        DPRINTF(RouterEngine, "%s: The GPNReq port is blocked.\n", __func__);
        blockedPacket = pkt;
    }
}

void
RouterEngine::GPTReqPort::sendPacket(PacketPtr pkt) {
    panic_if(blocked(), "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        DPRINTF(RouterEngine, "%s: The GPTReq port is blocked.\n", __func__);
        blockedPacket = pkt;
    }
}

Tick 
RouterEngine::GPTRespPort::recvAtomic(PacketPtr pkt) {
    panic("Not implemented yet!");
}

void
RouterEngine::GPTRespPort::checkRetryReq()
{
    if (needSendRetryReq) {
        needSendRetryReq = false;
        sendRetryReq();
    }
}

bool
RouterEngine::GPTRespPort::recvTimingReq(PacketPtr pkt) {
    if (!owner->handleRequest(id(), pkt)) {
        DPRINTF(RouterEngine, "%s: Router Rejected the packet %s.\n",
                    __func__, pkt->getAddr());
        needSendRetryReq = true;
        return false;
    }
    return true;
}

void
RouterEngine::recvReqRetry()
{
    DPRINTF(RouterEngine, "%s: Received a reqRetry.\n", __func__);
    if (!nextExternalRequestEvent.scheduled()) {
        schedule(nextExternalRequestEvent, nextCycle());
    }
}

bool
RouterEngine::handleRequest(PortID portId, PacketPtr pkt)
{
    auto &queue = gptReqQueues[portId];
    bool accepted = false;
    if (queue.size() < gptQSize) {
        DPRINTF(RouterEngine, "%s: gptReqQueues[%lu] size is: %d.\n",
                                            __func__, portId, queue.size());
        gptReqQueues[portId].push(pkt);
        accepted = true;
    } else {
         DPRINTF(RouterEngine, "%s: gptReqQueues[%lu] is full.\n",
                                                            __func__, portId);
        accepted = false;
    }

    if (accepted && (!nextGPTGPNEvent.scheduled())) {
        schedule(nextGPTGPNEvent, nextCycle());
    }
    DPRINTF(RouterEngine, "%s: GPT sent req to router: accepted: %d.\n",
                                                        __func__, accepted);
    return accepted;
}

void
RouterEngine::processNextGPTGPNEvent()
{
    bool found = false;
    bool queues_none_empty = false;
    DPRINTF(RouterEngine, "%s: Trying to send a request from GPT to GPN.\n",
                                                                    __func__);
    for (auto &queue: gptReqQueues) {
        if (!queue.second.empty()) {
            PacketPtr pkt = queue.second.front();
            Addr pkt_addr = pkt->getAddr();
            for (int i = 0; i < gpnReqPorts.size(); i++) {
                AddrRangeList addr_list = routerAddrMap[gpnReqPorts[i].id()];
                if ((contains(addr_list, pkt_addr))) {
                    if (gpnRespQueues[gpnReqPorts[i].id()].size() < gpnQSize) {
                        gpnRespQueues[gpnReqPorts[i].id()].push(pkt);
                        DPRINTF(RouterEngine, "%s: Pushing the pkt %s to  "
                                "gpnRespQueue[%d]. gpnRespQueue size is: %d\n",
                                __func__, pkt->getAddr(), i,
                                gpnRespQueues[gpnReqPorts[i].id()].size());
                        // stats.internalTrafficHist[gpnReqPorts[i].id()]->sample(gpnRespQueues[gpnReqPorts[i].id()].size());
                        queue.second.pop();
                        DPRINTF(RouterEngine, "%s: gptReqQueue size is: %d.\n",
                                                __func__, queue.second.size());
                        found |= true;
                        if ((!nextInternalRequestEvent.scheduled())) {
                            schedule(nextInternalRequestEvent, nextCycle());
                        }
                    // queue is full
                    } else {
                         DPRINTF(RouterEngine, "%s: gpnRespQueue[%d] is full."
                            "\n", __func__, pkt->getAddr(), i);
                        found |= false;
                    }
                }
            }
        }
        if (found) {
            checkGPTRetryReq();
        }
    }

    for (auto &queue: gptReqQueues)
    {
        if (!queue.second.empty()) {
            queues_none_empty = true;
        }
    }

    if (queues_none_empty) {
        DPRINTF(RouterEngine, "%s: The gptReqQueues is not empty.\n",
                                                                    __func__);
    } else {
        DPRINTF(RouterEngine, "%s: The gptReqQueues is empty.\n", __func__);
    }

    if (queues_none_empty && (!nextGPTGPNEvent.scheduled())) {
        schedule(nextGPTGPNEvent, nextCycle());
    }
}

void
RouterEngine::processNextInternalRequestEvent()
{
    DPRINTF(RouterEngine, "%s: Sending a request between two routers.\n",
                                                                    __func__);
    bool none_empty_queue = false;
    for (auto &queue: gpnRespQueues) {
        if (!queue.second.empty()) {
            if (!gpnReqPorts[queue.first].blocked()) {
                if  ((curCycle() - 
                    internalLatency[gpnReqPorts[queue.first].id()]) 
                    < routerLatency) {
                    continue;
                } 
                stats.internalAcceptedTraffic[gpnReqPorts[queue.first].id()]++;
                PacketPtr pkt = queue.second.front();
                DPRINTF(RouterEngine, "%s: Sending packet %s to router: %d.\n",
                    __func__, pkt->getAddr(), gpnReqPorts[queue.first].id());
                gpnReqPorts[queue.first].sendPacket(pkt);
                queue.second.pop();
                internalLatency[gpnReqPorts[queue.first].id()] = curCycle();
            } 
            else {
                DPRINTF(RouterEngine, "%s: port id %d is blocked.\n",
                    __func__, gpnReqPorts[queue.first].id());
                stats.internalBlockedTraffic[gpnReqPorts[queue.first].id()]++;
            }
        }
    }

    for (auto &queue: gpnRespQueues) {
        if (!queue.second.empty()) {
            none_empty_queue = true;
            break;
        }
    }

    if (none_empty_queue) {
        DPRINTF(RouterEngine, "%s: The gpnRespQueues is not empty.\n",
                                                                    __func__);
    } else {
        DPRINTF(RouterEngine, "%s: The gpnRespQueues is empty.\n", __func__);
    }

    Tick next_schedule = nextCycle() + cyclesToTicks(routerLatency);
    for (auto itr = internalLatency.begin(); 
            itr != internalLatency.end();
            itr++)
    {
        if (cyclesToTicks(itr->second + routerLatency) <  next_schedule) {
            if ((itr->second + routerLatency) <=  curCycle()) {
                next_schedule =  nextCycle();
                break;
            } else {
                next_schedule = std::min(
                                    cyclesToTicks(itr->second + routerLatency),
                                    next_schedule);
            }
        } 
    }


    if (none_empty_queue && (!nextInternalRequestEvent.scheduled())) {
        schedule(nextInternalRequestEvent, next_schedule);
    }
}

void
RouterEngine::GPTRespPort::recvFunctional(PacketPtr pkt) {
    panic("Not implemented yet!");
}

void
RouterEngine::GPTRespPort::recvRespRetry() {
    panic("Not implemented yet!");
}

Tick 
RouterEngine::GPNRespPort::recvAtomic(PacketPtr pkt) {
    panic("Not implemented yet!");
}

void
RouterEngine::GPNRespPort::checkRetryReq() {
    if (needSendRetryReq) {
        needSendRetryReq = false;
        sendRetryReq();
    }
}

bool
RouterEngine::GPNRespPort::recvTimingReq(PacketPtr pkt) {
    if (!owner->handleRemoteRequest(id(), pkt)) {
        DPRINTF(RouterEngine, "%s: Router Rejected the packet %s.\n",
                    __func__, pkt->getAddr());
        needSendRetryReq = true;
        return false;
    }
    return true;
}

bool
RouterEngine::handleRemoteRequest(PortID id, PacketPtr pkt) {
    bool accepted = false;
    if (gpnReqQueues[id].size() < gpnQSize) {
        gpnReqQueues[id].push(pkt);
        accepted = true;
    } else {
        accepted = false;
    }

    if (accepted && (!nextGPNGPTEvent.scheduled())) {
        schedule(nextGPNGPTEvent, nextCycle());
    }

    DPRINTF(RouterEngine, "%s: The remote packet: %s is accepted: %d.\n",
                                        __func__, pkt->getAddr(), accepted);
    return accepted;
}

void
RouterEngine::processNextGPNGPTEvent()
{
    bool found = false;
    bool queues_none_empty = false;
    for (auto &queue: gpnReqQueues) {
        if (!queue.second.empty()) {
            PacketPtr pkt = queue.second.front();
            Addr pkt_addr = pkt->getAddr();
            for (int i = 0; i < gptReqPorts.size(); i++) {
                AddrRangeList addr_list = gptAddrMap[gptReqPorts[i].id()];
                if ((contains(addr_list, pkt_addr))) {
                    if (gptRespQueues[gptReqPorts[i].id()].size() < gptQSize) {
                        gptRespQueues[gptReqPorts[i].id()].push(pkt);
                        DPRINTF(RouterEngine, "%s: The size of "
                                    "gptRespQueues[%d] is %d.\n", __func__, i,
                                    gptRespQueues[gptReqPorts[i].id()].size());
                        DPRINTF(RouterEngine,
                                    "%s: Sending pkt %s to GPT %d.\n",
                                    __func__, pkt->getAddr(), i);
                        queue.second.pop();
                        found |= true;
                        if ((!nextExternalRequestEvent.scheduled())) {
                            schedule(nextExternalRequestEvent, nextCycle());
                        }
                    } else {
                        DPRINTF(RouterEngine,
                                    "%s: gptRespQueues[%d] is full.\n",
                                    __func__, pkt->getAddr(), i);
                        found |= false;
                    }
                }
            }
        }
        if (found) {
            checkGPNRetryReq();
        }
    }

    for (auto &queue: gpnReqQueues) {
        if (!queue.second.empty()) {
            queues_none_empty = true;
        }
    }

    if (queues_none_empty) {
        DPRINTF(RouterEngine, "%s: gpnReqQueues is not empty.\n", __func__);
    } else {
        DPRINTF(RouterEngine, "%s: gpnReqQueues is empty.\n", __func__);
    }

    if (queues_none_empty && (!nextGPNGPTEvent.scheduled())) {
        schedule(nextGPNGPTEvent, nextCycle());
    }
}

void
RouterEngine::processNextExternalRequestEvent()
{
    DPRINTF(RouterEngine, "%s: Sending the request to the GPT.\n", __func__);
    bool none_empty_queue = false;
    for (auto &queue: gptRespQueues) {
        if (!queue.second.empty()) {
            if (!gptReqPorts[queue.first].blocked()) {
                if ((curCycle() - 
                    externalLatency[gptReqPorts[queue.first].id()]) 
                    < routerLatency) {
                    continue;
                }
                stats.externalAcceptedTraffic[gptReqPorts[queue.first].id()]++;
                PacketPtr pkt = queue.second.front();
                DPRINTF(RouterEngine, "%s: gptRespQueues[%d] is not empty. "
                        "the size is: %d.\n", __func__,
                         gptReqPorts[queue.first].id() ,queue.second.size());
                DPRINTF(RouterEngine, "%s: Sending packet %s to GPT: %d.\n",
                    __func__, pkt->getAddr(),gptReqPorts[queue.first].id());
                gptReqPorts[queue.first].sendPacket(pkt);
                queue.second.pop();
                externalLatency[gptReqPorts[queue.first].id()] = curCycle();
            }
            else {
                stats.externalBlockedTraffic[gptReqPorts[queue.first].id()]++;
            }
        }
    }

    for (auto &queue: gptRespQueues) {
        DPRINTF(RouterEngine, "%s: gptRespQueues[%d] size is: %d.\n", __func__,
                        gptReqPorts[queue.first].id() ,queue.second.size());
        if (!queue.second.empty()) {
            none_empty_queue = true;
            break;
        }
    }

    if (none_empty_queue) {
        DPRINTF(RouterEngine, "%s: The gptRespQueues is not empty.\n",
                                                                    __func__);
    } else {
        DPRINTF(RouterEngine, "%s: The gptRespQueues is empty.\n", __func__);
    }

    Tick next_schedule = cyclesToTicks(curCycle() + routerLatency);
    for (auto itr = externalLatency.begin(); 
        itr != externalLatency.end(); itr++)
    {
        if (cyclesToTicks(itr->second + routerLatency) <  next_schedule) {
            if ((itr->second + routerLatency) <=  curCycle()) {
                next_schedule =  nextCycle();
                break;
            } else {
                next_schedule = std::min(
                                    cyclesToTicks(itr->second + routerLatency),
                                    next_schedule);
            }
        } 
    }

    if (none_empty_queue) {
        if (!nextExternalRequestEvent.scheduled()) {
            schedule(nextExternalRequestEvent, next_schedule);
        }
    }
}

void
RouterEngine::GPNRespPort::recvFunctional(PacketPtr pkt)
{
    panic("Not implemented yet!");
}

void
RouterEngine::GPNRespPort::recvRespRetry()
{
    panic("Not implemented yet!");
}

void
RouterEngine::wakeUpInternal()
{
    if ((!nextInternalRequestEvent.scheduled())) {
        for (auto &queue: gpnRespQueues) {
            if (!queue.second.empty()) {
                schedule(nextInternalRequestEvent, nextCycle());
                return;
            }
        }
    }
}

void
RouterEngine::checkGPTRetryReq()
{
    for (int i = 0; i < gptRespPorts.size(); i++) {
        gptRespPorts[i].checkRetryReq();
    }
}

void
RouterEngine::checkGPNRetryReq()
{
    for (int i = 0; i < gpnRespPorts.size(); i++) {
        gpnRespPorts[i].checkRetryReq();
    }
}

RouterEngine::RouterEngineStat::RouterEngineStat(RouterEngine &_router)
    : statistics::Group(&_router),
    router(_router),
    ADD_STAT(internalBlockedTraffic, statistics::units::Count::get(),
             "Number of packets blocked between routers."),
    ADD_STAT(externalBlockedTraffic, statistics::units::Count::get(),
             "Number of external packets blocked."),
    ADD_STAT(internalAcceptedTraffic, statistics::units::Count::get(),
             "Number of packet passed between routers."),
    ADD_STAT(externalAcceptedTraffic, statistics::units::Count::get(),
             "Number of external packets passed.")
{}

void
RouterEngine::RouterEngineStat::regStats()
{
    using namespace statistics;

    internalBlockedTraffic.init(router.gpnReqPorts.size());
    externalBlockedTraffic.init(router.gptReqPorts.size());
    internalAcceptedTraffic.init(router.gpnReqPorts.size());
    externalAcceptedTraffic.init(router.gptReqPorts.size());

    for (uint32_t i = 0; i < router.gpnReqPorts.size(); ++i) {
        internalTrafficHist.push_back(new statistics::Histogram(this));
        internalTrafficHist[i]
            ->init(64)
            .name(csprintf("internal_traffic_hist"))
            .desc("");
    }
}
}// namespace gem5