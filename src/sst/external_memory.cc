// Copyright (c) 2023-2024 The Regents of the University of California
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


#include "sst/external_memory.hh"

#include <zlib.h>
#include <cassert>
#include <iomanip>
#include <sstream>

#include "base/trace.hh"
#include "debug/CheckpointFlag.hh"
#include "sim/stats.hh"

namespace gem5
{

ExternalMemory::ExternalMemory(
    const ExternalMemoryParams &params) :
    AbstractMemory(params),
    stats(this),
    outgoingPort(std::string(name()), this),
    sstResponder(nullptr),
    physicalAddressRanges(params.physical_address_ranges.begin(),
                          params.physical_address_ranges.end()),
    nodeIndex(params.node_index),
    useSSTSim(params.use_sst_sim)
{
    this->init_phase_bool = false;
    // This needs to be in the class constructor
}

ExternalMemory::~ExternalMemory()
{
}

ExternalMemory::
ExternalMemoryPort::ExternalMemoryPort(const std::string &name_,
                                         ExternalMemory* owner_) :
    ResponsePort(name_)
{
    owner = owner_;
}

ExternalMemory::
ExternalMemoryPort::~ExternalMemoryPort()
{
}


void
ExternalMemory::init()
{
    if (outgoingPort.isConnected())
        outgoingPort.sendRangeChange();
}

Port &
ExternalMemory::getPort(const std::string &if_name, PortID idx)
{
    return outgoingPort;
}

AddrRangeList
ExternalMemory::getAddrRanges() const
{
    return outgoingPort.getAddrRanges();
}

std::vector<std::pair<Addr, std::vector<uint8_t>>>
ExternalMemory::getInitData() const
{
    return initData;
}

void
ExternalMemory::setResponder(SSTResponderInterface* responder)
{
    sstResponder = responder;
}

bool
ExternalMemory::sendTimingResp(gem5::PacketPtr pkt)
{
    // A timing response will only be received if there was a timing request
    // sent at the first place. So we do not need an aseert() here.
    //
    // We also do not need to assert whether this response is a response.
    assert(pkt->isResponse());
    // see if the responder responded true or false. if it's true, then we
    // increment the stats counters.
    bool return_status = outgoingPort.sendTimingResp(pkt);
    if (return_status) {
        // This packet got a response! Add the latency to the stats.
        stats.packetLatency.sample(
                gem5::curTick() - outstanding_requests[pkt]);

        // delete this entry to save some memory.
        outstanding_requests.erase(pkt);
       
        // Count this packet as an incoming packet.
        ++stats.numIncomingPackets;

        if (pkt->isRead()) {
            // These should always be read responses!
            ++stats.numReadIncomingPackets;
            // This packet will have exactly 64 bytes of data. This has been
            // validated.
            stats.sizeIncomingPackets += pkt->getSize();
        }
        else {
            ++stats.numWriteIncomingPackets;
            assert(false && "Should only see read responses!");
        }
    }
    return return_status;
}

void
ExternalMemory::sendTimingSnoopReq(gem5::PacketPtr pkt)
{
    outgoingPort.sendTimingSnoopReq(pkt);
}

void
ExternalMemory::initPhaseComplete(bool value) {
    init_phase_bool = value;
}
bool
ExternalMemory::getInitPhaseStatus() {
    return init_phase_bool;
 }

void
ExternalMemory::clearInitData() {
    // free the memory
    initData.clear();
    assert(initData.size() == 0);
}

void
ExternalMemory::handleRecvFunctional(PacketPtr pkt)
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
ExternalMemory::
ExternalMemoryPort::recvAtomic(PacketPtr pkt)
{
    // We need to assert(!useSSTSim) but this will add an assert per memory
    // request. So we reply on the user to set the configs correctly.
    owner->access(pkt);
    return Tick();
}

void
ExternalMemory::
ExternalMemoryPort::recvFunctional(PacketPtr pkt)
{
    owner->handleRecvFunctional(pkt);
}

bool
ExternalMemory::
ExternalMemoryPort::recvTimingReq(PacketPtr pkt)
{
    return owner->handleTiming(pkt);
}

bool ExternalMemory::handleTiming(PacketPtr pkt)
{
    // Implementation and validation notes; I have validated that all requests
    // coming here has a fixed size of 64 bytes. I am removing the assert to
    // make the simulation faster.
    //
    // Make sure that this memory is being simulated in SST
    assert (useSSTSim);

    // This might be an unnecessary statistic. This was used to veryfy reads
    // and writes in the beginning.
    ++stats.numOutgoingPackets;
    if (pkt->isRead()) {
        // Add this packet to a read type outgoing request!
        ++stats.numReadOutgoingPackets;
        // A read packet cannot have valid data. An assert was removed as it
        // was verified.
    }
    else if (pkt->isWrite()) {
        // Add this packet to a write type outgoing request!
        ++stats.numWriteOutgoingPackets;
        // only write packets should have outgoing data. The assert was removed
        // as it was verified.
        stats.sizeOutgoingPackets += pkt->getSize();
    }
    else {
        // The simulation should fail if the request is not a read or a write
        // request! The external memory can only handle reads and writes.
        assert(false && "The external memory cannot handle this request!");
    }

    // Keep the time when this packet was sent out to SST.
    outstanding_requests[pkt] = gem5::curTick();

    // Take samples of the size of this map
    stats.outstandingPackets.sample(outstanding_requests.size());

    // The responder will always return true as SST can *just* accept the
    // request.
    sstResponder->handleRecvTimingReq(pkt);

    // This always returns true.
    return true;
}

void
ExternalMemory::
ExternalMemoryPort::recvRespRetry()
{
    owner->sstResponder->handleRecvRespRetry();
}

AddrRangeList
ExternalMemory::
ExternalMemoryPort::getAddrRanges() const
{
    return owner->physicalAddressRanges;
}

ExternalMemory::StatGroup::StatGroup(statistics::Group *parent)
    : statistics::Group(parent),
    ADD_STAT(numOutgoingPackets, statistics::units::Count::get(),
            "Number of packets going out of the gem5 port"),
    ADD_STAT(numReadOutgoingPackets, statistics::units::Count::get(),
            "Count of all the read outgoing packets"),
    ADD_STAT(numWriteOutgoingPackets, statistics::units::Count::get(),
            "Count of all the wirte outgoing packets"),
    ADD_STAT(sizeOutgoingPackets, statistics::units::Byte::get(),
            "Cumulative size of all the outgoing packets"),
    ADD_STAT(numIncomingPackets, statistics::units::Count::get(),
            "Number of packets coming into the gem5 port"),
    ADD_STAT(sizeIncomingPackets, statistics::units::Byte::get(),
            "Cumulative size of all the incoming packets"),
    ADD_STAT(numReadIncomingPackets, statistics::units::Count::get(),
            "Count of all the read incoming packets"),
    ADD_STAT(numWriteIncomingPackets, statistics::units::Count::get(),
            "Count of all the write incoming packets"),
    ADD_STAT(packetLatency, statistics::units::Count::get(),
            "Histogram of packet latency sent via this port."),
    ADD_STAT(outstandingPackets, statistics::units::Count::get(),
            "Histogram of outstanding packets.")
{
    using namespace statistics;
    // Initialize any histogram stats here
    packetLatency
        .init(2)
        .flags(pdf);
    outstandingPackets
        .init(2)
        .flags(pdf);
}
}; // namespace gem5
