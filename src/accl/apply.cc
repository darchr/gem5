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

#include "accl/apply.h"

#include <string>


typedef std::pair<PacketPtr, PortID> ReqPair;
typedef std::pair<uint64_t, PortID> QueuePair;

Apply::Apply(const ApplyParams &params):
    ClockedObject(params),
    nextApplyEvent([this]{processNextApplyEvent; }, name()),
    nextApplyCheckEvent([this]{processNextApplyCheckEvent; }, name()),
    queueSize(params.applyQueueSize) //add this to .py
{
    applyReadQueue(queueSize);
    pplyWriteQueue(queueSize);
}

bool Apply::ApplyRespPort::recvTimingReq(PacketPtr pkt)
{
    if (!owner->handleWL(pkt)){
        return false;
    }
    return true;
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
    memPort = ApplyMemPort
    while(!queue.empty()){
        auto pkt = queue.pop()
        /// conver to ReadReq
        bool ret = memPort->sendPacket(pkt);
        // handel responsehere
        if (!ret)
            break;
    }

}

virtual bool
Apply::MPUMemPort::recvTimingResp(PacketPtr pkt)
{
    return owner->handleMemResp(pkt);
}

bool
Apply::handleMemResp(PacktPtr pkt)
{
    auto queue = applyWriteQueue;
    //check pkt (temp_prop != prop)
    if (temp_prop != prop){
        //update prop with temp_prop
        if (queue->blocked()){
            sendPktRetry = true;
            return false;
        } else
            queue->push(pkt);

        if(!nextApplyEvent.scheduled()){
            schedule(nextApplyEvent, nextCycle());
        }
        return true;
    }
    return true;
}



void
Apply::processNextApplyEvent(){
    auto queue = applyWriteQueue;
    memPort = ApplyMemPort;
    pushPort = ApplyReqPort;
    while(!queue.empty()){
        auto pkt = queue.pop()
        /// conver to ReadReq
        bool ret = memPort->sendPacket(pkt);
        bool push = pushPort->sendPacket(pkt);
        // handel responsehere
        if (!ret || !push)
            break;

    }

}