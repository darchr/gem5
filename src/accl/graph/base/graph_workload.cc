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

#include "accl/graph/base/graph_workload.hh"

namespace gem5
{

BFSWorkload::BFSWorkload(uint64_t init_addr, uint32_t init_value, int atom_size):
    GraphWorkload(), initValue(init_value), atomSize(atom_size)
{
    initAddrBase = roundDown<uint64_t, int>(init_addr, atomSize);
    initIndex = (init_addr - initAddrBase) / atomSize;
    numElementsPerLine = atomSize / sizeof(WorkListItem);
}


void
BFSWorkload::init(PacketPtr pkt, int bit_index_base,
                std::bitset<MAX_BITVECTOR_SIZE>& needsPush, 
                std::deque<int>& activeBits)
{
    if (pkt->getAddr() == initAddrBase) {
        WorkListItem items[numElementsPerLine];

        pkt->writeDataToBlock((uint8_t*) items, atomSize);

        items[initIndex].tempProp = initValue;
        items[initIndex].prop = initValue;
        needsPush[bit_index_base + initIndex] = 1;
        activeBits.push_back(bit_index_base + initIndex);

        pkt->deleteData();
        pkt->allocate();
        pkt->setDataFromBlock((uint8_t*) items, atomSize);
    }

}

uint32_t
BFSWorkload::reduce(uint32_t update, uint32_t value)
{
    return std::min(update, value);
}

uint32_t
BFSWorkload::propagate(uint32_t value, uint32_t weight)
{
    return value + 1;
}

bool
BFSWorkload::applyCondition(WorkListItem wl)
{
    return wl.tempProp < wl.prop;
}

bool
BFSWorkload::preWBApply(WorkListItem& wl)
{
    if (applyCondition(wl)) {
        wl.prop = wl.tempProp;
        if (wl.degree > 0) {
            return true;
        }
    }
    return false;
}

std::tuple<uint32_t, bool, bool>
BFSWorkload::prePushApply(WorkListItem& wl)
{
    uint32_t value = wl.prop;
    return std::make_tuple(value, true, false);
}


uint32_t
PRWorkload::reduce(uint32_t update, uint32_t value)
{
    return update+value;
}

uint32_t
PRWorkload::propagate(uint32_t value, uint32_t weight)
{
    return (alpha*value*weight);
}

bool
PRWorkload::applyCondition(WorkListItem wl)
{
    return wl.tempProp != wl.prop;
}

bool
PRWorkload::preWBApply(WorkListItem& wl)
{
    if (applyCondition(wl)) {
        if (wl.degree > 0) {
            return true;
        }
    }
    return false;
}

std::tuple<uint32_t, bool, bool>
PRWorkload::prePushApply(WorkListItem& wl)
{
    uint32_t delta = abs(wl.prop - wl.tempProp)/wl.degree;
    if (delta > threshold) {
        return std::make_tuple(delta, true, true);
    }
    uint32_t value = wl.tempProp;
    return std::make_tuple(value, false, false);
}

} // namespace gem5
