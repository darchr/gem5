// Copyright (c) 2021-2023 The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met: redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer;
// redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution;
// neither the name of the copyright holders nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "sst/outgoing_request_bridge.hh"

#include <cassert>
#include <iomanip>
#include <sstream>

#include "sim/stats.hh"
#include "base/trace.hh"

namespace gem5
{

OutgoingRequestBridge::OutgoingRequestBridge(
    const OutgoingRequestBridgeParams &params) :
    SimObject(params),
    stats(this),
    outgoingPort(std::string(name()), this),
    sstResponder(nullptr),
    physicalAddressRanges(params.physical_address_ranges.begin(),
                          params.physical_address_ranges.end())
{
    this->init_phase_bool = false;
}

OutgoingRequestBridge::~OutgoingRequestBridge()
{
}

OutgoingRequestBridge::
OutgoingRequestPort::OutgoingRequestPort(const std::string &name_,
                                         OutgoingRequestBridge* owner_) :
    ResponsePort(name_)
{
    owner = owner_;
}

OutgoingRequestBridge::
OutgoingRequestPort::~OutgoingRequestPort()
{
}


void
OutgoingRequestBridge::init()
{
    if (outgoingPort.isConnected())
        outgoingPort.sendRangeChange();
}

Port &
OutgoingRequestBridge::getPort(const std::string &if_name, PortID idx)
{
    return outgoingPort;
}

AddrRangeList
OutgoingRequestBridge::getAddrRanges() const
{
    return outgoingPort.getAddrRanges();
}

std::vector<std::pair<Addr, std::vector<uint8_t>>>
OutgoingRequestBridge::getInitData() const
{
    return initData;
}

void
OutgoingRequestBridge::setResponder(SSTResponderInterface* responder)
{
    sstResponder = responder;
}

bool
OutgoingRequestBridge::sendTimingResp(gem5::PacketPtr pkt)
{
    // see if the responder responded true or false. if it's true, then we
    // increment the stats counters.
    bool return_status = outgoingPort.sendTimingResp(pkt);
    if (return_status == true) {
        ++stats.numIncomingPackets;
        stats.sizeIncomingPackets += pkt->getSize();
    }
    return return_status;
}

void
OutgoingRequestBridge::sendTimingSnoopReq(gem5::PacketPtr pkt)
{
    outgoingPort.sendTimingSnoopReq(pkt);
}

void
OutgoingRequestBridge::initPhaseComplete(bool value) {
    init_phase_bool = value;
}
bool
OutgoingRequestBridge::getInitPhaseStatus() {
    return init_phase_bool;
}
void
OutgoingRequestBridge::handleRecvFunctional(PacketPtr pkt)
{
    // This should not receive any functional accesses
    // gem5::MemCmd::Command pktCmd = (gem5::MemCmd::Command)pkt->cmd.toInt();
    // std::cout << "Recv Functional : 0x" << std::hex << pkt->getAddr() <<
    // std::dec << " " << pktCmd << " " << gem5::MemCmd::WriteReq << " " <<
    // getInitPhaseStatus() << std::endl;
    // Check at which stage are we at. If we are at INIT phase, then queue all
    // these packets.
    if (!getInitPhaseStatus())
    {
        // sstResponder->recvAtomic(pkt);
        uint8_t* ptr = pkt->getPtr<uint8_t>();
        uint64_t size = pkt->getSize();
        std::vector<uint8_t> data(ptr, ptr+size);
        initData.push_back(std::make_pair(pkt->getAddr(), data));
    }
    // This is the RUN phase. SST does not allow any sendUntimedData (AKA
    // functional accesses) to it's memory. We need to convert these accesses
    // to timing to at least store the correct data in the memory.
    else {
        // These packets have to translated at runtime. We convert these
        // packets to timing as its data has to be stored correctly in SST
        // memory. Otherwise reads from the SST memory will fail. To reproduce
        // this error, don not handle any functional accesses and the kernel
        // boot will fail while reading the correct partition from the vda
        // device.
        sstResponder->handleRecvFunctional(pkt);
    }
}

Tick
OutgoingRequestBridge::
OutgoingRequestPort::recvAtomic(PacketPtr pkt)
{
    // return 0;
    assert(false && "OutgoingRequestPort::recvAtomic not implemented");
    return Tick();
}

void
OutgoingRequestBridge::
OutgoingRequestPort::recvFunctional(PacketPtr pkt)
{
    owner->handleRecvFunctional(pkt);
}

bool
OutgoingRequestBridge::
OutgoingRequestPort::recvTimingReq(PacketPtr pkt)
{
    return owner->handleTiming(pkt);
}

bool OutgoingRequestBridge::handleTiming(PacketPtr pkt)
{
    // see if the responder responded true or false. if it's true, then we
    // increment the stats counters.
    bool return_status = sstResponder->handleRecvTimingReq(pkt);
    if(ret == true) {
        ++stats.numOutgoingPackets;
        stats.sizeOutgoingPackets += pkt->getSize();
    }
    return return_status;
}

void
OutgoingRequestBridge::
OutgoingRequestPort::recvRespRetry()
{
    owner->sstResponder->handleRecvRespRetry();
}

AddrRangeList
OutgoingRequestBridge::
OutgoingRequestPort::getAddrRanges() const
{
    return owner->physicalAddressRanges;
}

OutgoingRequestBridge::StatGroup::StatGroup(statistics::Group *parent)
    : statistics::Group(parent),
    ADD_STAT(numOutgoingPackets, statistics::units::Count::get(),
            "Number of packets going out of the gem5 port"),
    ADD_STAT(sizeOutgoingPackets, statistics::units::Byte::get(),
            "Cumulative size of all the outgoing packets"),
    ADD_STAT(numIncomingPackets, statistics::units::Count::get(),
            "Number of packets coming into the gem5 port"),
    ADD_STAT(sizeIncomingPackets, statistics::units::Byte::get(),
            "Cumulative size of all the incoming packets")
{
}
}; // namespace gem5
