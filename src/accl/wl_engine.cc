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

#include "accl/wl_engine.hh"

#include <string>

namespace gem5
{

WLEngine::WLEngine(const WLEngineParams &params):
    ClockedObject(params),
    system(params.system),
    requestorId(system->getRequestorId(this)),
    reqPort(name() + ".reqPort", this),
    respPort(name() + ".respPort", this),
    memPort(name() + ".memPort", this),
    nextWLReadEvent([this]{processNextWLReadEvent; }, name()),
    nextWLReduceEvent([this]{processNextWLReduceEvent; }, name())
{
    updateQueue(params.wlQueueSize);
    responseQueue(params.wlQueueSize);
}

Port &
WLEngine::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "reqPort") {
        return reqPort;
    } else if (if_name == "respPort") {
        return respPort;
    } else if (if_name == "memPort") {
        return memPort;
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

AddrRangeList
WLEngine::WLRespPort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

bool WLEngine::WLRespPort::recvTimingReq(PacketPtr pkt)
{
    if (!this->handleWLUpdate(pkt)){
        return false;
    }
    return true;
}

void
WLEngine::WLRespPort::trySendRetry()
{
    sendRetryReq();
}

void
WLEngine::WLMemPort::sendPacket(PacketPtr pkt)
{
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
        _blocked = true;
    }
}

void
WLEngine::WLMemPort::recvReqRetry()
{
    // We should have a blocked packet if this function is called.
    assert(_blocked && blockedPacket != nullptr);
    _blocked = false;
    sendPacket(blockedPacket);
    blockedPacket = nullptr;
}

virtual bool
WLEngine::WLMemPort::recvTimingResp(PacketPtr pkt)
{
    return this->handleMemResp(pkt);
}

void
WLEngine::WLMemPort::trySendRetry()
{
    sendRetryResp();
}

void
WLEngine::WLReqPort::recvReqRetry()
{
    // We should have a blocked packet if this function is called.
    assert(_blocked && blockedPacket != nullptr);
    _blocked = false;
    sendPacket(blockedPacket);
    blockedPacket = nullptr;
}

void
WLEngine::WLReqPort::sendPacket(PacketPtr pkt)
{
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
        _blocked = true;
    }
}

AddrRangeList
WLEngine::getAddrRanges() const
{
    return memPort.getAddrRanges();
}

bool WLEngine::handleWLUpdate(PacketPtr pkt){
    auto queue = updateQueue;
    if (queue.blocked()){
        queue.sendPktRetry = true;
        return false;
    } else
        queue.push(pkt);

    if(!nextWLReadEvent.scheduled()){
        schedule(nextWLReadEvent, nextCycle());
    }
    return true;
}

void WLEngine::processNextWLReadEvent(){
    auto queue = updateQueue;
    auto memPort = WLMemPort;
    while (!queue.empty()){ //create a map instead of front
        auto pkt = queue.front()
        /// conver to ReadReq
        Addr req_addr = (pkt->getAddr() / 64) * 64;
        int req_offset = (pkt->getAddr()) % 64;
        RequestPtr request =
            std::make_shared<Request>(req_addr, 64, 0 ,0);
        PacketPtr memPkt = new Packet(req, MemCmd::ReadReq);
        requestOffset[request] = req_offset;
        if (!memPort.blocked()){
            queue.pop()
            memPort.sendPacket(memPkt);
            break;
        }
    }
    if(!queue.empty() && !nextWLReadEvent.scheduled()){
        schedule(nextWLReadEvent, nextCycle());
    }
}

bool
WLEngine::handleMemResp(PacktPtr pkt)
{
    auto queue = responseQueue;
        if (queue.blocked()){
            sendPktRetry = true;
            return false;
        } else
            queue.push(writePkt);

        if(!nextWLReduceEvent.scheduled()){
            schedule(nextWLReduceEvent, nextCycle());
        }
        return true;
    return true;
}

void
WLEngine::processNextWLReduceEvent(){
    auto queue = responseQueue;
    auto updateQ = updateQueue;
    applyPort = reqPort;
    auto update = updateQ.front();
    auto value = update->getPtr<uint8_t>();
    auto pkt = queue.front();
    uint8_t* data = pkt->getPtr<uint8_t>();
    RequestPtr request = pkt->req;
    int request_offset = requestOffset[request];
    WorkListItem wl =  memoryToWorkList(data + request_offset)
    uint32_t temp_prop = wl.temp_prop;
    if (temp_prop != *value){
        //update prop with temp_prop
        temp_prop = std::min(value , temp_prop);
        if (!memPort.blocked() && !applyPort.blocked()){
            wl.temp_prop = temp_prop;
            uint8_t* wlItem = workListToMemory(wl);
            memcpy(data + request_offset, wlItem, sizeof(WorkListItem));
            PacketPtr writePkt  =
            getWritePacket(pkt->getAddr(), 64, data, requestorId);
            memPort.sendPacket(writePkt);
            applyPort.sendPacket(writePkt);
            queue.pop();
            if (!queue.blocked() && queue.sendPktRetry){
                memPort.trySendRetry();
                queue.sendPktRetry = false;
            }
            updateQ.pop();
            if (!updateQ.blocked() & updateQ.sendPktRetry){
                respPort.trySendRetry();
                updateQ.sendPktRetry = false;
            }
        }
    }
    else{
        queue.pop();
        if (!queue.blocked() && queue.sendPktRetry){
            memPort.trySendRetry();
            queue.sendPktRetry = false;
        }
        updateQ.pop()
        if (!updateQ.blocked() & updateQ.sendPktRetry){
            respPort.trySendRetry();
            updateQ.sendPktRetry = false;
        }

    }
    if (!queue.empty() && !nextWLReduceEvent.scheduled()){
            schedule(nextWLReduceEvent, nextCycle());
    }
}

}
