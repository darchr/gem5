/*
 * Copyright (c) 2017 Jason Lowe-Power
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

#include "new/simple_blocking_port.hh"

#include "base/trace.hh"
#include "debug/SimpleBlockingPort.hh"
#include "debug/PermissionTable.hh"

namespace gem5
{

SimpleBlockingPort::SimpleBlockingPort(const SimpleBlockingPortParams &params) :
    SimObject(params),
    event(this),
    stats(this),
    cpuSidePort(params.name + ".cpu_side_port", this),
    memSidePort(params.name + ".mem_side_port", this),
    enablePermissionCheck(params.enable_permission_check),
    creationLatency(params.creation_latency),
    hitLatency(params.hit_latency),
    missLatency(params.miss_latency),
    totalMemorySize(params.total_memory_size),
    cacheSize(params.cache_size),
    segmentSize(params.segment_size),
    cachePolicy(params.cache_policy)
    // blocked(false)
{
    // make sure that the user has defined the totla memory size
    panic_if(totalMemorySize == 0,
        "The MMP needs to know the size of the memory!\n");
    // Calculate the total size of the memory's permission table. Each MMP
    // entry is addr + size + permission (64 + 64 + 2 = 130 bits). We're doing
    // a bit of cheating and making sure that the maximum size can be 2^62
    // instead of total 64 bits (never going to happen anyway). Converting
    // this to bytes, we have: 128 / 8 = 16 Bytes.

    // We'll conside the worst case MMP. There are permissions per 4 KiB of
    // memory. There can be totalMemorySize / 2^12 number of MMP entries.
    // Each entry is of 2^4 Bytes.
    total_entries = totalMemorySize / segmentSize;
    DPRINTF(PermissionTable, "MMP table has %lu entries\n", total_entries);

    // Now set up the cache. We don't really need a lot of cache to maintain
    // this table.
    // We maintain a couple of states of the address in the cache. I am keeping
    // a couple of values to make sure that it is compatible with all tyoes of 
    // caches.
    // address, [is_cached (bool), last_accessed (Tick), access_count (int)]
    // The map is initialized with a dummy entry in the beginning. We might
    // remove this in the future.
    // permission_table.insert({uint64_t(-1), new struct cache_entry_vector});
    
    // Whether an entry is cached or not is detemined by the number of
    // is_cache number.
    total_cached_entries = 0;

    // need to figure out the maximum number of cachable entries.
    max_cached_entries = cacheSize / 2;

    DPRINTF(PermissionTable, "MMP cache has %lu entries\n", max_cached_entries);
}

Port &
SimpleBlockingPort::getPort(const std::string &if_name, PortID idx)
{
    panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

    // This is the name from the Python SimObject declaration (SimpleBlockingPort.py)
    if (if_name == "mem_side_port") {
        return memSidePort;
    } else if (if_name == "cpu_side_port") {
        return cpuSidePort;
    } else {
        // pass it along to our super class
        return SimObject::getPort(if_name, idx);
    }
}

AddrRangeList
SimpleBlockingPort::CPUSidePort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

Tick
SimpleBlockingPort::CPUSidePort::recvAtomic(PacketPtr pkt)
{
    // Just forward to the memobj.
    owner->handleAtomic(pkt);
    return Tick();
}
void
SimpleBlockingPort::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    // Just forward to the memobj.
    return owner->handleFunctional(pkt);
}

bool
SimpleBlockingPort::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    // rewriting this method in a simpler way
    DPRINTF(SimpleBlockingPort, "Got request for addr %#x\n", pkt->getAddr());

    // owner->processEvent();

    if (owner->memSidePort.sendTimingReq(pkt)) {
        requestMap[pkt->getAddr()] = pkt;
        return true;
    }

    DPRINTF(SimpleBlockingPort, "Failed to send %#x\n", pkt->getAddr());
    blockedRequests.push(pkt);
    return false;
}

void
SimpleBlockingPort::CPUSidePort::recvRespRetry()
{
    // If retry is called by the cpu side port, then it is important to send 
    // the packet to the memside request.
    DPRINTF(SimpleBlockingPort, "Retry logic called for outstanding responses.\n");

    while (!owner->memSidePort.blockedResponses.empty()) {
        PacketPtr pkt = owner->memSidePort.blockedResponses.front();
        if (!owner->cpuSidePort.sendTimingResp(pkt)) {
            owner->memSidePort.blockedResponses.pop();
            owner->memSidePort.responseMap.erase(pkt->getAddr()); // = pkt;
        }
        else
            break;
    }
}

bool
SimpleBlockingPort::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    // find the request for which the response is received!
    auto it = owner->cpuSidePort.requestMap.find(pkt->getAddr());
    DPRINTF(SimpleBlockingPort, "Received response for %#x\n", pkt->getAddr());
    assert(pkt->isResponse() && !pkt->isRequest());
    // assert(pkt)
    // we must find it! why?
    if (it != owner->cpuSidePort.requestMap.end()) {
        // we no longer need to keep a track of this request.
        // owner->outstandingRequests.erase(it);

        // now the response packet can be blocked as well.
        if (owner->cpuSidePort.sendTimingResp(pkt)) {
            DPRINTF(SimpleBlockingPort, "Successfully responded to the cpu port!\n");
            // delete the response now
            owner->cpuSidePort.requestMap.erase(pkt->getAddr());
            return true;
        }
        else {
            // We'll call response retry for these packets.
            DPRINTF(SimpleBlockingPort, "Couldn't respond to the cpu with %#x!\n", pkt->getAddr());
            owner->memSidePort.blockedResponses.push(pkt);
            owner->memSidePort.responseMap[pkt->getAddr()] = pkt;
            return false; 

        }
        // return owner->cpuSidePort.sendTimingResp(pkt);
    }
    else {
        panic("Saw an unexpected response for %#x\n", pkt->getAddr());
    }

    // unreachable code!
    return false;
}

void
SimpleBlockingPort::MemSidePort::recvReqRetry()
{
    // start clearing the request queue!
    DPRINTF(SimpleBlockingPort, "Retry logic called for outstanding requests.\n");


    // want to see all the packets that are queued
    std::queue<gem5::PacketPtr> temp(owner->cpuSidePort.blockedRequests);
    while (!temp.empty()) {
        DPRINTF(SimpleBlockingPort, "Printing request elements: %#x!\n", temp.front()->getAddr());
        temp.pop();
    }

    while(!owner->cpuSidePort.blockedRequests.empty()) {
        PacketPtr pkt = owner->cpuSidePort.blockedRequests.front();
        if (owner->memSidePort.sendTimingReq(pkt)) {
            DPRINTF(SimpleBlockingPort, "Retry successful for packet with addr %#x\n",
                                                            pkt->getAddr());
            owner->cpuSidePort.requestMap[pkt->getAddr()] = pkt;
            owner->cpuSidePort.blockedRequests.pop();
        }
        else
            break;
    }
    owner->cpuSidePort.sendRetryReq();
}

void
SimpleBlockingPort::MemSidePort::recvRangeChange()
{
    owner->sendRangeChange();
}

// make sure to implement the caching methods here to quickly copy paste them,
// if needed.
gem5::Tick
SimpleBlockingPort::isCachedRequest(gem5::Addr addr) {
    /*
    Simple caching function that determines the caching variable from the
    class contructor and then makes sure to return where the given address
    has the values in the cache.
    
    @params
    addr: address to check inside the cache
    
    :returns:
        A latency value to tell the user if this is a cache hit.
    */
    
    // figure out what kind of cache I am using.
    ++stats.numPermissionTableAccesses;

    if (cachePolicy == "lru") {
        return simpleLRU(addr);
    }
    else if (cachePolicy == "mru") {
        return simpleMRU(addr);
    }
    else if (cachePolicy == "random") {
        return simpleRandom(addr);
    }
    else {
        // unknown caching policy
        panic("Unknown caching policy!");
        // uncrachable code.
        return false;
    }
}

gem5::Tick
SimpleBlockingPort::simpleLRU(gem5::Addr addr) {
    // ideally see if there is an entry (MMP)
    auto lookup = permission_table.find(addr);

    // There can be a variable latency added for this lookup in the cache of
    // MMP.
    Tick latency = 0;

    if (lookup != permission_table.end()) {
        // found the entry in the permission table. see if this is cached.
        DPRINTF(PermissionTable, "PLB hit for addr %#x\n", addr);
        if (lookup->second->is_cached == true) {
            latency = hitLatency;
            // since this is LRU, increment the count by 1
            lookup->second->access_count++;
            ++stats.numPermissionTableCacheHits;
        }
        else {
            DPRINTF(PermissionTable, "PLB miss for addr %#x\n", addr);
            // this entry is not cached.
            latency = missLatency;
            // find a suitable location to cache this entry.
            // see what is the cache occupancy until now.
            if (total_cached_entries < max_cached_entries) {
                // there is space in the cache. just create a new entry
                lookup->second->is_cached = true;
                // lru ignores last access.
                lookup->second->access_count = 1;
                total_cached_entries++;
            }
            else {
                // need to replace something :(
                int min_count = INT_MAX;
                gem5::Addr key;
                for (auto it = permission_table.begin();
                        it !=  permission_table.end(); it++) {
                    if (min_count < it->second->access_count &&
                                            it->second->is_cached == true) {
                        min_count = it->second->access_count;
                        key = it->first;
                    }
                }
                // delete the min_count entry!
                permission_table[key]->is_cached = false;
                permission_table[key]->access_count = 0;

                // make sure to update the current lookup
                lookup->second->is_cached = true;
                lookup->second->access_count = 1;
                
            }
        }
    }
    else {
        // the entry doesnt exist in the permission table. The mmp needs to
        // create this first and no matter what it does, this will be a
        // cache miss.
        ++stats.numPermissionTableEntries;
        DPRINTF(PermissionTable, "Creating permission entry for addr %#x\n",
                                                                        addr);
        latency = creationLatency + missLatency;

        // create an entry first
        struct cache_entry_vector *cve = new struct cache_entry_vector;
        cve->is_cached = false;
        cve->access_count = 1;

        // insert this entry to the table.
        permission_table.insert({addr, cve});

        // see if there is space in the cache for us to cache it.
        if (total_cached_entries < max_cached_entries) {
            permission_table[addr]->is_cached = true;
            total_cached_entries++;
        }
        else {
            // this entry must be cached and there is no more space in the
            // cache.
            int min_count = INT_MAX;
            gem5::Addr key;
            for (auto it = permission_table.begin();
                            it !=  permission_table.end(); it++) {
                if (min_count < it->second->access_count && 
                                            it->second->is_cached == true) {
                    min_count = it->second->access_count;
                    key = it->first;
                }
            }
            // delete the min_count entry!
            permission_table[key]->is_cached = false;
            permission_table[key]->access_count = 0;
            permission_table[addr]->is_cached = true;
            permission_table[addr]->access_count = 1;
        }
    }

    // check if this address is in the cache
    return latency;
    
}

gem5::Tick
SimpleBlockingPort::simpleMRU(gem5::Addr addr) {
    // ideally see if there is an entry (MMP)
    auto lookup = permission_table.find(addr);

    // There can be a variable latency added for this lookup in the cache of
    // MMP.
    Tick latency = 0;

    if (lookup != permission_table.end()) {
        // found the entry in the permission table. see if this is cached.
        if (lookup->second->is_cached == true) {
            latency = hitLatency;
            lookup->second->last_accessed = gem5::curTick();
        }
        else {
            // this entry is not cached.
            latency = missLatency;
            // find a suitable location to cache this entry.
            // see what is the cache occupancy until now.
            if (total_cached_entries < max_cached_entries) {
                // there is space in the cache. just create a new entry
                lookup->second->is_cached = true;
                // lru ignores last access.
                lookup->second->last_accessed = gem5::curTick();
                total_cached_entries++;
            }
            else {
                // need to replace something :(
                gem5::Tick max_count = 0;
                gem5::Addr key;
                for (auto it = permission_table.begin();
                        it !=  permission_table.end(); it++) {
                    if (max_count > it->second->last_accessed &&
                                            it->second->is_cached == true) {
                        max_count = it->second->last_accessed;
                        key = it->first;
                    }
                }
                // delete the min_count entry!
                permission_table[key]->is_cached = false;

                // make sure to update the current lookup
                lookup->second->is_cached = true;
                lookup->second->last_accessed = gem5::curTick();
            }
        }
    }
    else {
        // the entry doesnt exist in the permission table. The mmp needs to
        // create this first and no matter what it does, this will be a
        // cache miss.
        ++stats.numPermissionTableEntries;
        latency = creationLatency + missLatency;

        // create an entry first
        struct cache_entry_vector *cve = new struct cache_entry_vector;
        cve->is_cached = false;
        cve->last_accessed = gem5::curTick();

        // insert this entry to the table.
        permission_table.insert({addr, cve});

        // see if there is space in the cache for us to cache it.
        if (total_cached_entries < max_cached_entries) {
            permission_table[addr]->is_cached = true;
            total_cached_entries++;
        }
        else {
            // this entry must be cached and there is no more space in the
            // cache.
            gem5::Tick max_count = 0;
            gem5::Addr key;
            for (auto it = permission_table.begin();
                            it !=  permission_table.end(); it++) {
                if (max_count > it->second->last_accessed && 
                                            it->second->is_cached == true) {
                    max_count = it->second->last_accessed;
                    key = it->first;
                }
            }
            // delete the min_count entry!
            permission_table[key]->is_cached = false;
            permission_table[addr]->is_cached = true;
            permission_table[addr]->last_accessed = gem5::curTick();
        }
    }

    // check if this address is in the cache
    return latency;
}
gem5::Tick
SimpleBlockingPort::simpleRandom(gem5::Addr addr) {
    // ideally see if  is an entry (MMP)
    auto lookup = permission_table.find(addr);

    // There can be a variable latency added for this lookup in the cache of
    // MMP.
    Tick latency = 0;

    if (lookup != permission_table.end()) {
        // found the entry in the permission table. see if this is cached.
        if (lookup->second->is_cached == true) {
            latency = hitLatency;
        }
        else {
            // this entry is not cached.
            latency = missLatency;
            // find a suitable location to cache this entry.
            // see what is the cache occupancy until now.
            if (total_cached_entries < max_cached_entries) {
                // there is space in the cache. just create a new entry
                lookup->second->is_cached = true;
                total_cached_entries++;
            }
            else {
                // need to replace something :(
                srand(time(NULL));
                // randomly choose an index to replace
                int index = (rand() % (permission_table.size() + 1));

                // FIXME:
                // need to travel to that index.
                int i = 0;
                for (auto it = permission_table.begin();
                        it !=  permission_table.end(); it++, i++) {
                    if (i == index) {
                        it->second->is_cached = false;
                        lookup->second->is_cached = true;
                        break;
                    }
                }
            }
        }
    }
    else {
        // the entry doesnt exist in the permission table. The mmp needs to
        // create this first and no matter what it does, this will be a
        // cache miss.
        ++stats.numPermissionTableEntries;
        latency = creationLatency + missLatency;

        // create an entry first
        struct cache_entry_vector *cve = new struct cache_entry_vector;
        cve->is_cached = false;
        cve->last_accessed = gem5::curTick();

        // insert this entry to the table.
        permission_table.insert({addr, cve});

        // see if there is space in the cache for us to cache it.
        if (total_cached_entries < max_cached_entries) {
            permission_table[addr]->is_cached = true;
            total_cached_entries++;
        }
        else {
            // need to replace something :(
            srand(time(NULL));
            // randomly choose an index to replace
            int index = (rand() % (permission_table.size() + 1));

            // FIXME:
            // need to travel to that index.
            int i = 0;
            for (auto it = permission_table.begin();
                    it !=  permission_table.end(); it++, i++) {
                if (i == index) {
                    it->second->is_cached = false;
                    lookup->second->is_cached = true;
                    break;
                }
            }
        }
    }

    // check if this address is in the cache
    return latency;
}

Tick
SimpleBlockingPort::handleAtomic(PacketPtr pkt)
{
    // Just pass this on to the memory side to handle for now and do nothing!
    memSidePort.sendAtomic(pkt);
    return Tick();
}
void
SimpleBlockingPort::handleFunctional(PacketPtr pkt)
{
    // Just pass this on to the memory side to handle for now.
    memSidePort.sendFunctional(pkt);
}

// make sure the event is correctly set for this simobject
void
SimpleBlockingPort::processEvent() {
    // This is only called if the user wants to add permission checks
    // make sure that this address is scheduled with some additional latency.
    // DPRINTF(PermissionTable, "Scheduling addr %#x with %lu latency\n",
    //                                     pkt->getAddr(), class_latency);
    // the class latency must be set before the event can be called.
    DPRINTF(PermissionTable, "Scheduling this* with %lu latency\n",
                                    class_latency);
    // assert(class_latency == hitLatency || 
    //             class_latency == creationLatency + missLatency || 
    //             class_latency == missLatency);
    scheduleNewEvent();
}
void
SimpleBlockingPort::startup()
{
    schedule(event, 10);
}

void
SimpleBlockingPort::scheduleNewEvent() {
    schedule(event, curTick() + 10);
}

AddrRangeList
SimpleBlockingPort::getAddrRanges() const
{
    DPRINTF(SimpleBlockingPort, "Sending new ranges\n");
    // Just use the same ranges as whatever is on the memory side.
    return memSidePort.getAddrRanges();
}

void
SimpleBlockingPort::sendRangeChange()
{
    cpuSidePort.sendRangeChange();
}

SimpleBlockingPort::StatGroup::StatGroup(statistics::Group *parent)
    : statistics::Group(parent),
    ADD_STAT(numIncomingCPUSidePackets, statistics::units::Count::get(),
        "Number of LLC incoming packets"),
    ADD_STAT(numOutgoingMemSidePackets, statistics::units::Count::get(),
        "Count the number of outgoing memory packets"),
    ADD_STAT(numOutgoingTrafficPackets, statistics::units::Count::get(),
        "Count the number of traffic packets"),
    ADD_STAT(numPermissionTableEntries, statistics::units::Count::get(),
        "Number of entries in the permission table"),
    ADD_STAT(numPermissionTableCacheHits, statistics::units::Count::get(),
        "Number of hits in the permission table cache"),
    ADD_STAT(numPermissionTableAccesses, statistics::units::Count::get(),
        "total number of accesses into the permission table (redundant!)")
{
    using namespace statistics;
}

} // namespace gem5
