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
#include "debug/MPU.hh"

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
    uint32_t value = *(pkt->getPtr<uint32_t>());

    Addr addr = pkt->getAddr();
    Addr req_addr = (addr / 64) * 64;
    Addr req_offset = addr % 64;

    DPRINTF(MPU, "%s: WorkListEngine is sending a read req to WorkList Item[%lu].\n"
                , __func__, pkt->getAddr() + req_offset);
    PacketPtr memPkt = getReadPacket(req_addr, 64, requestorId);

    requestOffsetMap[memPkt->req] = req_offset;
    requestValueMap[memPkt->req] = value;

    if (!memPortBlocked()) {
        sendMemReq(memPkt);
        updateQueue.pop();
    }
    if (!nextWLReadEvent.scheduled() && !updateQueue.empty()) {
        schedule(nextWLReadEvent, nextCycle());
    }
}

void
BaseWLEngine::processNextWLReduceEvent()
{
    PacketPtr resp = memRespQueue.front();
    uint8_t* respData = resp->getPtr<uint8_t>();
    Addr request_offset = requestOffsetMap[resp->req];
    uint32_t value = requestValueMap[resp->req];

    WorkListItem wl =  memoryToWorkList(respData + request_offset);

    DPRINTF(MPU, "%s: WorkListEngine received WorkList Item[%lu]: %s\n"
                , __func__, resp->getAddr() + request_offset, wl.to_string());
    if (value < wl.temp_prop){
        //update prop with temp_prop
        wl.temp_prop = value;

        uint8_t* wlData = workListToMemory(wl);
        memcpy(respData + request_offset, wlData, sizeof(WorkListItem));

        PacketPtr writePkt  =
        getWritePacket(resp->getAddr(), 64, respData, requestorId);

        DPRINTF(MPU, "%s: Sending a pkt with this info. "
                "pkt->addr: %lu, pkt->size: %lu\npkt->data: %s\n",
                __func__, writePkt->getAddr(),
                writePkt->getSize(), writePkt->printData());
        if (!memPortBlocked()) {
            if (sendWLNotif(resp->getAddr() + request_offset)) {
                sendMemReq(writePkt);
                memRespQueue.pop();
                // TODO: Erase map entries, delete wlData;
            }
        }
    }
    else {
        memRespQueue.pop();
    }
    if (!nextWLReduceEvent.scheduled() && !memRespQueue.empty()){
        DPRINTF(MPU, "%s: memRespQueue.size: %d\n"
                , __func__, memRespQueue.size());
        schedule(nextWLReduceEvent, nextCycle());
    }
}

void
BaseWLEngine::scheduleMainEvent()
{
    if (!memRespQueue.empty() && !nextWLReduceEvent.scheduled()) {
        schedule(nextWLReduceEvent, nextCycle());
    }
}


}
