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

// #include "cpu/testers/traffic_gen/distribution.hh"
#include "cpu/testers/traffic_gen/distribution_gen.hh"

#include "sim/sim_events.hh"
#include "mem/request.hh"
#include "base/random.hh"
#include "base/trace.hh"
#include "sim/sim_events.hh"
#include "mem/request.hh"
// #include "sim/system.hh"

namespace gem5
{
DistributionGen::DistributionGen(const Params &params):
    ClockedObject(params),
    timeLimit(params.max_cycle),
    maxCycles(params.max_cycle),
    injVnet(params.inj_vnet),
    emptyQueues(false),
    finishedTraffic(0),
    lambda(0),
    sigma(0)
{

    for (int i = 0; i < params.port_req_ports_connection_count; ++i) {
        ports.emplace_back(
                    name() + ".ports" + std::to_string(i), this, i);
        distribution = new RayleighDistribution(lambda, sigma);
    }
}

void
DistributionGen::init()
{
    numPacketsSent = 0;
}

void
DistributionGen::startup()
{
    for (PortID i = 0 ; i < ports.size(); i++) {
        generateTraffic(i);
    }
}

Port&
DistributionGen::getPort(const std::string& if_name, PortID idx)
{
    if (if_name == "ports") {
        return ports[idx];
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

void
DistributionGen::completeRequest(PacketPtr pkt)
{
    assert(pkt->isResponse());
    delete pkt;
}

bool
DistributionGen::ReqPort::recvTimingResp(PacketPtr pkt)
{
    owner->completeRequest(pkt);
    return 0;
}

void
DistributionGen::ReqPort::recvReqRetry()
{
    panic_if(blockedPacket == nullptr,
            "Received retry without a blockedPacket.");

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);
    if (blockedPacket == nullptr) {
        owner->recvReqRetry(id());
    }
}

void 
DistributionGen::recvReqRetry(PortID id)
{
    generateTraffic(id);
}

void
DistributionGen::ReqPort::sendPacket(PacketPtr pkt) {
    panic_if(blocked(), "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
    owner->numPacketsSent++;
}


void
DistributionGen::generateTraffic(PortID id)
{
    /* generate packet */
    // Addr dst_addr = ;
    // create packet
    // send
   Addr paddr =  id;
    paddr <<= 6;
    unsigned access_size = 1; // Does not affect Ruby simulation

    MemCmd::Command requestType;

    RequestPtr req = nullptr;
    Request::Flags flags;

    // Inject in specific Vnet
    // Vnet 0 and 1 are for control packets (1-flit)
    // Vnet 2 is for data packets (5-flit)
    int injReqType = injVnet;

    if (injReqType < 0 || injReqType > 2)
    {
        // randomly inject in any vnet
        injReqType = random_mt.random(0, 2);
    }

    if (injReqType == 0) {
        // generate packet for virtual network 0
        requestType = MemCmd::ReadReq;
        req = std::make_shared<Request>(paddr, access_size, flags, id);
    } else if (injReqType == 1) {
        // generate packet for virtual network 1
        requestType = MemCmd::ReadReq;
        flags.set(Request::INST_FETCH);
        req = std::make_shared<Request>(
            0x0, access_size, flags, id, 0x0, 0);
        req->setPaddr(paddr);
    } else {
        requestType = MemCmd::WriteReq;
        req = std::make_shared<Request>(paddr, access_size, flags, id);
    }

    req->setContext(id);

    PacketPtr pkt = new Packet(req, requestType);
    pkt->dataDynamic(new uint8_t[req->getSize()]);
    pkt->senderState = NULL;
    ports[id].sendPacket(pkt);

    if (!ports[id].blocked()) {
        double interval = distribution[id].generateInterEventTime();
        if (curTick() + interval < timeLimit) {
            GeneratorEvent* event = new GeneratorEvent(this, id);
            schedule(event, curTick() + interval);
        } else {
            finishedTraffic++;
            if (finishedTraffic == ports.size()) {
                exitSimLoopNow("Simulation Finished.");
            }
        }
    }

}

}// namespace gem5