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

#ifndef __ACCL_GRAPH_BASE_GRAPH_WORKLOAD_HH__
#define  __ACCL_GRAPH_BASE_GRAPH_WORKLOAD_HH__

#include <bitset>
#include <deque>
#include <tuple>

#include "accl/graph/base/data_structs.hh"
#include "mem/packet.hh"


namespace gem5
{

class GraphWorkload
{
  public:
    GraphWorkload() {}
    ~GraphWorkload() {}

    virtual void init(PacketPtr pkt, int bit_index_base,
                    std::bitset<MAX_BITVECTOR_SIZE>& needsPush,
                    std::deque<int>& activeBits) = 0;
    virtual uint32_t reduce(uint32_t update, uint32_t value) = 0;
    virtual uint32_t propagate(uint32_t value, uint32_t weight) = 0;
    virtual bool applyCondition(WorkListItem wl) = 0;
    virtual bool preWBApply(WorkListItem& wl) = 0;
    virtual std::tuple<uint32_t, bool, bool> prePushApply(WorkListItem& wl) = 0;
    virtual std::string printWorkListItem(const WorkListItem wl) = 0;
};

class BFSWorkload : public GraphWorkload
{
  private:
    uint64_t initAddrBase;
    int initIndex;
    uint32_t initValue;
    int numElementsPerLine;
    int atomSize;

  public:
    BFSWorkload(uint64_t init_addr, uint32_t init_value, int atom_size);

    ~BFSWorkload() {}

    virtual void init(PacketPtr pkt, int bit_index_base,
                    std::bitset<MAX_BITVECTOR_SIZE>& needsPush,
                    std::deque<int>& activeBits);
    virtual uint32_t reduce(uint32_t update, uint32_t value);
    virtual uint32_t propagate(uint32_t value, uint32_t weight);
    virtual bool applyCondition(WorkListItem wl);
    virtual bool preWBApply(WorkListItem& wl);
    virtual std::tuple<uint32_t, bool, bool> prePushApply(WorkListItem& wl);
    virtual std::string printWorkListItem(const WorkListItem wl);
};


class PRWorkload : public GraphWorkload
{
  private:
    float alpha;
    float threshold;

    int numElementsPerLine;
    int atomSize;

  public:
    PRWorkload(float alpha, float threshold, int atom_size);

    ~PRWorkload() {}

    virtual void init(PacketPtr pkt, int bit_index_base,
                    std::bitset<MAX_BITVECTOR_SIZE>& needsPush,
                    std::deque<int>& activeBits);
    virtual uint32_t reduce(uint32_t update, uint32_t value);
    virtual uint32_t propagate(uint32_t value, uint32_t weight);
    virtual bool applyCondition(WorkListItem wl);
    virtual bool preWBApply(WorkListItem& wl);
    virtual std::tuple<uint32_t, bool, bool> prePushApply(WorkListItem& wl);
    virtual std::string printWorkListItem(const WorkListItem wl);
};

}

#endif // __ACCL_GRAPH_BASE_GRAPH_WORKLOAD_HH__
