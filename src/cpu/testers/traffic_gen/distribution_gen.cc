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

#include "cpu/testers/traffic_gen/distribution_gen.hh"

#include "base/random.hh"
#include "base/trace.hh"
#include "debug/DistributionGen.hh"
#include "sim/sim_exit.hh"
#include "sim/system.hh"

namespace gem5
{
DistributionGen::DistributionGen(const Params& params):
    ClockedObject(params), port(".port", this),
    requestorId(params.system->getRequestorId(this)),
    lsb(params.intlv_low_bit), bits(params.intlv_bits),
    match(params.intlv_match), pktsize(params.pkt_size),
    duration(params.duration), distribution(nullptr),
    nextGenEvent([this] { processNextGenEvent(); }, "nextGenEvent")
{}

Port&
DistributionGen::getPort(const std::string& if_name, PortID idx)
{
    if (if_name == "port") {
        return port;
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

void
DistributionGen::recvReqRetry()
{
    assert(!nextGenEvent.scheduled());
    scheduleNextGenEvent();
}

void
DistributionGen::setRayleighDistribution(
    Tick time_unit, double lambda, double sigma)
{
    distribution = new RayleighDistribution(time_unit, lambda, sigma);
}

void
DistributionGen::startup()
{
    panic_if(distribution == nullptr, "Make sure to set distribution.");
    assert(!nextGenEvent.scheduled());
    scheduleNextGenEvent();
}

bool
DistributionGen::GenPort::recvTimingResp(PacketPtr pkt)
{
    assert(pkt->isResponse());
    assert(pkt->isWrite());
    delete pkt;
    return true;
}

void
DistributionGen::GenPort::sendPacket(PacketPtr pkt) {
    panic_if(blocked(), "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

void
DistributionGen::GenPort::recvReqRetry()
{
    panic_if(blockedPacket == nullptr,
            "Received retry without a blockedPacket.");

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);
    if (blockedPacket == nullptr) {
        owner->recvReqRetry();
    }
}

PacketPtr
DistributionGen::createPacket(Addr addr)
{
    RequestPtr req = std::make_shared<Request>(addr, pktsize, 0, requestorId);

    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr) requestorId) << 2);

    PacketPtr pkt = new Packet(req, MemCmd::WriteReq);
    pkt->allocate();

    return pkt;
}

void
DistributionGen::scheduleNextGenEvent()
{
    assert(!nextGenEvent.scheduled());
    Tick interval = distribution->sample();
    DPRINTF(DistributionGen, "%s: %ld < %ld.\n", __func__, curTick() + interval, duration);
    if ((curTick() + interval) < duration) {
        schedule(nextGenEvent, curTick() + interval);
        DPRINTF(DistributionGen, "%s: Scheduled nextGenEvent for %ld.\n", __func__, curTick() + interval);
    } else {
        exitSimLoop("Done sending traffic.", 0, duration, 0, false);
    }
}

void
DistributionGen::processNextGenEvent()
{
    Addr dst_addr = 0;
    Addr match_mask = match;
    if (match == -1) {
        match_mask = random_mt.random(0, (1 << bits) - 1);
    }
    match_mask <<= lsb;
    dst_addr |= match_mask;
    PacketPtr pkt = createPacket(dst_addr);
    DPRINTF(DistributionGen, "%s: Sending pkt: %s.\n", __func__, pkt->print());
    port.sendPacket(pkt);

    if (!port.blocked()) {
        scheduleNextGenEvent();
    }
}

}// namespace gem5
