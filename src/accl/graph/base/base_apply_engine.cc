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

#include "accl/base_apply_engine.hh"

#include <string>

#include "accl/graph/base/util.hh"

namespace gem5
{

BaseApplyEngine::BaseApplyEngine(const BaseApplyEngineParams &params):
    ClockedObject(params),
    requestorId(-1),
    applyReadQueue(params.applyQueueSize),
    applyWriteQueue(params.applyQueueSize),
    nextApplyCheckEvent([this]{ processNextApplyCheckEvent(); }, name()),
    nextApplyEvent([this]{ processNextApplyEvent(); }, name())
{}

Port &
BaseApplyEngine::getPort(const std::string &if_name, PortID idx)
{
        return SimObject::getPort(if_name, idx);
}

RequestorID
BaseApplyEngine::getRequestorId()
{
    return requestorId;
}

void
BaseApplyEngine::setRequestorId(RequestorID requestorId)
{
    this->requestorId = requestorId;
}

bool BaseApplyEngine::handleWL(PacketPtr pkt){
    auto queue = applyReadQueue;
    if (queue.blocked()){
        queue.sendPktRetry = true;
        return false;
    } else{
        queue.push(pkt);
    }
    if (!nextApplyCheckEvent.scheduled()){
        schedule(nextApplyCheckEvent, nextCycle());
    }
    return true;
}

void BaseApplyEngine::processNextApplyCheckEvent(){
    auto queue = applyReadQueue;
    // if (!memPort.blocked()){
    PacketPtr pkt = queue.front();
    // if (queue.sendPktRetry && !queue.blocked()){
    //         // respPort.trySendRetry();
    //         queue.sendPktRetry = false;
    // }
    // conver to ReadReq
    Addr req_addr = (pkt->getAddr() / 64) * 64;
    int req_offset = (pkt->getAddr()) % 64;
    RequestPtr request = std::make_shared<Request>(req_addr, 64, 0 ,0);
    PacketPtr memPkt = new Packet(request, MemCmd::ReadReq);
    requestOffset[request] = req_offset;
    if (parent.sendMemReq(memPkt)){
        queue.pop();
    }
    if (!queue.empty() &&  !nextApplyCheckEvent.scheduled()){
        schedule(nextApplyCheckEvent, nextCycle());
    }
}

bool
BaseApplyEngine::handleMemResp(PacketPtr pkt)
{
    auto queue = applyWriteQueue;

        if (queue.blocked()){
            queue.sendPktRetry = true;
            return false;
        } else
            queue.push(pkt);

        if(!nextApplyEvent.scheduled()){
            schedule(nextApplyEvent, nextCycle());
        }
        return true;
    return true;
}

void
BaseApplyEngine::processNextApplyEvent(){
    auto queue = applyWriteQueue;
        PacketPtr pkt = queue.front();
        uint8_t* data = pkt->getPtr<uint8_t>();

        RequestPtr request = pkt->req;
        int request_offset = requestOffset[request];
        WorkListItem wl = memoryToWorkList(data + request_offset);
        uint32_t prop = wl.prop;
        uint32_t temp_prop = wl.temp_prop;

        if (temp_prop != prop){
            // if (!memPort.blocked() && !reqPort.blocked()){
            //update prop with temp_prop
            if(prop < temp_prop){
                wl.prop = prop;
            }else{
                wl.prop = temp_prop;
            }
            //write back the new worklist item to  memory
            uint8_t* wList = workListToMemory(wl);
            memcpy(data + request_offset, wList, sizeof(WorkListItem));
            //Create memory write requests.
            PacketPtr writePkt  =
            getWritePacket(pkt->getAddr(), 64, data, requestorId);
            if (parent.sendMemReq(writePkt) &&
                parent.recvApplyNotif(WorkListItem.prop,
                                      WorkListItem.degree,
                                      WorkListItem.edgeIndex)){
                queue.pop();
                // memPort.trySendRetry();
                // queue.sendPktRetry = false;
            }
        }else{
            queue.applyQueue.pop();
            if (queue.sendPktRetry && !queue.blocked()){
                // memPort.trySendRetry();
                queue.sendPktRetry = false;
            }
        }
    if(!queue.empty() && !nextApplyEvent.scheduled()){
        schedule(nextApplyEvent, nextCycle());
    }
}

}
