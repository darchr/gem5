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
#include "base/intmath.hh"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <deque>

namespace gem5
{

struct __attribute__ ((packed)) WorkListItem
{
    uint32_t tempProp : 32;
    uint32_t prop : 32;
    uint32_t edgeIndex : 32;
    uint32_t degree : 30;
    bool activeNow: 1;
    bool activeFuture: 1;

    std::string to_string()
    {
        return csprintf("WorkListItem{tempProp: %u, prop: %u, edgeIndex: %u, "
                        "degree: %u, activeNow: %s, activeFuture: %s}",
                        tempProp, prop, edgeIndex, degree,
                        activeNow ? "true" : "false",
                        activeFuture ? "true" : "false");
    }

    WorkListItem():
        tempProp(0),
        prop(0),
        edgeIndex(0),
        degree(0),
        activeNow(false),
        activeFuture(false)
    {}

    WorkListItem(uint32_t temp_prop, uint32_t prop,
                uint32_t degree, uint32_t edge_index,
                bool active_now, bool active_future):
        tempProp(temp_prop), prop(prop), edgeIndex(edge_index), degree(degree),
        activeNow(active_now), activeFuture(active_future)
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

    Edge(): weight(0), neighbor(0) {}

    Edge(uint16_t weight, uint64_t neighbor):
        weight(weight),
        neighbor(neighbor)
    {}
};

struct __attribute__ ((packed)) MirrorVertex
{
    uint32_t vertexId : 32;
    uint32_t prop : 32;
    uint32_t edgeIndex : 32;
    uint32_t degree : 30;
    bool activeNow: 1;
    bool activeNext: 1;

    std::string to_string()
    {
        return csprintf("MirrorVertex{vertexId: %u, prop: %u, edgeIndex: %u, "
                        "degree: %u, activeNow: %s, activeNext: %s}",
                        vertexId, prop, edgeIndex, degree,
                        activeNow ? "true" : "false",
                        activeNext ? "true" : "false");
    }
    MirrorVertex():
        vertexId(-1),
        prop(-1),
        edgeIndex(-1),
        degree(-1),
        activeNow(false),
        activeNext(false)
    {}

    MirrorVertex(uint32_t vertex_id, uint32_t prop, uint32_t degree,
                uint32_t edge_index, bool active_now, bool active_next):
                vertexId(vertex_id), prop(prop), edgeIndex(edge_index),
                degree(degree), activeNow(active_now), activeNext(active_next)
    {}

};

static_assert(isPowerOf2(sizeof(WorkListItem)));
static_assert(isPowerOf2(sizeof(Edge)));
static_assert(isPowerOf2(sizeof(MirrorVertex)));

struct MetaEdge {
    uint64_t src;
    uint64_t dst;
    uint32_t weight;
    uint32_t value;

    MetaEdge(): src(0), dst(0), weight(0), value(0)
    {}
    MetaEdge(uint64_t src, uint64_t dst, uint32_t weight, uint32_t value):
        src(src), dst(dst), weight(weight), value(value)
    {}

    std::string to_string()
    {
        return csprintf("MetaEdge{src: %lu, dst:%lu, weight: %u, value: %u}",
                                                    src, dst, weight, value);
    }
};

struct Update {
    uint64_t src;
    uint64_t dst;
    uint32_t value;

    Update(): src(0), dst(0), value(0)
    {}
    Update(uint64_t src, uint64_t dst, uint32_t value):
        src(src), dst(dst), value(value)
    {}

    std::string to_string()
    {
        return csprintf("Update{src: %lu, dst:%lu, value: %u}",
                                                src, dst, value);
    }
};

template<typename T>
class UniqueFIFO
{
  private:
    int cap;
    int pop;

    int* added;
    int* deleted;
    std::deque<T> container;

  public:
    UniqueFIFO() {
        cap = 0;
        pop = 0;
        added = nullptr;
        deleted = nullptr;
    }

    UniqueFIFO(int size) {
        cap = size;
        pop = 0;

        added = (int*) new int [cap];
        deleted = (int*) new int [cap];

        for (int i = 0; i < cap; i++) {
            added[i] = 0;
            deleted[i] = 0;
        }
        container.clear();
    }

    ~UniqueFIFO() {
        delete [] added;
        delete [] deleted;
    }

    void fix_front() {
        while(true) {
            T elem = container.front();
            if (deleted[elem] > 0) {
                deleted[elem]--;
                added[elem]--;
                container.pop_front();
            } else {
                assert(deleted[elem] == 0);
                assert(added[elem] == 1);
                break;
            }
        }
    }

    T front() {
        fix_front();
        return container.front();
    }

    size_t size() {
        return pop;
    }

    void clear() {
        pop = 0;
        for (int i = 0; i < cap; i++) {
            added[i] = 0;
            deleted[i] = 0;
        }
        container.clear();
    }

    bool empty() {
        return size() == 0;
    }

    bool find(T item) {
        assert(added[item] >= 0);
        assert(deleted[item] >= 0);
        int diff = added[item] - deleted[item];
        assert((diff == 0) || (diff == 1));
        return (diff == 1);
    }

    void push_back(T item) {
        if (!find(item)) {
            added[item]++;
            pop++;
            container.push_back(item);
        }
    }

    void pop_front() {
        T elem = front();
        assert(added[elem] == 1);
        added[elem] = 0;
        pop--;
        container.pop_front();
    }

    void erase(T item) {
        assert(find(item));
        deleted[item]++;
        pop--;
    }

    void operator=(const UniqueFIFO<T>& rhs) {
        cap = rhs.cap;
        pop = rhs.pop;
        container = rhs.container;
        added = (int*) new int [cap];
        deleted = (int*) new int [cap];
        std::memcpy(added, rhs.added, cap * sizeof(int));
        std::memcpy(deleted, rhs.deleted, cap * sizeof(int));
    }
};

}

#endif // __ACCL_GRAPH_BASE_DATA_STRUCTS_HH__
