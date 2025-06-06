#include "new/clocked_permission.hh"

#include "base/trace.hh"

#include "debug/PermissionTable.hh"
#include "debug/PermissionTableEvent.hh"
#include "debug/ClockedPermissionDebug.hh"

namespace gem5 {

ClockedPermission::ClockedPermission(const ClockedPermissionParams &params) :
    ClockedObject(params),
    memSidePort(params.name + ".mem_side_port", *this),
    // cpuSidePort(params.name + ".cpu_side_port", this),
    enablePermissionCheck(params.enable_permission_check),
    creationLatency(params.creation_latency),
    hitLatency(params.hit_latency),
    missLatency(params.miss_latency),
    totalMemorySize(params.total_memory_size),
    cacheSize(params.cache_size),
    segmentSize(params.segment_size),
    cachePolicy(params.cache_policy),
    stats(this)
{
    for (int i = 0 ; i < params.port_cpu_side_ports_connection_count; i++)
        cpuSidePorts.emplace_back(
            name() + csprintf(".cpu_side_ports[%d]", i), i, *this, i
        );
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

    // create a mask for the the segment size. Each cached entry will be 64 B
    // and each segment table size will be segmentSize.
    segment_mask = 0xFFFFFFFF & !(segmentSize - 1);
    cache_mask = 0xFFFFFFF0;

    // The number of the entries in MMP is variable and we need to count the
    // varying number of entries
    permission_table_entries = 0;

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

    DPRINTF(PermissionTable, "MMP cache has %lu entries\n",
                                                        max_cached_entries);
}

AddrRangeList
ClockedPermission::getAddrRanges() const
{
    return memSidePort.getAddrRanges();
}

Tick
ClockedPermission::recvAtomic(PacketPtr pkt)
{
    memSidePort.sendAtomic(pkt);
    return Tick();
}

void
ClockedPermission::recvFunctional(PacketPtr pkt)
{
    memSidePort.sendFunctional(pkt);
}

bool
ClockedPermission::recvTimingReq(PacketPtr pkt, uint64_t packet_id) {
    // If the permission tables are enabled by the user.
    if (enablePermissionCheck) {
        // TODO: 
        // Change the return structure to a struct with <bool, gem5::Tick>.
        // If this is a miss, then create an additional packet that accesses
        // the memory to fetch the data from the permission table. 
        struct permission_handler status = isCachedRequest(pkt->getAddr());

        // TODO:
        // Create an additional dummy packet that handles a PLB miss. 
        
        // TODO:
        // Schedule a new AccessEvent with this latency. Since this is a clock
        // edge, both the rising and the falling edges can be used in this
        // case. Maybe cite the dual edged flip flop if needed in the paper.
        schedule(new EventFunctionWrapper([this, pkt]{ },
                        name() + ".accessEvent", true),
                        clockEdge(static_cast<Cycles>(status.latency / 2)));

    }

    // business as usual:
    if (memSidePort.sendTimingReq(pkt)) {
        // Send successful, keep the packet_id for later.
        portMap[pkt->id] = packet_id;
        return true;
    }
    DPRINTF(ClockedPermissionDebug, "Failed to send %#x on port %lu\n",
                                                    pkt->getAddr(), packet_id);
    retry_queue.push(packet_id);
    return false;
}

void
ClockedPermission::recvRespRetry(const PortID id) {
    memSidePort.sendRetryResp();
    DPRINTF(ClockedPermissionDebug, "Found the issue! Retry called for %lu\n",
                                                                        id);
}

bool
ClockedPermission::recvTimingResp(PacketPtr pkt) {
    PacketId id = pkt->id;
    return cpuSidePorts[portMap[id]].sendTimingResp(pkt);
}

void
ClockedPermission::recvReqRetry() {
    while (!retry_queue.empty()) {
        uint64_t id = retry_queue.front();
        cpuSidePorts[id].sendRetryReq();
        retry_queue.pop();
        DPRINTF(ClockedPermissionDebug, "Found the retry Issue! Port %lu\n", 
                                                                        id);
    }
}

void
ClockedPermission::recvRangeChange() {
    for (auto p : cpuSidePorts)
        p.sendRangeChange();
}

Port&
ClockedPermission::getPort(const std::string &if_name, PortID idx) {
    if (if_name == "mem_side_port") {
        return memSidePort;
    }
    else if (if_name == "cpu_side_ports" && idx < cpuSidePorts.size()) {
        return cpuSidePorts[idx];
    }
    else {
        return ClockedObject::getPort(if_name, idx);
    }
    assert(false && "unreachable code!\n");
}

void
ClockedPermission::startup() {
    // do nothing!
}

ClockedPermission::StatGroup::StatGroup(statistics::Group *parent)
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


// --------------------------- All MMP Methods are here -------------------- //

// make sure to implement the caching methods here to quickly copy paste them,
// if needed.
ClockedPermission::permission_handler
ClockedPermission::isCachedRequest(gem5::Addr addr) {
    /*
    Simple caching function that determines the caching variable from the
    class contructor and then makes sure to return where the given address
    has the values in the cache.
    
    @params
    addr: address to check inside the cache
    
    :returns:
        A struct with cache hit status and the latency value.
    */
    
    // figure out what kind of cache I am using.
    ++stats.numPermissionTableAccesses;

    // TODO: A cached request needs to fetch 64 bytes of data. Each entry in
    // the permission table is 2 bytes. So an aligned entry should have 32
    // cached entries!

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
    }
}

ClockedPermission::permission_handler
ClockedPermission::simpleLRU(gem5::Addr addr) {
    // ideally see if there is an entry (MMP). The address needs to ignore the
    // last 8 bits as each entry can have.
    gem5::Addr addr_key = addr | cache_mask;
    auto lookup = permission_table.find(addr_key);

    // create a return structure
    struct permission_handler return_struct;
    return_struct.is_cached = false;

    // TODO:
    // Permissions are per segment. So a binary search needs to be made to
    // figure out the exact delay of MMP. The simplest implementation is when
    // the segment size is the same as the page size (i.e. 4KiB)

    // There can be a variable latency added for this lookup in the cache of
    // MMP. the lateny of a hit is actually a variable latency. Since this
    // is a binary lookup, the latency is log2 N where N is the number
    // of entries. Make sure that the latency is never 0.
    Tick latency = permission_table_entries > 0 ? 
                                    std::log2(permission_table_entries) : 1;

    if (lookup != permission_table.end()) {
        // found the entry in the permission table. see if this is cached.
        DPRINTF(PermissionTable, "PLB hit for addr %#x\n", addr);
        if (lookup->second->is_cached == true) {
            // Hit latency must be very small!
            latency = hitLatency;
            // since this is LRU, increment the count by 1
            lookup->second->access_count++;
            ++stats.numPermissionTableCacheHits;
            return_struct.is_cached = true;
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
        permission_table.insert({addr_key, cve});

        // see if there is space in the cache for us to cache it.
        if (total_cached_entries < max_cached_entries) {
            permission_table[addr_key]->is_cached = true;
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
            permission_table[addr_key]->is_cached = true;
            permission_table[addr_key]->access_count = 1;
        }
    }

    // check if this address is in the cache
    return_struct.latency = latency;
    return return_struct;
    
}

ClockedPermission::permission_handler
ClockedPermission::simpleMRU(gem5::Addr addr) {
    // ideally see if there is an entry (MMP)
    auto lookup = permission_table.find(addr);
    // create a return structure
    struct permission_handler return_struct;
    return_struct.is_cached = false;

    // There can be a variable latency added for this lookup in the cache of
    // MMP.
    Tick latency = 0;

    if (lookup != permission_table.end()) {
        // found the entry in the permission table. see if this is cached.
        if (lookup->second->is_cached == true) {
            latency = hitLatency;
            lookup->second->last_accessed = gem5::curTick();
            return_struct.is_cached = true;
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
    return_struct.latency = latency;
    return return_struct;
}
ClockedPermission::permission_handler
ClockedPermission::simpleRandom(gem5::Addr addr) {
    // ideally see if  is an entry (MMP)
    auto lookup = permission_table.find(addr);
    // create a return structure
    struct permission_handler return_struct;
    return_struct.is_cached = false;

    // There can be a variable latency added for this lookup in the cache of
    // MMP.
    Tick latency = 0;

    if (lookup != permission_table.end()) {
        // found the entry in the permission table. see if this is cached.
        if (lookup->second->is_cached == true) {
            latency = hitLatency;
            return_struct.is_cached = true;
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
    return_struct.latency = latency;
    return return_struct;
}


} // namespace gem5
