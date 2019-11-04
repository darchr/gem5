/*
 * Copyright (c) 2019 Jason Lowe-Power
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
 *
 * Authors: Jason Lowe-Power
 */

#include "debug/LearningSimpleCPU.hh"
#include "learning_gem5/part4/cpu.hh"
#include "learning_gem5/part4/memory_request.hh"

void
LearningSimpleCPU::CPUPort::sendPacket(MemoryRequest *request, PacketPtr pkt)
{
    // Note: This flow control is very simple since the CPU is blocking.
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    // On a retry, the request is first removed, then replaced here.
    assert(outstandingRequest == nullptr);
    outstandingRequest = request;

    DPRINTF(LearningSimpleCPU, "Sending packet %s\n", pkt->print());

    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

bool
LearningSimpleCPU::CPUPort::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(LearningSimpleCPU, "Got response for addr %#x\n",
             pkt->getAddr());
    assert(outstandingRequest);

    MemoryRequest *request = outstandingRequest;
    outstandingRequest = nullptr;

    return request->recvResponse(pkt);
}

void
LearningSimpleCPU::CPUPort::recvReqRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);
    assert(outstandingRequest);

    DPRINTF(LearningSimpleCPU, "Got retry request.\n");

    // Grab the blocked packet.
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    MemoryRequest *request = outstandingRequest;
    outstandingRequest = nullptr;

    // Try to resend it. It's possible that it fails again.
    sendPacket(request, pkt);
}
