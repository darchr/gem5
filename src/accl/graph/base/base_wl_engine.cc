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

#include "accl/graph/base/base_wl_engine.hh"

#include <string>

namespace gem5
{

BaseWLEngine::BaseWLEngine(const BaseWLEngineParams &params):
    BaseEngine(params),
    nextWLReadEvent([this]{ processNextWLReadEvent(); }, name()),
    nextWLReduceEvent([this]{ processNextWLReduceEvent(); }, name())
{}

bool
BaseWLEngine::handleWLUpdate(PacketPtr pkt)
{
    updateQueue.push(pkt);
    if(!nextWLReadEvent.scheduled()) {
        schedule(nextWLReadEvent, nextCycle());
    }
    return true;
}

void BaseWLEngine::processNextWLReadEvent()
{
    PacketPtr pkt = updateQueue.front();

    Addr addr = pkt->getAddr();
    Addr req_addr = (addr / 64) * 64;
    Addr req_offset = addr % 64;

    PacketPtr memPkt = getReadPacket(req_addr, 64, requestorId);
    requestOffsetMap[request] = req_offset;

    if (memPortBlocked()) {
        sendMemReq(memPkt)
        updateQueue.pop();
    }
    if (!queue.empty() && !nextWLReadEvent.scheduled()) {
        schedule(nextWLReadEvent, nextCycle());
    }
}

bool
BaseWLEngine::handleMemResp(PacketPtr pkt)
{
    responseQueue.push(pkt);
    if(!nextWLReduceEvent.scheduled()){
        schedule(nextWLReduceEvent, nextCycle());
    }
    return true;
}

void
BaseWLEngine::processNextWLReduceEvent(){
    PacketPtr update = updateQ.front();
    uint8_t* value = update->getPtr<uint8_t>();
    PacketPtr pkt = queue.front();
    uint8_t* data = pkt->getPtr<uint8_t>();
    RequestPtr request = pkt->req;
    int request_offset = requestOffset[request];
    WorkListItem wl =  memoryToWorkList(data + request_offset);
    uint32_t temp_prop = wl.temp_prop;
    if (temp_prop != *value){
        //update prop with temp_prop
        if(*value < temp_prop){
            temp_prop = *value;
        }
        // if (!memPort.blocked() && !applyPort.blocked()){
        wl.temp_prop = temp_prop;
        uint8_t* wlItem = workListToMemory(wl);
        memcpy(data + request_offset, wlItem, sizeof(WorkListItem));
        PacketPtr writePkt  =
        getWritePacket(pkt->getAddr(), 64, data, requestorId);
        if (sendMemReq(writePkt) &&
            sendWLNotif(writePkt->getAddr())) {
            queue.pop();
            if (!queue.blocked() && queue.sendPktRetry){
                queue.sendPktRetry = false;
            }
            updateQ.pop();
            if (!updateQ.blocked() & updateQ.sendPktRetry){
                // respPort.trySendRetry();
                updateQ.sendPktRetry = false;
            }
        }
    }
    else{
        queue.pop();
        if (!queue.blocked() && queue.sendPktRetry){
            queue.sendPktRetry = false;
        }
        updateQ.pop();
        if (!updateQ.blocked() & updateQ.sendPktRetry){
            updateQ.sendPktRetry = false;
        }

    }
    if (!queue.empty() && !nextWLReduceEvent.scheduled()){
            schedule(nextWLReduceEvent, nextCycle());
    }
}

}
