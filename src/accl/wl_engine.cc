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


WLEngine::WLEngine(const WLEngineParams &params):
    ClockedObject(params),
    nextWLReadEvent([this]{processNextWLReadEvent; }, name()),
    nextWLReduceEvent([this]{processNextWLReduceEvent; }, name()),
    queueSize(params.wlQueueSize) //add this to .py
{
    wlReadQueue(queueSize);
    wlWriteQueue(queueSize);
}

bool WLEngine::WLRespPort::recvTimingReq(PacketPtr pkt)
{
    if (!this->handleWLUpdate(pkt)){
        return false;
    }
    return true;
}

bool WLEngine::handleWLUpdate(PacketPtr pkt){
    auto queue = wlReadQueue;
    if (queue->blocked()){
        queue->sendPktRetry = true;
        return false;
    } else
        queue->push(pkt);

    if(!nextWLReadEvent.scheduled()){
        schedule(nextWLReadEvent, nextCycle());
    }
    return true;
}


void WLEngine::processNextWLReadEvent(){
    auto queue = wlReadQueue;
    memPort = WLMemPort
    while(!queue.empty()){ //create a map instead of front
        auto pkt = queue.front()
        /// conver to ReadReq
        RequestPtr req = std::make_shared<Request>(pkt->getAddr(), 64, 0 ,0);
        PacketPtr memPkt = new Packet(req, MemCmd::ReadReq);
        if (!memPort->blocked()){
            memPort->sendPacket(memPkt);
            break;
        }
    }

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

    owner->wakeUp(); //TODO
}

virtual bool
WLEngine::WLMemPort::recvTimingResp(PacketPtr pkt)
{
    return this->handleMemResp(pkt);
}

bool
WLEngine::handleMemResp(PacktPtr pkt)
{
    auto queue = applyWriteQueue;
        if (queue->blocked()){
            sendPktRetry = true;
            return false;
        } else
            queue->push(writePkt);

        if(!nextWLReduceEvent.scheduled()){
            schedule(nextWLReduceEvent, nextCycle());
        }
        return true;
    return true;
}

void
WLEngine::processNextWLReduceEvent(){
    auto queue = wlWriteQueue;
    auto updateQ = wlReadQueue;
    memPort = WLMemPort;
    applyPort = WLReqPort;
    while(!queue.empty()){
        auto update = updateQ.pop()
        if (!updateQ->blocked() & updateQ->sendPktRetry){
            WLRespPort->trySendRetry();
            updateQ->sendPktRetry = false;
        }
        auto pkt = queue.front()
        uint64_t* updatePtr = pkt->getPtr<uint64_t>();
        uint64_t* data = pkt->getPtr<uint64_t>();
        uint32_t* value = updatePtr;
        uint32_t* temp_prop = prop + 1;
        if (*value != *prop){
            //update prop with temp_prop
            *temp_prop = min(*value , *temp_prop);
            RequestPtr req = std::make_shared<Request>(pkt->getAddr(), 64, 0 ,0);
            PacketPtr writePkt = new Packet(req, MemCmd::WriteReq);
            writePkt->setData(data);
            if (!memPort->blocked() && !applyPort->blocked()){
                memPort->sendPacket(pkt);
                applyPort->sendPacket(pkt);
                queue.pop();
                if (!queue->blocked() && queue->sendPktRetry){
                    memPort->trySendRetry();
                    queue->sendPktRetry = false;
                }
            }
            else
                break;
        }
        else{
            queue.pop();
            if (!queue->blocked() && queue->sendPktRetry){
                memPort->trySendRetry();
                queue->sendPktRetry = false;
            }

        }

    }

}

void
WLEngine::WLRespPort::trySendRetry()
{
    sendRetryReq();
}

void
WLEngine::WLMemPort::trySendRetry()
{
    sendRetryResp();
}