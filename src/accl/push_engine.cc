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

#include "accl/push_engine.hh"

#include "debug/PushEngine.hh"

PushEngine::PushEngine(const PushEngineParams& params):
    ClockedObject(params),
    system(params.system),
    requestorId(system->getRequestorId(this)),
    reqPort(name() + ".reqPort", this),
    respPort(name() + ".respPort", this),
    memPort(name() + ".memPort", this),
    vertexQueueSize(params.vertex_queue_size),
    vertexQueueLen(0),
    updateQueue(params.update_queue_size),
    updateQueueLen(0),
    nextReceiveEvent([this]{ processNextReceiveEvent(); }, name()),
    nextReadEvent([this]{ processNextReadEvent(); }, name()),
    nextCreateEvent([this]{ processNextCreateEvent(); }, name()),
    nextSendEvent([this]{ processNextSendEvent(); }, name())
{}

Port &
PushEngine::getPort(const std::string &if_name, PortID idx)
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

bool
PushEngine::PushRespPort::recvTimingReq(PacketPtr pkt)
{
    return owner->handleUpdate(pkt);
}

bool
PushEngine::handleUpdate(PacketPtr pkt)
{
    if (vertexQueueLen < vertexQueueSize) {
        vertexQueue.push(pkt)
        vertexQueueLen++;
        return true;

        if (!nextReceiveEvent.scheduled()){
            schedule(nextReceiveEvent, nextCycle());
        }
    }
    return false;
}

void
PushEngine::processNextReceiveEvent()
{
    PacketPtr updatePkt = vertexQueue.pop();
    uint8_t* data = updatePkt->getData<uint8_t>();

    Addr edgeListAddr = ; // TODO: Generalize finding this address.
    int outDegree = ; // TODO: Generalize finding this value.

    Addr reqAddr = (edgeListAddr / 64) * 64;
    Addr offsetAddr = edgeListAddr % 64;

    PacketPtr pkt = getReadPacket(reqAddr, 64, requestorId);

    memPort.sendPacket(pkt);


}

void
PushEngine::processNextReadEvent()
{

}

void
PushEngine::processNextCreateEvent()
{

}

void
PushEngine::processNextSendEvent()
{

}