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

#include <cstring>

#include "base/cprintf.hh"
#include "base/intmath.hh"

namespace gem5
{

template<typename T>
float
writeToFloat(T value)
{
    assert(sizeof(T) == sizeof(float));
    float float_form;
    std::memcpy(&float_form, &value, sizeof(float));
    return float_form;
}

template<typename T>
T
readFromFloat(float value)
{
    assert(sizeof(T) == sizeof(float));
    T float_bits;
    std::memcpy(&float_bits, &value, sizeof(float));
    return float_bits;
}

void
BFSWorkload::init(PacketPtr pkt, WorkDirectory* dir)
{
    size_t pkt_size = pkt->getSize();
    uint64_t aligned_addr = roundDown<uint64_t, size_t>(initAddr, pkt_size);

    if (pkt->getAddr() == aligned_addr) {
        int num_elements = (int) (pkt_size / sizeof(WorkListItem));
        WorkListItem items[num_elements];

        pkt->writeDataToBlock((uint8_t*) items, pkt_size);
        int index = (int) ((initAddr - aligned_addr) / sizeof(WorkListItem));
        WorkListItem new_wl = items[index];
        new_wl.tempProp = initValue;
        if (activeCondition(new_wl, items[index])) {
            new_wl.activeNow = true;
            dir->activate(aligned_addr);
        }
        items[index] = new_wl;

        pkt->deleteData();
        pkt->allocate();
        pkt->setDataFromBlock((uint8_t*) items, pkt_size);
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
BFSWorkload::activeCondition(WorkListItem new_wl, WorkListItem old_wl)
{
    return (new_wl.tempProp < old_wl.tempProp) && (old_wl.degree > 0);
}

uint32_t
BFSWorkload::apply(WorkListItem& wl)
{
    wl.prop = wl.tempProp;
    return wl.prop;
}

std::string
BFSWorkload::printWorkListItem(const WorkListItem wl)
{
    return csprintf(
            "WorkListItem{tempProp: %u, prop: %u, degree: %u, edgeIndex: %u}",
            wl.tempProp, wl.prop, wl.degree, wl.edgeIndex
            );
}

// void
// PRWorkload::init(PacketPtr pkt, WorkDirectory* dir)
// {
//     size_t pkt_size = pkt->getSize();
//     int num_elements = (int) (pkt_size / sizeof(WorkListItem));
//     WorkListItem items[num_elements];

//     pkt->writeDataToBlock((uint8_t*) items, pkt_size);
//     bool atom_active = false;
//     for (int i = 0; i < num_elements; i++) {
//         items[i].tempProp = readFromFloat<uint32_t>(0);
//         items[i].prop = readFromFloat<uint32_t>(1 - alpha);
//         atom_active |= activeCondition(items[i]);
//     }
//     if (atom_active) {
//         dir->activate(pkt->getAddr());
//     }
//     pkt->deleteData();
//     pkt->allocate();
//     pkt->setDataFromBlock((uint8_t*) items, pkt_size);
// }

// uint32_t
// PRWorkload::reduce(uint32_t update, uint32_t value)
// {
//     float update_float = writeToFloat<uint32_t>(update);
//     float value_float = writeToFloat<uint32_t>(value);
//     return readFromFloat<uint32_t>(update_float + value_float);
// }

// uint32_t
// PRWorkload::propagate(uint32_t value, uint32_t weight)
// {
//     float value_float = writeToFloat<uint32_t>(value);
//     float weight_float = writeToFloat<uint32_t>(weight);
//     if (weight == 0) {
//         weight_float = 1.0;
//     }
//     return readFromFloat<uint32_t>(alpha * value_float * weight_float);
// }

// bool
// PRWorkload::activeCondition(WorkListItem wl)
// {
//     float temp_float = writeToFloat<uint32_t>(wl.tempProp);
//     float prop_float = writeToFloat<uint32_t>(wl.prop);
//     float dist = std::abs(temp_float - prop_float);
//     return (dist >= threshold) && (wl.degree > 0);
// }

// uint32_t
// PRWorkload::apply(WorkListItem& wl)
// {
//     float temp_float = writeToFloat<uint32_t>(wl.tempProp);
//     float prop_float = writeToFloat<uint32_t>(wl.prop);
//     float delta = (temp_float - prop_float) / wl.degree;
//     uint32_t delta_uint = readFromFloat<uint32_t>(delta);
//     wl.prop = wl.tempProp;
//     return delta_uint;
// }

// std::string
// PRWorkload::printWorkListItem(const WorkListItem wl)
// {
//     float temp_float = writeToFloat<uint32_t>(wl.tempProp);
//     float prop_float = writeToFloat<uint32_t>(wl.prop);
//     return csprintf(
//             "WorkListItem{tempProp: %f, prop: %f, degree: %u, edgeIndex: %u}",
//             temp_float, prop_float, wl.degree, wl.edgeIndex);
// }

} // namespace gem5
