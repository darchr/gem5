 #include "new/clocked_permission.hh"

#include "base/trace.hh"

#include "debug/PermissionTable.hh"
#include "debug/PermissionTableEvent.hh"
#include "debug/ClockedPermissionDebug.hh"
#include "debug/PermissionPackets.hh"
#include "debug/RemoteAddress.hh"

namespace gem5 {

ClockedPermission::ClockedPermission(const ClockedPermissionParams &params) :
    ClockedObject(params),
    memSidePort(params.name + ".mem_side_port", *this),
    // cpuSidePort(params.name + ".cpu_side_port", this),
    enablePermissionCheck(params.enable_permission_check),
    useDedicatedCaching(params.use_dedicated_caching),
    baseAddrPermissionTable(params.permission_base_addr),
    numberOfEntries(params.number_of_entries),
    binarySearch(params.binary_search),
    permissionEntrySize(params.permission_entry_size),
    creationLatency(params.creation_latency),
    hitLatency(params.hit_latency),
    missLatency(params.miss_latency),
    totalMemorySize(params.total_memory_size),
    cacheSize(params.cache_size),
    segmentSize(params.segment_size),
    cachePolicy(params.cache_policy),
    // event([this] {
    //       // We’re about to notify the CPU master to retry.
    //       waitingForCpuRetry = false;
    //       DPRINTF(PermissionPackets, "sendRetryReq() to CPU side\n");
    //       recvReqRetry();
    //   }, name() + ".cpuRetryEvent"),
    event([this]{processEvent();}, name()),
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
    // All plb packets will be of size 64 bytes.
    permission_block_size = 64;

    // FIXME:
    // All plb packets will be of READ type.
    permission_cmd = MemCmd::ReadReq;

    // extending the PLB model, we need to implement the number of lookups 
    // needed to fetch an entry from the permission table via binary
    // search
    if (binarySearch) {
        // it must never be zero
        max_search_attempts = ceil(log2(total_entries)) + 1;
        DPRINTF(PermissionTable, "MMP binary search will take %d attempts\n",
                                                max_search_attempts);
        assert(max_search_attempts == ceil(log2(numberOfEntries) + 1));
    }
    else {
        // this is a linear search
        assert(numberOfEntries <= 0);
        max_search_attempts = numberOfEntries;
    }
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

// void
// ClockedPermission::init() {
//     requestorId = -1;
// }


// Schedule the retry one cycle later (avoid re-entrancy)
// void
// ClockedPermission::scheduleCpuRetry()
// {
//     DPRINTF(PermissionPackets, "scheduleCpuRetry called\n");

//     if (!event.scheduled() && !waitingForCpuRetry) {
//         waitingForCpuRetry = true;
//         // Use next clock edge to be well-behaved wrt timing
//         DPRINTF(PermissionTable, "Scheduled an event to retry sending pkt\n");

//         schedule(event, clockEdge(Cycles(1)));
//     }
// }

// make sure the event is correctly set for this simobject
void
ClockedPermission::processEvent() {
    // This is only called if the user wants to add permission checks
    // make sure that this address is scheduled with some additional latency.
    // DPRINTF(PermissionTable, "Scheduling addr %#x with %lu latency\n",
    //                                     pkt->getAddr(), class_latency);
    // the class latency must be set before the event can be called.
    DPRINTF(PermissionPackets, "kg says hi. "
                    "number of jobs left: %lu\n", permission_packets.size());

    // this event must be called in the future.

    // if i have packets to send, I'll send them.
    if (!permission_packets.empty() && !waitingForMemRetry) {
        PacketPtr pkt = permission_packets.front();
        if (memSidePort.sendTimingReq(pkt)) {// }, portMap[pkt->id])) {
            // Increment the count of permission packets sent for this address.
            permission_request_tracker[pkt->getAddr()]++;
            DPRINTF(PermissionPackets, "Sent permission pkt %#x on port %lu!\n",
                                            pkt->getAddr(), portMap[pkt->id]);
            // see if i need another event for the next packet? or did i sche
            // dule another event when i received the packet?
            if (!permission_packets.empty() && !event.scheduled()) {
                schedule(event, clockEdge(Cycles(1)));
            }
            // portMap[pkt->id] = packet_id;
            // return true;
        }
        else {
            DPRINTF(PermissionPackets, "Failed to sent permission pkt %#x on port %lu!\n",
                                            pkt->getAddr(), portMap[pkt->id]);
            // Since we couldn't accept the CPU's req (because we can't forward it),
            // we also need to block the CPU and only call cpuSidePort.sendRetryReq()
            // once we have successfully forwarded (or have buffer space).
            // For a simple "keep asking" demo, you could still schedule another CPU retry:
            // scheduleCpuRetry();
            failedPermissionPackets.push(pkt);
            waitingForMemRetry = true;
            if (!event.scheduled())
                schedule(event, clockEdge(Cycles(1)));

            // retry_queue.push(packet_id);
            // return false;
        }
        permission_packets.pop();
    }
}

bool
ClockedPermission::recvTimingReq(PacketPtr pkt, uint64_t port_id) {
    if (!enablePermissionCheck) {
        // if the permission check is not enabled, just forward the packet.
        if (memSidePort.sendTimingReq(pkt)) {
            DPRINTF(PermissionPackets, "Sent %#x on port %lu!\n",
                                                pkt->getAddr(), port_id);
            portMap[pkt->id] = port_id;
            return true;
        }
        else {
            DPRINTF(PermissionPackets, "Failed to sent %#x on port %lu!\n",
                                                pkt->getAddr(), port_id);
            // the memsideport will call recvRespRetry when it is ready.

            retry_queue.push(port_id);
            return false;
        }
    }
    else {
        // our code!
        stats.numIncomingCPUSidePackets++;
        if (have_i_seen_this[pkt->getAddr()] == false) {
            // this is the first time I have seen this address.
            have_i_seen_this[pkt->getAddr()] = true;
            permission_request_tracker[pkt->getAddr()] = 0;
            for (int i = 0 ; i < max_search_attempts; i++) {
                // create fake permission packets//
                Addr permission_addr = baseAddrPermissionTable;
                // getBinarySearchPermissionTableAddr(
                // permission_checker[pkt->getAddr()]);
                // Request::Flags flags;
                RequestPtr req = std::make_shared<Request>(
                                    permission_addr,
                                    1,
                                    pkt->req->getFlags(),
                                    pkt->requestorId());
                PacketPtr permission_pkt = new Packet(req,
                                                permission_cmd,
                                                permissionEntrySize);
                // req->setFlags(Request::VALID_SIZE);
                // permission_pkt->setSize(permissionEntrySize);

                // system caches should not be used for permission packets.
                // permission_pkt->req->setFlags(Request::UNCACHEABLE);

                permission_pkt->allocate();
                permission_packets.push(permission_pkt);
            }

            DPRINTF(PermissionPackets, "First time seeing addr %#x. "
                            "Will send %d permission packets\n",
                            pkt->getAddr(), max_search_attempts);
            // scheduleCpuRetry();
        }
        // do i have permission packets on the queue?
        if (!permission_packets.empty() && !event.scheduled()) {
            schedule(event, clockEdge(Cycles(1)));
        }

        if (memSidePort.sendTimingReq(pkt)) {
            DPRINTF(ClockedPermissionDebug, "Sent %#x on port %lu!\n",
                                                pkt->getAddr(), port_id);
            portMap[pkt->id] = port_id;
            return true;
        }
        else {
            DPRINTF(ClockedPermissionDebug, "Failed to sent %#x on port %lu!\n",
                                                pkt->getAddr(), port_id);
            // the memsideport will call recvRespRetry when it is ready.
            waitingForMemRetry = true;

            retry_queue.push(port_id);
            return false;
        }

    }
}


void
ClockedPermission::recvReqRetry() {
    // see if i have to queue events from the past.
    waitingForMemRetry = false;
    DPRINTF(PermissionPackets, "recvReqRetry is called with size %lu\n", retry_queue.size());
    // do i have failed permission packets?
    while (!failedPermissionPackets.empty()) {
        // PacketPtr pkt = failedPermissionPackets.front();
        // if (memSidePort.sendTimingReq(pkt)) {// }, portMap[pkt->id])) {
        //     // Increment the count of permission packets sent for this address.
        //     permission_request_tracker[pkt->getAddr()]++;
        //     DPRINTF(PermissionPackets, "Sent failed permission pkt %#x on port %lu!\n",
        //                                     pkt->getAddr(), portMap[pkt->id]);
            failedPermissionPackets.pop();
        // }
        // else {
        //     DPRINTF(PermissionPackets, "Failed to sent permission pkt %#x on port %lu!\n",
        //                                     pkt->getAddr(), portMap[pkt->id]);
        //     break;
        // }
    }
    
    while (!retry_queue.empty()) {

        uint64_t id = retry_queue.front();
        cpuSidePorts[id].sendRetryReq();
        retry_queue.pop();
        // retry_queue.unset(id);
        DPRINTF(ClockedPermissionDebug, "Found the retry Issue! Port %lu\n",
                                                                        id);
        // DPRINTF(PermissionPackets, "There is content inside is called  %lu\n",
        //                                                                 id);
    }
}

void
ClockedPermission::scheduleNewEvent() {
    schedule(event, curTick() + 1);
}

Addr
ClockedPermission::getPLBAddr(Addr addr) {
    // return the base of the permission table address + the entry.
    // XXX: This is unimplemented with the ID
    return baseAddrPermissionTable;
}


Addr
ClockedPermission::getBinarySearchPermissionTableAddr(int attempt) {
    // return the address of the permission table by binary search without the
    // actual map. uncached entries will work perfectly.
    // XXX: Assumption: the permission table is always 1 GiB with
    // 2 less entries
    Addr table_start = 0x80;        // 128 bytes
    Addr table_end = 0x40000000;    // 1 GiB

    // since we have a table size of 32 GiB, we need ~0.49 GiB of permission
    // table.
    table_end = 0x1FFFFF80; 

    // hardcoding this because i am lazy

    Addr addr = baseAddrPermissionTable + ((table_end - table_start) / (attempt + 1));

    // must always be cache-line sized 
    assert(addr % 0x40 == 0 && (addr >= baseAddrPermissionTable &&
            addr < baseAddrPermissionTable + 0x40000000));

    // XXX: This is unimplemented with the ID
    return addr;
}

void
ClockedPermission::recvRespRetry(const PortID id) {
    memSidePort.sendRetryResp();
    DPRINTF(ClockedPermissionDebug, "recvRespRetry Found the issue! Retry called for %lu\n",
                                                                        id);
}

bool
ClockedPermission::recvTimingResp(PacketPtr pkt) {
    // TODO
    // Let's create the full solution. here is the plan

    // TODO: delete the packet if this is a permission packet
    if (pkt->getAddr() >= baseAddrPermissionTable && pkt->getAddr() < baseAddrPermissionTable + 0x40000000) {
            DPRINTF(PermissionPackets, "Got response for permission pkt %#x and size %d\n",
                                                            pkt->getAddr(), pkt->getSize());

        // this is the additonal latency required compare the permission address
        // address to the actual memory packet address.
        // TODO: Add a new parameter for comparison purposes.
        Tick comparison_latency = 1;
        schedule(new EventFunctionWrapper([this, pkt]{ },
            name() + ".accessEvent", true),
            clockEdge(static_cast<Cycles>(comparison_latency)));
        
        // do not send this packet to the CPU side ports as the job of the
        // SimObject is over. if you do, then popSenderState will fail!
        if (permission_request_tracker[pkt->getAddr()] < 23)
            warn("Permission table response received before all permission packets were sent for addr %#x!\n", pkt->getAddr());
        delete pkt;
        return true;
    }
    PacketId id = pkt->id;
    return cpuSidePorts[portMap[id]].sendTimingResp(pkt);
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
    schedule(event, 1);
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
