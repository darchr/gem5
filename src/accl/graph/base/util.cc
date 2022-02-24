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

#include "accl/graph/base/util.hh"

namespace gem5
{

WorkListItem
memoryToWorkList(uint8_t* data){
    WorkListItem wl;

    uint32_t temp_prop = *((uint32_t*) data);
    uint32_t prop = *((uint32_t*) (data + 4));
    uint32_t degree = *((uint32_t*) (data + 8));
    uint32_t addr = *((uint32_t*) (data + 12));

    wl  = {temp_prop, prop, degree, addr};
    return wl;
}

uint8_t*
workListToMemory(WorkListItem wl){
    int  data_size = sizeof(WorkListItem) / sizeof(uint8_t);
    uint8_t* data = new uint8_t [data_size];

    uint32_t* tempPtr = (uint32_t*) data;
    *tempPtr = wl.temp_prop;

    uint32_t* propPtr = (uint32_t*) (data + 4);
    *propPtr = wl.prop;

    uint32_t* degreePtr = (uint32_t*) (data + 8);
    *degreePtr = wl.degree;

    uint32_t* edgePtr = (uint32_t*) (data + 12);
    *edgePtr = wl.edgeIndex;

    return data;
}

// Edge: (weight: 64 bits, neighbor: 64 bits)
Edge
memoryToEdge(uint8_t *data)
{
    uint64_t weight = *((uint64_t*) data);
    Addr neighbor = *((Addr*) (data + 8)); // data + 8 because weight: 8 bytes
    Edge e = {weight, neighbor};
    return e;
}

// Edge: (weight: 64 bits, neighbor: 64 bits)
uint8_t*
edgeToMemory(Edge e)
{
    int data_size = (int) ((sizeof(Edge)) / (sizeof(uint8_t)));

    uint8_t* data = new uint8_t [data_size];

    uint64_t* weightPtr = (uint64_t*) data;
    *weightPtr = e.weight;

    Addr* neighborPtr = (Addr*) (data + 8); // data + 8 because weight: 8 bytes
    *neighborPtr = e.neighbor;

    return data;
}

PacketPtr
getReadPacket(Addr addr, unsigned int size, RequestorID requestorId)
{
    RequestPtr req = std::make_shared<Request>(addr, size, 0, requestorId);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr)requestorId) << 2);

    // Embed it in a packet
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    pkt->allocate();

    return pkt;
}

PacketPtr
getWritePacket(Addr addr, unsigned int size,
            uint8_t* data, RequestorID requestorId)
{
    RequestPtr req = std::make_shared<Request>(addr, size, 0,
                                               requestorId);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr)requestorId) << 2);

    PacketPtr pkt = new Packet(req, MemCmd::WriteReq);
    pkt->allocate();
    pkt->setData(data);

    return pkt;
}

PacketPtr
getUpdatePacket(Addr addr, unsigned int size,
            uint8_t *data, RequestorID requestorId)
{
    RequestPtr req = std::make_shared<Request>(addr, size, 0,
                                               requestorId);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr)requestorId) << 2);

    // FIXME: MemCmd::UpdateWL
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);

    pkt->allocate();
    pkt->setData(data);

    return pkt;
}

}
