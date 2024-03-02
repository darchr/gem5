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
    nodeIndex(params.node_index),
    blockSize(params.block_size),
    startRange(params.start_range),
    endRange(params.end_range)
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

    functionalAccess(pkt);
}

Tick
OutgoingRequestBridge::
OutgoingRequestPort::recvAtomic(PacketPtr pkt)
{
    // return 0;
    // assert(false && "OutgoingRequestPort::recvAtomic not implemented");

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
        stats.sizeOutgoingPackets += pkt->getSize();
        // access(pkt);
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

void
OutgoingRequestBridge::serialize(CheckpointOut &cp) const
{
    ScopedCheckpointSection sec(cp, "remoteMemoryStore_" + nodeIndex);

    paramOut(cp, "numMemoryBlocks", (endRange - startRange) / blockSize);

    for (Addr addr = startRange; addr < endRange; addr += blockSize) {
        uint8_t  *host_addr = toHostAddr(addr);
        assert(host_addr);
        paramOut(cp, "data", *host_addr);
        DPRINTF(CheckpointFlag, "Serialized addr: %llu with data: %llu\n",
                &host_addr, *host_addr);
    }
}

void
OutgoingRequestBridge::unserialize(CheckpointIn &cp)
{
    ScopedCheckpointSection sec(cp, "remoteMemoryStore_" + nodeIndex);

    // uint64_t num_blocks;
    // Addr addr = startRange;
    // uint64_t *data;

    // paramIn(cp, "numMemoryBlocks", num_blocks);

    // for (int i = 0; i < num_blocks; i++) {
    //     paramIn(cp, "data", *data);
    //     uint8_t *host_addr = toHostAddr(addr);
    //     std::memcpy(host_addr, &data, blockSize);
    //     addr += blockSize;
    //     DPRINTF(
    //         CheckpointFlag, "Unserialized data: %llu into host addr:%llu\n",
    //         *data, &host_addr);
    // }

    // const uint32_t chunk_size = 16384;


    // unsigned int store_id;
    // UNSERIALIZE_SCALAR(store_id);

    std::string filename = "/home/kaustavg/simulators/X86/darchr/gem5/ext/sst/ckpt-dir/board.physmem.store8.pmem";
    // UNSERIALIZE_SCALAR(filename);
    // std::string filepath = cp.getCptDir() + "/" + filename;

    // mmap memoryfile
    gzFile compressed_mem = gzopen(filename.c_str(), "rb");
    if (compressed_mem == NULL)
        fatal("Can't open physical memory checkpoint file '%s'", filename);

    // we've already got the actual backing store mapped
    // uint8_t* pmem = backingStore[store_id].pmem;
    // AddrRange range = backingStore[store_id].range;
    AddrRange range = getAddrRanges();
    uint64_t start_addr = range.start();
    // uint8_t* pmem = (uint8_t*) mmap(NULL, range.size(),
    //                                 PROT_READ | PROT_WRITE,
    //                                 MAP_ANON | MAP_PRIVATE, -1, 0);


//     AddrRangeList
    uint64_t curr_size = 0;
    // long* temp_page = new long[chunk_size];
    // long* pmem_current;

    const int amount_in_bytes = 64;

    char buf[amount_in_bytes];

    // char* offset = buf;

    uint32_t bytes_read;
    while (curr_size < range.size()) {

        // uint8_t *gem5_data;

        bytes_read = gzread(compressed_mem, buf, amount_in_bytes);
        // buf[amount_in_bytes] = '\0';

        if (bytes_read == 0)
            break;

        // assert(bytes_read % sizeof(long) == 0);


        // for (uint32_t x = 0; x < bytes_read ; x++) {
        //     if (buf != 0) {
        //         std::cout << buf << std::endl;
        //     }
        // }
    // //         // Only copy bytes that are non-zero, so we don't give
    // //         // the VM system hell
    //         if (*(temp_page + x) != 0) {
    //             pmem_current = (long*)(pmem + curr_size + x * sizeof(long));

    // //             std::vector<uint8_t> data(ptr, ptr + curr_size);
    // //             initData.push_back(std::make_pair(pkt->getAddr(), data));
    //             std::cout << "/" << pmem_current << std::endl;
    //             *pmem_current = *(temp_page + x);
    //         }
    //     }
        curr_size += bytes_read;

        gem5::Addr taddr = start_addr + curr_size;
        // std::string teststr(buf);
        std::vector<uint8_t> data;
        for (int i = 0 ; i < amount_in_bytes ; i++)
            data.push_back((uint8_t)buf[i]);
        // for (int i = 0 ; i < amount_in_bytes ; i++) {
        //     // reading one byte at a time.
        //     data.push_back((uint8_t)buf[i]);
        // }
        // (*buf, amount_in_bytes);
        initData.push_back(std::make_pair(taddr, data));
        // std::cout << "|" << start_addr << " " << taddr << " " << start_addr + curr_size  << " " << bytes_read << "|" << teststr << "|" << std::endl; //*temp_page << " " << temp_page << " " << &temp_page << "|" << std::endl;
    }

    // delete[] temp_page;

    if (gzclose(compressed_mem))
        fatal("Close failed on physical memory checkpoint file '%s'\n",
              filename);
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
