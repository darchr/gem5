// Copyright (c) 2021-2024 The Regents of the University of California
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

#include <zlib.h>
#include <cassert>
#include <iomanip>
#include <sstream>

#include "base/trace.hh"
#include "debug/CheckpointFlag.hh"
#include "sim/stats.hh"

namespace gem5
{

OutgoingRequestBridge::OutgoingRequestBridge(
    const OutgoingRequestBridgeParams &params) :
    AbstractMemory(params),
    stats(this),
    outgoingPort(std::string(name()), this),
    sstResponder(nullptr),
    physicalAddressRanges(params.physical_address_ranges.begin(),
                          params.physical_address_ranges.end()),
    // nodeIndex(params.node_index),
    // blockSize(params.block_size),
    // startRange(params.start_range),
    // endRange(params.end_range),
    useSSTSim(params.use_sst_sim)
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
    if (return_status) {
        ++stats.numIncomingPackets;
        unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
        stats.sizeIncomingPackets += pkt_size;
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
OutgoingRequestBridge::clearInitData() {
    // free the memory
    initData.clear();
    assert(initData.size() == 0);
}

void
OutgoingRequestBridge::handleRecvFunctional(PacketPtr pkt)
{
    // Check at which stage are we at. If we are at INIT phase, then queue all
    // these packets.
    if(useSSTSim == true) { 
        if (!getInitPhaseStatus())
        {
            uint8_t* ptr = pkt->getPtr<uint8_t>();
            uint64_t size = pkt->getSize();
            std::vector<uint8_t> data(ptr, ptr+size);
            initData.push_back(std::make_pair(pkt->getAddr(), data));
            initPhaseComplete(true);
        }
        // This is the RUN phase. SST does not allow any sendUntimedData (AKA
        // functional accesses) to it's memory. We need to convert these
        // accesses to timing to at least store the correct data in the memory.
        else {
            // These packets have to translated at runtime. We convert these
            // packets to timing as its data has to be stored correctly in SST
            // memory. Otherwise reads from the SST memory will fail. To
            // reproduce this error, don not handle any functional accesses and
            // the kernel boot will fail while reading the correct partition
            // from the vda device.
            //
            // These requests will be sent to SST to keep the SST's memory
            // updated, however, these are being handled in gem5.
            // FIXME:
            sstResponder->handleRecvFunctional(pkt);
        }
    }
    // It does not matter if SST is used or not, all functional accesses (only
    // seen in ARM and RISCV should have a gem5 functionalAccess(pkt).
    functionalAccess(pkt);
}

Tick
OutgoingRequestBridge::
OutgoingRequestPort::recvAtomic(PacketPtr pkt)
{
    // We need to assert(!useSSTSim) but this will add an assert per memory
    // request. So we reply on the user to set the configs correctly.
    owner->access(pkt);
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
    if (return_status) {
        ++stats.numOutgoingPackets;
        unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
        stats.sizeOutgoingPackets += pkt_size;
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
