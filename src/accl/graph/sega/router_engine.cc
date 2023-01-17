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

namespace gem5
{
RouterEngine::RouterEngine(const Params &params):
  ClockedObject(params),
  gptQSize(params.gpt_queue_size),
  gpnQSize(params.gpn_queue_size),
  nextInteralGPTGPNEvent([this] { processNextInteralGPTGPNEvent(); }, name()),
  nextRemoteGPTGPNEvent([this] { processNextRemoteGPTGPNEvent(); }, name()),
  nextInteralGPNGPTEvent([this] { processNextInteralGPNGPTEvent(); }, name()),
  nextRemoteGPNGPTEvent([this] { processNextRemoteGPNGPTEvent(); }, name())
{

    for (int i = 0; i < params.port_gpt_req_side_connection_count; ++i) {
        gptReqPorts.emplace_back(
                    name() + ".gpt_req_side" + std::to_string(i), this, i);
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
        for (auto &addrRange: gptReqPorts[i].getAddrRanges()) {
            std::cout<< name()<<", "<<__func__<<addrRange.to_string()<<std::endl;
        }
    }
}

void
RouterEngine::startup()
{
    for (int i = 0; i < gpnReqPorts.size(); i++) {
        routerAddrMap[gpnReqPorts[i].id()] = gpnReqPorts[i].getAddrRanges();
        for (auto &addrRange: gpnReqPorts[i].getAddrRanges()) {
            std::cout<< name()<<", "<<__func__<<addrRange.to_string()<<std::endl;
        }
    }
    std::cout<<"******************"<<std::endl;
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
RouterEngine::GPTReqPort::recvTimingResp(PacketPtr pkt) {
    panic("Not implemented yet!");
    return 0;
}

bool
RouterEngine::GPNReqPort::recvTimingResp(PacketPtr pkt) {
    panic("Not implemented yet!");
    return 0;
}

void
RouterEngine::GPNReqPort::recvReqRetry() {
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);

    sendPacket(blockedPacket);
    blockedPacket = nullptr;

    owner->wakeUpInternal();
}

void
RouterEngine::GPTReqPort::recvReqRetry() {
    assert(blockedPacket != nullptr);

    sendPacket(blockedPacket);
    blockedPacket = nullptr;

    owner->wakeUpExternal();
}

void
RouterEngine::GPNReqPort::sendPacket(PacketPtr pkt) {
    panic_if(blocked(), "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

void
RouterEngine::GPTReqPort::sendPacket(PacketPtr pkt) {
    panic_if(blocked(), "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
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

void
RouterEngine::checkRetryExternal()
{
    for (int i = 0; i < gptRespPorts.size(); i++) {
        gptRespPorts[i].checkRetryReq();
    }
}

bool
RouterEngine::GPTRespPort::recvTimingReq(PacketPtr pkt) {
    if (!owner->handleRequest(id(), pkt)) {
        needSendRetryReq = true;
        return false;
    }
    return true;
}

bool
RouterEngine::handleRequest(PortID portId, PacketPtr pkt)
{
    auto queue = gptReqQueues[portId];
    bool accepted = false;
    if (queue.size() < gptQSize) {
        gptReqQueues[portId].push(pkt);
        accepted = true;
    } else {
        accepted = false;
    }

    if(accepted && (!nextInteralGPTGPNEvent.scheduled())) {
        schedule(nextInteralGPTGPNEvent, nextCycle());
    }

    return accepted;
}

void
RouterEngine::processNextInteralGPTGPNEvent()
{
    bool found = false;
    int queues_none_empty = 0;
    for (auto &queue: gptReqQueues) {
        if (!queue.second.empty()) {
            PacketPtr pkt = queue.second.front();
            Addr pkt_addr = pkt->getAddr();
            queues_none_empty += 1;
            for (int i = 0; i < gpnReqPorts.size(); i++) {
                AddrRangeList addr_list = routerAddrMap[gpnReqPorts[i].id()];
                if ((contains(addr_list, pkt_addr))) {
                    if (gpnRespQueues[gpnReqPorts[i].id()].size() < gpnQSize) {
                        gpnRespQueues[gpnReqPorts[i].id()].push(pkt);
                        queue.second.pop();
                        checkRetryExternal();
                        found = true;
                        queues_none_empty -= 1;
                        if (!queue.second.empty()) {
                            queues_none_empty += 1;
                        }
                        if ((!nextRemoteGPTGPNEvent.scheduled())) {
                            schedule(nextRemoteGPTGPNEvent, nextCycle());
                        }
                        break;
                    // queue is full
                    } else {
                        found = false;
                        break;
                    }
                }
            }
        }
        if (found) {
            break;
        }
    }

    if (queues_none_empty > 0) {
        schedule(nextInteralGPTGPNEvent, nextCycle());
    }
}

void
RouterEngine::processNextRemoteGPTGPNEvent()
{
    bool none_empty_queue = false;
    for (auto &queue: gpnRespQueues) {
        if (!queue.second.empty()) {
            if (!gpnReqPorts[queue.first].blocked()) {
                PacketPtr pkt = queue.second.front();
                gpnReqPorts[queue.first].sendPacket(pkt);
                queue.second.pop();
                break;
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
        schedule(nextRemoteGPTGPNEvent, nextCycle());
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
RouterEngine::GPNRespPort::checkRetryReq()
{
    if (needSendRetryReq) {
        needSendRetryReq = false;
        sendRetryReq();
    }
}

void
RouterEngine::checkRetryInternal()
{
    for (int i = 0; i < gpnRespPorts.size(); i++) {
        gpnRespPorts[i].checkRetryReq();
    }
}

bool
RouterEngine::GPNRespPort::recvTimingReq(PacketPtr pkt) {
    if (!owner->handleRemoteRequest(id(), pkt)) {
        needSendRetryReq = true;
        return false;
    }
    return true;
}

bool
RouterEngine::handleRemoteRequest(PortID id, PacketPtr pkt) {
    // std::queue<PacketPtr>& queue = routerAddrMap[id];
    // for (auto &itr: routerAddrMap) {
    //     if (itr.first == id) {

    //     }
    // }
    bool accepted = false;
    if (gpnReqQueues[id].size() < gpnQSize) {
        gpnReqQueues[id].push(pkt);
        accepted = true;
    } else {
        accepted = false;
    }

    if (accepted && (!nextInteralGPNGPTEvent.scheduled())) {
        schedule(nextInteralGPNGPTEvent, nextCycle());
    }

    return accepted;
}

void
RouterEngine::processNextInteralGPNGPTEvent()
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
                    if (gptRespQueues[gpnReqPorts[i].id()].size() < gptQSize) {
                        gptRespQueues[gpnReqPorts[i].id()].push(pkt);
                        queue.second.pop();
                        checkRetryInternal();
                        found = true;
                        if ((!nextRemoteGPNGPTEvent.scheduled())) {
                            schedule(nextRemoteGPNGPTEvent, nextCycle());
                        }
                        break;
                    } else {
                        found = false;
                        break;
                    }
                }
            }
        }
        if (found) {
            break;
        }
    }
    for (auto &queue: gpnReqQueues) {
        if (!queue.second.empty()) {
            queues_none_empty = true;
        }
    }

    if (queues_none_empty && !nextInteralGPNGPTEvent.scheduled()) {
        schedule(nextInteralGPNGPTEvent, nextCycle());
    }
}

void
RouterEngine::processNextRemoteGPNGPTEvent()
{
    bool none_empty_queue = false;
    for (auto &queue: gptRespQueues) {
        if (!queue.second.empty()) {
            if (!gptReqPorts[queue.first].blocked()) {
                PacketPtr pkt = queue.second.front();
                gptReqPorts[queue.first].sendPacket(pkt);
                queue.second.pop();
                break;
            }
        }
    }

    for (auto &queue: gptRespQueues) {
        if (!queue.second.empty()) {
            none_empty_queue = true;
            break;
        }
    }

    if (none_empty_queue) {
        schedule(nextRemoteGPNGPTEvent, nextCycle());
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
RouterEngine::wakeUpExternal()
{
    if (!nextRemoteGPNGPTEvent.scheduled()) {
        for (auto &queue: gptRespQueues) {
            if (!queue.second.empty()) {
                schedule(nextRemoteGPNGPTEvent, nextCycle());
                return;
            }
        }
    }
}

void
RouterEngine::wakeUpInternal()
{
    if (!nextRemoteGPTGPNEvent.scheduled()) {
        for (auto &queue: gpnRespQueues) {
            if (!queue.second.empty()) {
                schedule(nextRemoteGPTGPNEvent, nextCycle());
                return;
            }
        }
    }
}

}// namespace gem5
