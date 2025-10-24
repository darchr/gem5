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


// bool
// ClockedPermission::recvTimingReq(PacketPtr pkt, uint64_t packet_id) {
//     // If the permission tables are enabled by the user.
//     ++stats.numIncomingCPUSidePackets;
//     bool did_permission_failed = false;
//     if (pkt->getAddr() >= 4294967296 && pkt->getAddr() < 38654705664)
//         // 50% of the requests need to be here! why is this not getting printed?????
//         DPRINTF(RemoteAddress, "Remote address %#x\n", pkt->getAddr());
//     /*
//     if (enablePermissionCheck && useDedicatedCaching) {
//         // for debugging only
//         // make this throw an error!
//         assert(false && "disabled if-else condition\n");

//         // TODO:
//         // Change the return structure to a struct with <bool, gem5::Tick>.
//         // If this is a miss, then create an additional packet that accesses
//         // the memory to fetch the data from the permission table.
//         struct permission_handler status = isCachedRequest(pkt->getAddr());

//         // TODO:
//         // Create an additional dummy packet that handles a PLB miss. This
//         // only happens if there is a PLB miss
//         if (status.is_cached == false) {
//             // figure out where is the entry stored in the permission table.
//             // this is a linear table and the caching will depend on the
//             // structure of this table.
//             Addr permission_addr = getPLBAddr(pkt->getAddr());

//             // even
//             // if this is a linear table, the timing correctness is implemented
//             // as the lookup latency. this request is only made to make sure
//             // that the memory contention is correctly modeled.
//             // assume that this is a flat table where the address is the index.

//             // First create a new request
//             Request::Flags flags;
//             RequestPtr req = std::make_shared<Request>(
//                                 permission_addr, 1, pkt->req->getFlags(), pkt->requestorId());
//             PacketPtr permission_pkt = new Packet(req, permission_cmd);
//             permission_pkt->allocate();

//             DPRINTF(PermissionPackets,
//                 "Created custom packet with addr %#lu and req ID %d\n",
//                                 permission_pkt->getAddr(), pkt->requestorId());
//             // TODO: What do I do with this packet? Try sending this packet?
//             if (memSidePort.sendTimingReq(permission_pkt)) {
//                 // what is packet_id
//                 portMap[pkt->id] = packet_id;
//             }
//             // TODO: can this packet go into the same retry queue?
//             else {
//                 DPRINTF(PermissionPackets, "Couldn't send %#lu on port %lu\n",
//                                         permission_pkt->getAddr(), packet_id);

//                 retry_queue.push(packet_id);
//             }
//         }


//         // TODO:
//         // Schedule a new AccessEvent with this latency. Since this is a clock
//         // edge, both the rising and the falling edges can be used in this
//         // case. Maybe cite the dual edged flip flop if needed in the paper.
//         schedule(new EventFunctionWrapper([this, pkt]{ },
//                         name() + ".accessEvent", true),
//                         clockEdge(static_cast<Cycles>(status.latency / 2)));

//     }
//     else {
//         // if this is a retry request then skip this. why?
//         // if (retry_queue.empty()) {
//         // for every request that goes to the remote memory, check for permissions.
//             // if (retry_queue.empty() && pkt->getAddr() >= 0x100000000 &&
//             //             pkt->getAddr() < 0x800000000) {

//             // make sure to print the address if this is a remote memory request
//             if (pkt->getAddr() >= 4294967296 && pkt->getAddr() < 38654705664)
//                 // 50% of the requests need to be here! why is this not getting printed?????
//                 DPRINTF(RemoteAddress, "Remote address %#x\n", pkt->getAddr());

//             // if this is a remote memory request, then check for permissions.
//             // if (pkt->getAddr() >= 0x100000000 &&
//             //             pkt->getAddr() < 0x800000000) {
//             // This is the beginning of space control with no dedicated caching
//             // first for every memory request in the shared memory region, create
//             // another memory request to enforce permission checks. otherwise the
//             // OS is writing permissions to the permission section
//             if (pkt->getAddr() >= baseAddrPermissionTable &&
//                         pkt->getAddr() < baseAddrPermissionTable + 0x40000000) {
//                 // This is an OS request to read or write into the permission table
//                 // don't do anything actually!
//                 DPRINTF(PermissionPackets, "Writing to the permission table at %#x\n", pkt->getAddr());
//                 // disable this
//                 assert(false && "artifacts aren't created yet for this kind of testing!\n");
//             }
//             else {
//                 // lookup the entry. the lookup time is dependent up on the number
//                 // of permission entries.
//                 // XXX: The number of entries is preset.

//                 // regardless of a dedicated cache is present, the time required to
//                 // lookup an entry will always be constant.
//                 Tick lookup_time = 0;
//                 if (binarySearch)
//                     lookup_time = (gem5::Tick) log2(numberOfEntries);
//                 else
//                     lookup_time = numberOfEntries;

//                 // assume system caching and schedule a number of fake requests to
//                 // the dedicated memory region. The packets must be
//                 int memory_packes_required = (permissionEntrySize / 64);

//                 for (int i = 0 ; i < memory_packes_required ; i++) {
//                     ++stats.numOutgoingMemSidePackets;

//                     // even
//                     // if this is a linear table, the timing correctness is implemented
//                     // as the lookup latency. this request is only made to make sure
//                     // that the memory contention is correctly modeled.
//                     // assume that this is a flat table where the address is the index.

//                     // First create a new request
//                     // FIXME: There needs to be n number of read requests by 64.
//                     Addr permission_addr = baseAddrPermissionTable + i * 64;
//                     // Request::Flags flags;
//                     RequestPtr req = std::make_shared<Request>(
//                                         permission_addr, 1, pkt->req->getFlags(), pkt->requestorId());
//                     PacketPtr permission_pkt = new Packet(req, permission_cmd, 64);
//                     // req->setFlags(Request::VALID_SIZE);
//                     // permission_pkt->setSize(permissionEntrySize);

//                     // system caches should not be used for permission packets.
//                     // permission_pkt->req->setFlags(Request::UNCACHEABLE);

//                     permission_pkt->allocate();

//                     DPRINTF(PermissionPackets,
//                         "Created custom packet for pkt addr %#x with permission addr"
//                         " %#x and size %lu and req ID %d and packet_id %d\n",
//                                         pkt->getAddr(), permission_pkt->getAddr(), permission_pkt->getSize(),
//                                         pkt->requestorId(), packet_id);
//                     // TODO: What do I do with this packet? Try sending this packet?
//                     if (memSidePort.sendTimingReq(permission_pkt)) {
//                         // what is packet_id
//                         ++stats.numPermissionTableAccesses;
                        
//                         // this should not happen
//                         // portMap[permission_pkt->id] = packet_id;

//                         // this is the additonal latency required to do the lookup.
//                         schedule(new EventFunctionWrapper([this, permission_pkt]{ },
//                             name() + ".accessEvent", true),
//                             clockEdge(static_cast<Cycles>(lookup_time)));

//                     }
//                     // TODO: can this packet go into the same retry queue?
//                     else {
//                         DPRINTF(PermissionPackets, "Couldn't send %#x for %#x on port %lu\n",
//                                                 permission_pkt->getAddr(), pkt->getAddr(), packet_id);
//                         // only store the actual request and delete the fake request.
//                         // it'll be created again.
//                         //
//                         // do not push the permission packet into the retry
//                         // queue
//                         // retry_queue.push(packet_id);
//                         did_permission_failed = true;
//                         // delete the traffic packet
//                         delete permission_pkt;
//                         // delete req;

//                         // also cannot send the actual packet now as the permission packet will always go first.
//                         return false;
//                     }
//                 }
//                 // for (int i = 0 ; i < memory_packes_required ; i++) {
//                     // simulate this memory request. is this required?
//                     // ++stats.numOutgoingTrafficPackets;
//                     // FIXME: The lookup happens once but the number of memory packets are multiple

//                 // }
//             }

//         }
//         // else {
//             // this packet does to local memory
//         // }
//     }

//     // if (!did_permission_failed) {
//     */
//     // business as usual. if the permission packet is not sent, then this part of the code will never reach
//     if (memSidePort.sendTimingReq(pkt)) {
//         // Send successful, keep the packet_id for later.s
//         portMap[pkt->id] = packet_id;
//         // if (did_permission_failed) 
//         //     assert(false && "it doesn't make sense that the permission packet failed!\n");
//         return true;
//         // }
//     }

//     // cannot send this packet now.
//     DPRINTF(ClockedPermissionDebug, "Failed to send %#x on port %lu\n",
//                                                     pkt->getAddr(), packet_id);
//     // if (!(pkt->getAddr() == baseAddrPermissionTable) && !did_permission_failed)
//     retry_queue.push(packet_id);
//     return false;
// }

bool
ClockedPermission::sendPermissionPackets(PacketPtr pkt) {
    // this is a helper function that sends permission packets to the memsideport
    return true;
}

bool
ClockedPermission::recvTimingReq(PacketPtr pkt, uint64_t packet_id) {
    // If the permission tables are enabled by the user.
    ++stats.numIncomingCPUSidePackets;
    // keep different states to track the packet
    int did_permission_failed = 0x0;
    if (pkt->getAddr() >= 4294967296 && pkt->getAddr() < 38654705664)
        // 50% of the requests need to be here! why is this not getting printed?????
        DPRINTF(RemoteAddress, "Remote address %#x\n", pkt->getAddr());
    
    // if permission checks are not enabled, then this simobject doesn't do
    // anything
    if (enablePermissionCheck) {
        // permission cache is not implemented yet
        if (useDedicatedCaching) {
            assert(false && "Caching is not implemented yet\n");
        }
        else {
            // simple uncached version.
            // TODO: enable system-level permission

            // permission checks only happen for remote memory addresses!
            // TODO: Fix hardcoding issues
            if (pkt->getAddr() >= 4294967296 && pkt->getAddr() < 38654705664 && permission_checker[pkt->getAddr()] == false) {
                // this request can be either in the data section or the table
                if (pkt->getAddr() >= baseAddrPermissionTable &&
                pkt->getAddr() < baseAddrPermissionTable + 0x40000000) {
                    // This is an OS request to read or write into the
                    // permission table don't do anything and let this request
                    // pass.
                    if (pkt->isRead()) {
                        DPRINTF(PermissionPackets,
                                "Reading from the permission table at %#x\n",
                                pkt->getAddr());
                    }
                    else {
                        DPRINTF(PermissionPackets,
                                "Writing into the permission table at %#x\n",
                                pkt->getAddr());
                    }
                    // disable this
                    assert(false &&
                        "artifacts aren't created yet for this kind of "
                        " testing!\n");

                }
                else {
                    // there needs to be a permission lookup.
                    // regardless of a dedicated cache is present, the time
                    //  required to lookup an entry will always be constant.
                    Tick lookup_time = 0;
                    if (binarySearch)
                        lookup_time = (gem5::Tick) log2(numberOfEntries);
                    else
                        lookup_time = numberOfEntries;

                    // assume system caching and schedule a number of fake 
                    // requests to the dedicated memory region. The packets 
                    // must be of 64 bytes.
                    int memory_packes_required = (permissionEntrySize / 64);

                    for (int i = 0 ; i < memory_packes_required ; i++) {
                        // out of all the remote memory requests, these many
                        // are going to read the permission table!
                        ++stats.numOutgoingMemSidePackets;

                        // even
                        // if this is a linear table, the timing correctness is implemented
                        // as the lookup latency. this request is only made to make sure
                        // that the memory contention is correctly modeled.
                        // assume that this is a flat table where the address is the index.

                        // First create a new request
                        // FIXME: There needs to be n number of read requests by 64.
                        Addr permission_addr = baseAddrPermissionTable + i * 64;
                        // Request::Flags flags;
                        RequestPtr req = std::make_shared<Request>(
                                            permission_addr,
                                            1,
                                            pkt->req->getFlags(),
                                            pkt->requestorId());
                        PacketPtr permission_pkt = new Packet(req,
                                                             permission_cmd,
                                                            64);
                        // req->setFlags(Request::VALID_SIZE);
                        // permission_pkt->setSize(permissionEntrySize);

                        // system caches should not be used for permission packets.
                        permission_pkt->req->setFlags(Request::UNCACHEABLE);

                        permission_pkt->allocate();

                        DPRINTF(PermissionPackets,
                            "Created custom packet for pkt addr %#x with "
                            "permission addr %#x and size %lu and req ID %d "
                            "and packet_id %d\n", pkt->getAddr(),
                                                    permission_pkt->getAddr(),
                                                    permission_pkt->getSize(),
                                                    pkt->requestorId(),
                                                    packet_id);
                        
                        if (memSidePort.sendTimingReq(permission_pkt)) {
                            // what is packet_id
                            ++stats.numPermissionTableAccesses;
                            // the permission packet went through, now see if
                            // the real packet can go through in the same tick
                            did_permission_failed = 0x2;
                            
                            // this is an infinite cache rn.
                            permission_checker[pkt->getAddr()] = true;
                            
                            // this should not happen
                            portMap[permission_pkt->id] = packet_id;

                            // this is the additonal latency required to do the lookup.
                            schedule(new EventFunctionWrapper([this, permission_pkt]{ },
                                name() + ".accessEvent", true),
                                clockEdge(static_cast<Cycles>(lookup_time)));

                        }
                        // TODO: can this packet go into the same retry queue?
                        else {
                            DPRINTF(PermissionPackets, "Couldn't send %#x for %#x on port %lu\n",
                                                    permission_pkt->getAddr(), pkt->getAddr(), packet_id);
                            // only store the actual request and delete the fake request.
                            // it'll be created again.
                            //
                            // do not push the permission packet into the retry
                            // queue
                            // retry_queue.push(packet_id);
                            did_permission_failed = 0x1;
                            // delete the traffic packet
                            delete permission_pkt;
                            // delete req;

                            // also cannot send the actual packet now as the permission packet will always go first.
                            // return false;
                        }

                        // just schedule this event and delete the packet for
                        // now.
                        // this is the additonal latency required to do the lookup.
                        // schedule(new EventFunctionWrapper([this, permission_pkt]{ },
                        //     name() + ".accessEvent", true),
                        //     clockEdge(static_cast<Cycles>(lookup_time)));
                        
                        // delete permission_pkt;

                        // keep packet map
                        // permission_tracker[pkt->getAddr()] = 

                        // if 
                        
                        /*
                                This logic is incorrect
                                plan
                                send the permission packet and queue the actual
                                packet
                                when retried, look if the corresponding
                                permission packet is sent already.
                                if yes, then send the actual packet.

                        // TODO: What do I do with this packet? Try sending this packet?
                        if (memSidePort.sendTimingReq(permission_pkt)) {
                            // what is packet_id
                            ++stats.numPermissionTableAccesses;
                            
                            // this should not happen
                            // portMap[permission_pkt->id] = packet_id;

                            // this is the additonal latency required to do the lookup.
                            schedule(new EventFunctionWrapper([this, permission_pkt]{ },
                                name() + ".accessEvent", true),
                                clockEdge(static_cast<Cycles>(lookup_time)));

                        }
                        // TODO: can this packet go into the same retry queue?
                        else {
                            DPRINTF(PermissionPackets, "Couldn't send %#x for %#x on port %lu\n",
                                                    permission_pkt->getAddr(), pkt->getAddr(), packet_id);
                            // only store the actual request and delete the fake request.
                            // it'll be created again.
                            //
                            // do not push the permission packet into the retry
                            // queue
                            // retry_queue.push(packet_id);
                            did_permission_failed = true;
                            // delete the traffic packet
                            delete permission_pkt;
                            // delete req;

                            // also cannot send the actual packet now as the permission packet will always go first.
                            return false;
                        }
                        */
                    }

                }

            }

        }
    }

    // business as usual. if the permission packet is not sent, then this part
    // of the code will never reach.
    if (memSidePort.sendTimingReq(pkt)) {
        // Send successful, keep the packet_id for later.s
        portMap[pkt->id] = packet_id;

        if (did_permission_failed == 0x2) {
            // a permission packet was sent and so was the actual packet
            DPRINTF(PermissionPackets, "Both permission and actual packet sent for %#x\n",
                                                    pkt->getAddr());
        }
        else if (did_permission_failed == 0x1) {
            // permission packet failed to send, so did the actual packet
            DPRINTF(PermissionPackets, "Both permission and actual packet failed for %#x\n",
                                                    pkt->getAddr());

            assert(false && "it doesn't make sense that the permission packet failed!\n");
        }
        // else {
        //     // no permission packet was sent, only the actual packet was sent
        //     DPRINTF(PermissionPackets, "Only actual packet sent for %#x\n",
        //                                             pkt->getAddr());
        // }
        // if (did_permission_failed) 
        return true;
        // }
    }

    // cannot send this packet now.
    if (did_permission_failed == 0x2) {
        // a permission packet was sent and so was the actual packet
        DPRINTF(PermissionPackets, "Permission packet for %#x was sent but the actual packet failed!\n",
                                                pkt->getAddr());
    }
    else if (did_permission_failed == 0x1) {
        // permission packet failed to send, so did the actual packet
        DPRINTF(PermissionPackets, "Both permission and actual packet failed for %#x\n",
                                                pkt->getAddr());

        assert(false && "it doesn't make sense that the permission packet failed!\n");
    }

    DPRINTF(ClockedPermissionDebug, "Failed to send %#x on port %lu\n",
                                                    pkt->getAddr(), packet_id);
    // if (!(pkt->getAddr() == baseAddrPermissionTable) && !did_permission_failed)
    retry_queue.push(packet_id);
    return false;
}

Addr
ClockedPermission::getPLBAddr(Addr addr) {
    // return the base of the permission table address + the entry.
    // XXX: This is unimplemented with the ID
    return baseAddrPermissionTable;
}

void
ClockedPermission::recvRespRetry(const PortID id) {
    memSidePort.sendRetryResp();
    DPRINTF(ClockedPermissionDebug, "Found the issue! Retry called for %lu\n",
                                                                        id);
}

bool
ClockedPermission::recvTimingResp(PacketPtr pkt) {
    // TODO: delete the packet if this is a permission packet
    if (pkt->getAddr() >= baseAddrPermissionTable && pkt->getAddr() < baseAddrPermissionTable + 0x40000000) {
            DPRINTF(PermissionPackets, "Got response for permission pkt %#x and size %d\n",
                                                            pkt->getAddr(), pkt->getSize());
        // do not send this packet to the CPU side ports as the job of the
        // SimObject is over.
        delete pkt;
        return true;
    }
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
