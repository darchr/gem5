/*
 * Copyright (c) 2017 Jason Lowe-Power
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

#include "cpu/learning_simple_cpu/cpu.hh"

#include "debug/LearningSimpleCPU.hh"

LearningSimpleCPU::LearningSimpleCPU(LearningSimpleCPUParams *params) :
    BaseCPU(params),
    port(name()+".port", this),
    memOutstanding(false)
{
    fatal_if(FullSystem, "The LearningSimpleCPU doesn't support full system.");

    thread = new SimpleThread(this, 0, params->system, params->workload[0],
                              params->itb, params->dtb, params->isa[0]);

    // Register this thread with the BaseCPU.
    threadContexts.push_back(thread->getTC());
}

void
LearningSimpleCPU::init()
{
    DPRINTF(LearningSimpleCPU, "LearningSimpleCPU init\n");

    BaseCPU::init();

    thread->getTC()->initMemProxies(thread->getTC());
}

void
LearningSimpleCPU::startup()
{
    DPRINTF(LearningSimpleCPU, "LearningSimpleCPU startup\n");

    BaseCPU::startup();

    thread->startup();
}

void
LearningSimpleCPU::wakeup(ThreadID tid)
{
    assert(tid == 0); // This CPU doesn't support more than one thread!

    // Not sure what to do here.

    //  if (threadInfo[tid]->thread->status() == ThreadContext::Suspended) {
    //      DPRINTF(SimpleCPU,"[tid:%d] Suspended Processor awoke\n", tid);
    //      threadInfo[tid]->thread->activate();
    //  }
}

void
LearningSimpleCPU::CPUPort::sendPacket(PacketPtr pkt)
{
    // Note: This flow control is very simple since the memobj is blocking.

    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

bool
LearningSimpleCPU::CPUPort::recvTimingResp(PacketPtr pkt)
{
    // Just forward to the memobj.
    return owner->handleResponse(pkt);
}

void
LearningSimpleCPU::CPUPort::recvReqRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);

    // Grab the blocked packet.
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    // Try to resend it. It's possible that it fails again.
    sendPacket(pkt);
}

bool
LearningSimpleCPU::handleResponse(PacketPtr pkt)
{
    assert(memOutstanding);
    DPRINTF(LearningSimpleCPU, "Got response for addr %#x\n", pkt->getAddr());

    // The packet is now done. We're about to put it in the port, no need for
    // this object to continue to stall.
    // We need to free the resource before sending the packet in case the CPU
    // tries to send another request immediately (e.g., in the same callchain).
    memOutstanding = false;

    // Do something?

    return true;
}

LearningSimpleCPU*
LearningSimpleCPUParams::create()
{
    return new LearningSimpleCPU(this);
}
