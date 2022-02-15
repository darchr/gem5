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

namespace gem5
{

Apply::Apply(const ApplyParams &params):
    ClockedObject(params),
    system(params.system),
    requestorId(system->getRequestorId(this)),
    reqPort(name() + ".reqPort", this),
    respPort(name() + ".respPort", this),
    memPort(name() + ".memPort", this),
    nextApplyEvent([this]{ processNextApplyEvent(); }, name()),
    nextApplyCheckEvent([this]{ processNextApplyCheckEvent(); }, name())
{
    applyReadQueue(params.applyQueueSize);
    applyWriteQueue(params.applyQueueSize);
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
Apply::ApplyRespPort::getAddrRanges()
{
    return owner->getAddrRanges();
}

bool Apply::ApplyRespPort::recvTimingReq(PacketPtr pkt)
{
    if (!owner->handleWL(pkt)){
        return false;
    }
    return true;
}

void
Apply::ApplyRespPort::trySendRetry()
{
    sendRetryReq();
}

void
Apply::ApplyRespPort::recvFunctional(PacketPtr pkt)
{
    panic("Not implemented");
}

Tick
Apply::ApplyRespPort::recvAtomic(PacketPtr pkt)
{
    panic("recvAtomic unimpl.");
}

void
Apply::ApplyRespPort::recvRespRetry()
{
    panic("recvRespRetry from response port is called.");
}

bool
Apply::ApplyMemPort::recvTimingResp(PacketPtr pkt)
{
    return owner->handleMemResp(pkt);
}

void
Apply::ApplyMemPort::sendPacket(PacketPtr pkt)
{
    panic_if(_blocked, "Should never try to send if blocked MemSide!");

    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
        _blocked = true;
    }
}

void
Apply::ApplyMemPort::trySendRetry()
{
    sendRetryResp();
}

void
Apply::ApplyMemPort::recvReqRetry()
{
    _blocked = false;
    sendPacket(blockedPacket);
    blockedPacket = nullptr;
}

void
Apply::ApplyReqPort::sendPacket(PacketPtr pkt)
{
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
        _blocked = true;
    }
}

void
Apply::ApplyReqPort::recvReqRetry()
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

void Apply::processNextApplyCheckEvent(){
    auto queue = applyReadQueue;
    if (!memPort.blocked()){
        auto pkt = queue.pop();
        if (queue.sendPktRetry && !queue.blocked()){
                respPort.trySendRetry();
                queue.sendPktRetry = false;
        }
        // conver to ReadReq
        Addr req_addr = (pkt->getAddr() / 64) * 64;
        int req_offset = (pkt->getAddr()) % 64;
        RequestPtr request = std::make_shared<Request>(req_addr, 64, 0 ,0);
        PacketPtr memPkt = new Packet(request, MemCmd::ReadReq);
        requestOffset[request] = req_offset;
        memPort.sendPacket(memPkt);
    }
    if (!queue.empty() &&  !nextApplyCheckEvent.scheduled()){
        schedule(nextApplyCheckEvent, nextCycle());
    }
}

bool
Apply::handleMemResp(PacketPtr pkt)
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
Apply::processNextApplyEvent(){
    auto queue = applyWriteQueue;
        PacketPtr pkt = queue.front();
        uint8_t* data = pkt->getPtr<uint8_t>();

        RequestPtr request = pkt->req;
        int request_offset = requestOffset[request];
        WorkListItem wl = memoryToWorkList(data + request_offset);
        uint32_t prop = wl.prop;
        uint32_t temp_prop = wl.temp_prop;

        if (temp_prop != prop){
            if (!memPort.blocked() && !reqPort.blocked()){
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
                memPort.sendPacket(writePkt);
                reqPort.sendPacket(writePkt);
                queue.pop();
                if (queue.sendPktRetry && !queue.blocked()){
                    memPort.trySendRetry();
                    queue.sendPktRetry = false;
                }
            }
        }else{
            queue.pop();
            if (queue.sendPktRetry && !queue.blocked()){
                memPort.trySendRetry();
                queue.sendPktRetry = false;
            }
        }
    if(!queue.empty() && !nextApplyEvent.scheduled()){
        schedule(nextApplyEvent, nextCycle());
    }
}

}
