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

#include "accl/apply.hh"

#include <string>

Apply::Apply(const ApplyParams &params):
    ClockedObject(params),
    reqPort(name() + ".reqPort", this),
    respPort(name() + ".respPort", this),
    memPort(name() + ".memPort", this),
    nextApplyEvent([this]{processNextApplyEvent; }, name()),
    nextApplyCheckEvent([this]{processNextApplyCheckEvent; }, name()),
    queueSize(params.applyQueueSize) //add this to .py
{
    applyReadQueue(queueSize);
    applyWriteQueue(queueSize);
}

Port &
Apply::getPort(const std::string &if_name, PortID idx)
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
Apply::ApplyRespPort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

bool Apply::ApplyRespPort::recvTimingReq(PacketPtr pkt)
{
    if (!this->handleWL(pkt)){
        return false;
    }
    return true;
}

void
Apply::ApplyRespPort::trySendRetry()
{
    sendRetryReq();
}


virtual bool
Apply::ApplyMemPort::recvTimingResp(PacketPtr pkt)
{
    return this->handleMemResp(pkt);
}

void
WLEngine::ApplyMemPort::sendPacket(PacketPtr pkt)
{
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
        _blocked = true;
    }
}

void
Apply::ApplyMemPort::trySendRetry()
{
    sendRetryReq();
}

void
Apply::ApplyMemPort::recvReqRetry()
{
    _blocked = false;
    sendPacket(blockedPacket);
    blockedPacket = nullptr;
}

void
WLEngine::ApplyRequestPort::sendPacket(PacketPtr pkt)
{
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
        _blocked = true;
    }
}

void
Apply::ApplyRequestPort::recvReqRetry()
{
    _blocked = false;
    sendPacket(blockedPacket);
    blockedPacket = nullptr;
}

AddrRangeList
Apply::getAddrRanges() const
{
    return memPort.getAddrRanges();
}

bool Apply::handleWL(PacketPtr pkt){
    auto queue = applyReadQueue;
    if (queue->blocked()){
        sendPktRetry = true;
        return false;
    } else
        queue->push(pkt);
    if(!nextApplyCheckEvent.scheduled()){
        schedule(nextApplyCheckEvent, nextCycle());
    }
    return true;
}

void Apply::processNextApplyCheckEvent(){
    auto queue = applyReadQueue;
    while(!queue.empty()){
        if(!memPort->blocked()){
            auto pkt = queue.pop();
            if(queue->sendPktRetry && !queue->blocked()){
                    respPort->trySendRetry();
                    queue->sendPktRetry = false;
            }
            // conver to ReadReq
            RequestPtr req = std::make_shared<Request>(pkt->getAddr(), 64, 0 ,0);
            PacketPtr memPkt = new Packet(req, MemCmd::ReadReq);
            memPort->sendPacket(memPkt);
        }
        else
            return;
    }
}

bool
Apply::handleMemResp(PacktPtr pkt)
{
    auto queue = applyWriteQueue;

        if (queue->blocked()){
            sendPktRetry = true;
            return false;
        } else
            queue->push(writePkt);

        if(!nextApplyEvent.scheduled()){
            schedule(nextApplyEvent, nextCycle());
        }
        return true;
    return true;
}

void
Apply::processNextApplyEvent(){
    auto queue = applyWriteQueue;
    while(!queue.empty()){
        auto pkt = queue.front();
        uint64_t* data = pkt->getPtr<uint64_t>();
        uint32_t* prop = data;
        uint32_t* temp_prop = prop + 1;
        if (*temp_prop != *prop){
            //update prop with temp_prop
            *prop = min(*prop , *temp_prop);
            RequestPtr req =
                std::make_shared<Request>(pkt->getAddr(), 64, 0 ,0);
            PacketPtr writePkt = new Packet(req, MemCmd::WriteReq);
            writePkt->setData(data);
            if (!memPort->blocked() && !reqPort->blocked()){ //re-think this
                memPort->sendPacket(pkt);
                applyReqPort->sendPacket(pkt);
                queue.pop();
                if(queue->sendPktRetry && !queue->blocked()){
                    memPort->trySendRetry();
                    queue->sendPktRetry = false;
                }
            }
            else
                break;
        }
        else{
            queue.pop();
            if(queue->sendPktRetry && !queue->blocked()){
                memPort->trySendRetry();
                queue->sendPktRetry = false;
            }
        }
    }
}