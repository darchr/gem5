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

#ifndef __ACCL_GRAPH_BASE_DATA_STRUCTS_HH__
#define __ACCL_GRAPH_BASE_DATA_STRUCTS_HH__

#include "base/cprintf.hh"

namespace gem5
{

struct __attribute__ ((packed)) WorkListItem
{
    uint32_t tempProp : 32;
    uint32_t prop : 32;
    uint32_t degree : 32;
    uint32_t edgeIndex : 32;

    std::string to_string()
    {
        return csprintf(
        "WorkListItem{temp_prop: %u, prop: %u, degree: %u, edgeIndex: %u}",
        tempProp, prop, degree, edgeIndex);
    }

    WorkListItem():
        tempProp(0),
        prop(0),
        degree(0),
        edgeIndex(0)
    {}

    WorkListItem(uint32_t temp_prop, uint32_t prop,
                uint32_t degree, uint32_t edge_index):
        tempProp(temp_prop),
        prop(prop),
        degree(degree),
        edgeIndex(edge_index)
    {}

};

struct __attribute__ ((packed)) Edge
{
    uint16_t weight : 16;
    uint64_t neighbor : 48;

    std::string to_string()
    {
        return csprintf("Edge{weight: %u, neighbor: %lu}", weight, neighbor);
    }

    Edge(uint16_t weight, uint64_t neighbor):
        weight(weight),
        neighbor(neighbor)
    {}
};

}

#endif // __ACCL_GRAPH_BASE_DATA_STRUCTS_HH__
