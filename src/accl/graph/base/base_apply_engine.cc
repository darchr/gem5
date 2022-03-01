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

#include "accl/graph/base/base_apply_engine.hh"

#include <string>

#include "accl/graph/base/util.hh"
#include "debug/MPU.hh"


namespace gem5
{

BaseApplyEngine::BaseApplyEngine(const BaseApplyEngineParams &params):
    BaseEngine(params),
    nextApplyCheckEvent([this]{ processNextApplyCheckEvent(); }, name()),
    nextApplyEvent([this]{ processNextApplyEvent(); }, name())
{}

bool
BaseApplyEngine::recvWLNotif(Addr addr)
{
    // TODO: Investigate the situation where the queue is full.
    applyReadQueue.push(addr);
    if (!nextApplyCheckEvent.scheduled()){
        schedule(nextApplyCheckEvent, nextCycle());
    }
    return true;
}

void
BaseApplyEngine::processNextApplyCheckEvent()
{
    // TODO: We might want to change the way this function
    // pops items off queue, maybe we should pop every n cycles
    // or change the clock domain for this simobject.
    Addr addr = applyReadQueue.front();
    Addr req_addr = (addr / 64) * 64;
    Addr req_offset = (addr % 64);
    if (acquireAddress(req_addr)) {
        PacketPtr memPkt = getReadPacket(req_addr, 64, requestorId);
        requestOffset[memPkt->req] = req_offset;
        if (!memPortBlocked()) {
            sendMemReq(memPkt);
            applyReadQueue.pop();
        }
    }
    if (!applyReadQueue.empty() &&  !nextApplyCheckEvent.scheduled()){
        schedule(nextApplyCheckEvent, nextCycle());
    }
}

void
BaseApplyEngine::processNextApplyEvent()
{
    PacketPtr pkt = memRespQueue.front();
    uint8_t* data = pkt->getPtr<uint8_t>();

    RequestPtr request = pkt->req;
    Addr request_offset = requestOffset[request];

    WorkListItem wl = memoryToWorkList(data + request_offset);
    DPRINTF(MPU, "%s: Apply Engine is reading WorkList Item[%lu]: %s\n"
                , __func__, pkt->getAddr() + request_offset, wl.to_string());
    // FIXME: Not so much of a fixme. However, why do we fwd a worklistitem
    // to applyengine if temp_prop < prop. If temp_prop has not changed, why
    // fwd it to applyengine?
    if (wl.temp_prop < wl.prop) {
        // TODO: instead of min add a Reduce function.
        //update prop with temp_prop
        wl.prop = wl.temp_prop;
        //write back the new worklist item to  memory
        uint8_t* wList = workListToMemory(wl);
        memcpy(data + request_offset, wList, sizeof(WorkListItem));
        //Create memory write requests.
        PacketPtr writePkt  =
        getWritePacket(pkt->getAddr(), 64, data, requestorId);

        DPRINTF(MPU, "%s: Sending a pkt with this info. "
                "pkt->addr: %lu, pkt->size: %lu\npkt->data: %s\n",
                __func__, writePkt->getAddr(),
                writePkt->getSize(), writePkt->printData());

        if (!memPortBlocked()) {
            if (sendApplyNotif(wl.prop, wl.degree, wl.edgeIndex)) {
                sendMemReq(writePkt);
                memRespQueue.pop();
                DPRINTF(MPU, "%s: The Apply Engine is applying the new value into WorkList Item[%lu]: %s\n"
                              , __func__, pkt->getAddr() + request_offset, wl.to_string());
            }
        }
    } else {
        memRespQueue.pop();
    }
    if (!releaseAddress(pkt->getAddr())) {
        panic("Could not release an address");
    }
    if (!nextApplyEvent.scheduled() && !memRespQueue.empty()){
        schedule(nextApplyEvent, nextCycle());
    }
}

void
BaseApplyEngine::scheduleMainEvent()
{
    if (!memRespQueue.empty() && !nextApplyEvent.scheduled()) {
        schedule(nextApplyEvent, nextCycle());
    }
}

}
