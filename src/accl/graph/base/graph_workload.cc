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
            "WorkListItem{tempProp: %u, prop: %u, degree: %u, "
            "edgeIndex: %u, activeNow: %s, activeFuture: %s}",
            wl.tempProp, wl.prop, wl.degree, wl.edgeIndex,
            wl.activeNow ? "true" : "false",
            wl.activeFuture ? "true" : "false");
}

uint32_t
BFSVisitedWorkload::propagate(uint32_t value, uint32_t weight) {
    return value;
}

void
CCWorkload::init(PacketPtr pkt, WorkDirectory* dir)
{
    Addr pkt_addr = pkt->getAddr();
    size_t pkt_size = pkt->getSize();
    int num_elements = (int) (pkt_size / sizeof(WorkListItem));
    WorkListItem items[num_elements];

    pkt->writeDataToBlock((uint8_t*) items, pkt_size);
    bool atom_active = false;
    for (int i = 0; i < num_elements; i++) {
        WorkListItem new_wl = items[i];
        new_wl.tempProp = (int) (pkt_addr / sizeof(WorkListItem)) + i;
        new_wl.activeNow = activeCondition(new_wl, items[i]);
        atom_active |= new_wl.activeNow;
        items[i] = new_wl;
    }
    if (atom_active) {
        dir->activate(pkt->getAddr());
    }
    pkt->deleteData();
    pkt->allocate();
    pkt->setDataFromBlock((uint8_t*) items, pkt_size);
}

uint32_t
SSSPWorkload::propagate(uint32_t value, uint32_t weight)
{
    return value + weight;
}

void
BSPPRWorkload::init(PacketPtr pkt, WorkDirectory* dir)
{
    size_t pkt_size = pkt->getSize();
    int num_elements = (int) (pkt_size / sizeof(WorkListItem));
    WorkListItem items[num_elements];

    pkt->writeDataToBlock((uint8_t*) items, pkt_size);
    bool atom_active = false;
    for (int i = 0; i < num_elements; i++) {
        WorkListItem new_wl = items[i];
        new_wl.tempProp = readFromFloat<uint32_t>(1 - alpha);
        new_wl.prop = readFromFloat<uint32_t>(1);
        new_wl.activeNow = activeCondition(new_wl, items[i]);
        atom_active |= new_wl.activeNow;
        items[i] = new_wl;
    }
    if (atom_active) {
        dir->activate(pkt->getAddr());
    }
    pkt->deleteData();
    pkt->allocate();
    pkt->setDataFromBlock((uint8_t*) items, pkt_size);
}

uint32_t
BSPPRWorkload::reduce(uint32_t update, uint32_t value)
{
    float update_float = writeToFloat<uint32_t>(update);
    float value_float = writeToFloat<uint32_t>(value);
    return readFromFloat<uint32_t>(update_float + value_float);
}

uint32_t
BSPPRWorkload::propagate(uint32_t value, uint32_t weight)
{
    float value_float = writeToFloat<uint32_t>(value);
    return readFromFloat<uint32_t>(alpha * value_float);
}

bool
BSPPRWorkload::activeCondition(WorkListItem new_wl, WorkListItem old_wl)
{
    return (old_wl.degree > 0);
}

uint32_t
BSPPRWorkload::apply(WorkListItem& wl)
{
    float prop_float = writeToFloat<uint32_t>(wl.prop);
    float delta = prop_float / wl.degree;
    uint32_t delta_uint = readFromFloat<uint32_t>(delta);
    return delta_uint;
}

void
BSPPRWorkload::interIterationInit(WorkListItem& wl)
{
    wl.prop = wl.tempProp;
    wl.tempProp = readFromFloat<uint32_t>(1 - alpha);
    wl.activeFuture = (wl.degree > 0);
}

std::string
BSPPRWorkload::printWorkListItem(const WorkListItem wl)
{
    float temp_float = writeToFloat<uint32_t>(wl.tempProp);
    float prop_float = writeToFloat<uint32_t>(wl.prop);
    return csprintf(
            "WorkListItem{tempProp: %f, prop: %f, degree: %u, "
            "edgeIndex: %u, activeNow: %s, activeFuture: %s}",
            temp_float, prop_float, wl.degree, wl.edgeIndex,
            wl.activeNow ? "true" : "false",
            wl.activeFuture ? "true" : "false");
}

void
BSPBCWorkload::init(PacketPtr pkt, WorkDirectory* dir)
{
    int pkt_size = pkt->getSize();
    int aligned_addr = roundDown<uint32_t, size_t>(initAddr, pkt_size);

    if (aligned_addr == pkt->getAddr()) {
        int num_elements = pkt_size / sizeof(WorkListItem);
        WorkListItem items[num_elements];
        pkt->writeDataToBlock((uint8_t*) items, pkt_size);
        int index = (initAddr - aligned_addr) / sizeof(WorkListItem);
        WorkListItem new_wl = items[index];
        uint32_t prop = 0;
        prop |= initValue;
        // NOTE: Depth of the initial vertex is 0.
        prop &= (4294967295U >> 8);
        new_wl.tempProp = prop;
        new_wl.prop = prop;
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
BSPBCWorkload::reduce(uint32_t update, uint32_t value)
{
    uint32_t update_depth = (update & depthMask) >> 24;
    uint32_t update_count = (update & countMask);
    assert(update_depth == (currentDepth - 1));
    uint32_t value_depth = (value & depthMask) >> 24;
    uint32_t value_count = (value & countMask);
    if (value_depth == 255) {
        value_depth = update_depth;
        value_count = 0;
    }
    if (value_depth == currentDepth) {
        value_count += update_count;
    }
    uint32_t ret = 0;
    ret |= value_count;
    warn_if(value_count > 16777215, "value count has grown bigger than 16777125."
                                " This means the algorithm result might not be correct."
                                " However, the traversal will not be affected."
                                " Therefore, performane metrics could be used.");
    // HACK: Make sure to always set the depth correctly even if count
    // exceeds the 2^24-1 limit. Here we reset the depth section of ret.
    ret &= (4294967295U >> 8);
    // NOTE: Now that the depth is securely reset we can copy the correct value.
    ret |= (value_depth << 24);
    return ret;
}

uint32_t
BSPBCWorkload::propagate(uint32_t value, uint32_t weight)
{
    return value;
}

uint32_t
BSPBCWorkload::apply(WorkListItem& wl)
{
    return wl.prop;
}

void
BSPBCWorkload::interIterationInit(WorkListItem& wl)
{
    wl.prop = wl.tempProp;
}

bool
BSPBCWorkload::activeCondition(WorkListItem new_wl, WorkListItem old_wl)
{
    uint32_t depth = (new_wl.tempProp & depthMask) >> 24;
    return (depth == currentDepth);
}

std::string
BSPBCWorkload::printWorkListItem(WorkListItem wl)
{
    uint32_t temp_depth = (wl.tempProp & depthMask) >> 24;
    uint32_t temp_count = (wl.tempProp & countMask);
    uint32_t depth = (wl.prop & depthMask) >> 24;
    uint32_t count = (wl.prop & countMask);
    return csprintf(
            "WorkListItem{tempProp: (depth: %d, count: %d), "
            "prop: (depth: %d, count: %d), degree: %u, "
            "edgeIndex: %u, activeNow: %s, activeFuture: %s}",
            temp_depth, temp_count, depth, count, wl.degree, wl.edgeIndex,
            wl.activeNow ? "true" : "false",
            wl.activeFuture ? "true" : "false");
}

} // namespace gem5
