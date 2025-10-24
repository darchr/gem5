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
    event([this] {
          // We’re about to notify the CPU master to retry.
          waitingForCpuRetry = false;
          DPRINTF(PermissionPackets, "sendRetryReq() to CPU side\n");
          recvReqRetry();
      }, name() + ".cpuRetryEvent"),
    // event([this]{processEvent();}, name()),
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


    // // if (have_i_sent_this[pkt->getAddr()] == true) {
    //     // this address has been sent before. so, we can just forward it.
    //     if (memSidePort.sendTimingReq(pkt)) {
    //         DPRINTF(PermissionPackets, "Sent %#x on port %lu!\n",
    //                                             pkt->getAddr(), packet_id);
    //         portMap[pkt->id] = packet_id;
    //         return true;
    //     }
    //     else {
    //         DPRINTF(PermissionPackets, "Failed to sent %#x on port %lu!\n",
    //                                             pkt->getAddr(), packet_id);
    //         // Since we couldn't accept the CPU's req (because we can't forward it),
    //         // we also need to block the CPU and only call cpuSidePort.sendRetryReq()
    //         // once we have successfully forwarded (or have buffer space).
    //         // For a simple "keep asking" demo, you could still schedule another CPU retry:
    //         scheduleCpuRetry();

    //         retry_queue.push(packet_id);
    //         return false;
    //     // }
    // }


// Schedule the retry one cycle later (avoid re-entrancy)
void
ClockedPermission::scheduleCpuRetry()
{
    DPRINTF(PermissionPackets, "scheduleCpuRetry called\n");

    if (!event.scheduled() && !waitingForCpuRetry) {
        waitingForCpuRetry = true;
        // Use next clock edge to be well-behaved wrt timing
        DPRINTF(PermissionTable, "Scheduled an event to retry sending pkt\n");

        schedule(event, clockEdge(Cycles(1)));
    }
}


// simple logic to understand what's going on. logic: see if returning false
// everytime forces this simobject to call retry request.
// bool
// ClockedPermission::recvTimingReq(PacketPtr pkt, uint64_t packet_id) {
//     // block a packet 23 times before sending it through this port
//     if (permission_packet_tracker[pkt->getAddr()] == false) {
//         permission_packet_tracker[pkt->getAddr()] = true;
//         permission_checker[pkt->getAddr()] = 0;
//         have_i_sent_this[pkt->getAddr()] = false;
//     }

//     if (permission_checker[pkt->getAddr()] < 23) {
//         // retry_queue.unset();
//         permission_checker[pkt->getAddr()]++;
//         DPRINTF(PermissionPackets, "addr %#x attempt %d on port %lu!\n",
//                 pkt->getAddr(), permission_checker[pkt->getAddr()], packet_id);
//         // Tell the upstream CPU to retry again on the next cycle.
//         // This is the key line you were missing:
//         // when the packet fails once in this cycle, you need to reset the
//         // retry bit so that the CPU does not send the packet again on the
//         // same event
//         scheduleCpuRetry();
//         DPRINTF(PermissionPackets, "retry event %d\n", waitingForCpuRetry);

//         DPRINTF(PermissionPackets, "queue size before %lu!\n", retry_queue.size());
//         retry_queue.push(packet_id);
//         DPRINTF(PermissionPackets, "queue size after %lu!\n", retry_queue.size());
//         // recvRespRetry(packet_id);
//         return false;
//     }
//     else {
//         // now the retry is free!
//         // retry_queue.set();
//         if (memSidePort.sendTimingReq(pkt)) {
//             // successful
//             permission_checker[pkt->getAddr()] = 0;
//             // also remove the retry request from the queue. force no retries
//             // as the packet has been sent.
//             // if (have_i_sent_this[pkt->getAddr()] == false) {
//                 retry_queue.set(packet_id);
//             // }
//             // have_i_sent_this[pkt->getAddr()] = true;
//             // }
//             DPRINTF(PermissionPackets, "Finally sent %#x on port %lu!\n",
//                                                 pkt->getAddr(), packet_id);
//             portMap[pkt->id] = packet_id;
//             return true;
//         }
//         DPRINTF(PermissionPackets, "Failed to sent %#x on port %lu!\n",
//                                             pkt->getAddr(), packet_id);
//         // Since we couldn't accept the CPU's req (because we can't forward it),
//         // we also need to block the CPU and only call cpuSidePort.sendRetryReq()
//         // once we have successfully forwarded (or have buffer space).
//         // For a simple "keep asking" demo, you could still schedule another CPU retry:
//         scheduleCpuRetry();

//         retry_queue.push(packet_id);
//         return false;
//     }
//     assert(false && "unreachable code\n");
//     return false;
    
// }

// bool
// ClockedPermission::recvTimingReq(PacketPtr pkt, uint64_t packet_id) {
//     // If the permission tables are enabled by the user.
//     ++stats.numIncomingCPUSidePackets;

//     // the number of attempts is always constant and it must be a python side
//     // parameters.

//     // logic: there needs to be lg(max_entries) number of memory packets that
//     // model the permission table lookup.
//     // for a given memory pkt, until the total number of permission references
//     // are made, gem5 needs to keep retrying the packet.

//     // see if this is a remote memory address
//     bool is_remote = 
//         (pkt->getAddr() >= 4294967296 && pkt->getAddr() < 38654705664) ? true :
//                                                                         false;

//     // TODO: Marked for deletion: this works
//     if (is_remote) {
//         // 50% of the requests need to be here when there is one lookup
//         DPRINTF(RemoteAddress, "Remote address %#x\n", pkt->getAddr());
//         // cerate an entry in the tracker
//         if (permission_packet_tracker[pkt->getAddr()] == false) {
//             // seen this address for the first time. start tracking the attempt
//             permission_checker[pkt->getAddr()] = 0;
//         }
//     }
    
//     // if permission checks are not enabled, then this simobject doesn't do
//     // anything
//     if (enablePermissionCheck) {
//         // permission cache is not implemented yet
//         if (useDedicatedCaching) {
//             assert(false && "Caching is not implemented yet\n");
//         }
//         else {
//             // simple uncached version.
//             // TODO: enable system-level permission

//             // permission checks only happen for remote memory addresses!
//             // TODO: Fix hardcoding issues
//             if (is_remote) { // } && permission_checker[pkt->getAddr()] <= max_binary_search_attempts) {
//                 // out of all the remote memory requests, these many
//                 // are going to read the permission table!

//                 // FIXME: this doesn't belong here.
//                 ++stats.numOutgoingMemSidePackets;

//                 // this request can be either in the data section or the table
//                 if (pkt->getAddr() >= baseAddrPermissionTable &&
//                         pkt->getAddr() < baseAddrPermissionTable + 0x40000000) {
//                     // This is an OS request to read or write into the
//                     // permission table don't do anything and let this request
//                     // pass.
//                     if (pkt->isRead()) {
//                         DPRINTF(PermissionPackets,
//                                 "Reading from the permission table at %#x\n",
//                                 pkt->getAddr());
//                     }
//                     else {
//                         DPRINTF(PermissionPackets,
//                                 "Writing into the permission table at %#x\n",
//                                 pkt->getAddr());
//                     }
//                     // disable this
//                     assert(false &&
//                         "artifacts aren't created yet for this kind of "
//                         " testing!\n");

//                 }
//                 else {
//                     // make sure that this block is called only if the number
//                     // of attemps is not the maximum lookups
//                     if (permission_checker[pkt->getAddr()] < max_search_attempts) {

//                         // retry_queue.unset();
//                         // there needs to be a permission lookup.
//                         // regardless of a dedicated cache is present, the time
//                         // required to lookup an entry will always be
//                         // constant.

//                         // there needs to be a permission packet creation
//                         // latency at this point.
//                         Tick permission_packet_creation_latency = 1;
//                         // if (binarySearch)
//                         //     lookup_time = (gem5::Tick) log2(numberOfEntries);
//                         // else
//                         //     lookup_time = numberOfEntries;

//                         // we create a permission packet to read the permission
//                         Addr permission_addr = baseAddrPermissionTable;
//                                         // getBinarySearchPermissionTableAddr(
//                                         // permission_checker[pkt->getAddr()]);
//                         // Request::Flags flags;
//                         RequestPtr req = std::make_shared<Request>(
//                                             permission_addr,
//                                             1,
//                                             pkt->req->getFlags(),
//                                             pkt->requestorId());
//                         PacketPtr permission_pkt = new Packet(req,
//                                                         permission_cmd,
//                                                         permissionEntrySize);
//                         // req->setFlags(Request::VALID_SIZE);
//                         // permission_pkt->setSize(permissionEntrySize);

//                         // system caches should not be used for permission packets.
//                         // permission_pkt->req->setFlags(Request::UNCACHEABLE);

//                         permission_pkt->allocate();
//                         // make sure that the permission packet creation
//                         // latency is modeled correctly.
                        // schedule(new EventFunctionWrapper(
                        //                             [this, permission_pkt]{ },
                        //                             name() + ".accessEvent",
                        //                             true),
                        //     clockEdge(static_cast<Cycles>(
                        //                 permission_packet_creation_latency)));
                        

//                         // make sure that the permission entry size is 64 bytes.
//                         // cannot send a higher packet to the memory
//                         assert(permissionEntrySize == 64);

//                         DPRINTF(PermissionPackets,
//                             "Created custom packet for pkt addr %#x with "
//                             "permission addr %#x and size %lu and req ID %d "
//                             "and packet_id %d attempt %d\n",
//                                         pkt->getAddr(),
//                                         permission_pkt->getAddr(),
//                                         permission_pkt->getSize(),
//                                         pkt->requestorId(),
//                                         packet_id,
//                                         permission_checker[pkt->getAddr()]);
                            
//                         if (memSidePort.sendTimingReq(permission_pkt)) {
//                             // what is packet_id
//                             ++stats.numPermissionTableAccesses;
                            
//                             // this is an infinite cache rn.
//                             permission_checker[pkt->getAddr()]++;

//                             // // the permission packet went through, now see if
//                             // // the real packet can go through in the same tick
//                             // if (permission_checker[pkt->getAddr()] == 23)
//                             //     did_permission_failed = 0x3;

//                             // // if a permission packet is sent, keep a track of
//                             // // the status using the did_permission_failed
//                             // // number.
//                             // //
//                             // // 0x0  -> unused
//                             // // 0x1  -> the permission packet failed!
//                             // // 0x2  -> the permission packet went through
//                             // // 0x3  -> all permission pacets for that address
//                             // //          went through
//                             // did_permission_failed = 0x2;
                            
//                             // this should not happen
//                             // portMap[permission_pkt->id] = packet_id;


//                         }
//                         // TODO: can this packet go into the same retry queue?
//                         else {
//                             DPRINTF(PermissionPackets,
//                                     "Couldn't send %#x for %#x on port %lu\n",
//                                     permission_pkt->getAddr(),
//                                     pkt->getAddr(),
//                                     packet_id);
//                             // only store the actual request and delete the
//                             // fake request.
//                             // it'll be created again.
//                             //
//                             // do not push the permission packet into the retry
//                             // queue
//                             // retry_queue.push(packet_id);
//                             // delete the traffic packet
//                             delete permission_pkt;
//                             // delete req;

//                             // also cannot send the actual packet now as the permission packet will always go first.
//                             // return false;
//                         }
//                         // forward the clock by 1 tick to queue permission packets
//                         // schedule(delayEvent, curTick() + cyclesToTicks(1));
//                         // processEvent(0);

//                         // explicitly not sent, so queue the request
//                         // retry_queue.push(packet_id);
//                         // ideally we want the stalling code here!
//                         DPRINTF(PermissionPackets, "Permission packet number %d for %#x "
//                                             "was sent but the actual packet "
//                                             "is explicitly stalled! Queue size: %lu\n",
//                                             permission_checker[pkt->getAddr()],
//                                             pkt->getAddr(),
//                                             retry_queue.size());
//                         scheduleCpuRetry();
//                         retry_queue.push(packet_id);
//                         return false;

//                     }
//                     // if the total number of permission packets are already
//                     // sent then notify the user that the permission checker
//                     // has finished its job
//                     else {
//                             DPRINTF(PermissionPackets,
//                                     "Permissions for %#x are sent! Waiting for "
//                                     "the SimObject to actually send the pkt\n",
//                                     pkt->getAddr());

//                     }
//                 }
//             } // __ if (is_remote) __
//         } // __ if (useDedicatedCaching) __
//     } // __ if (enablePermissions) __

//     // business as usual. 
//     //
//     // regardless of whether this is a permission packet, remote memory packet
//     // or a local memory packet, it always needs to go through.
//     //
//     // we should not send a packet if the corresponding permission packets for
//     // that packet is not sent yet!

//     if (memSidePort.sendTimingReq(pkt)) {
//         // retry_queue.set();
//         // business as usual!
//         if (is_remote) {
//             DPRINTF(PermissionPackets, "Finally sent %#x on port %lu!\n",
//                                                 pkt->getAddr(), packet_id);
//             // reset the count so that the request needs to fetch the 
//             // permission entry again.
//             permission_checker[pkt->getAddr()] = 0;
//         }
//         else { // Found the retry 
//             // this is a regular packet.
//             DPRINTF(ClockedPermissionDebug, "Send %#x on port %lu\n",
//                                                                 pkt->getAddr(),
//                                                                 packet_id);
//         }
//         // keep a track of the port
//         portMap[pkt->id] = packet_id;
//         return true;
//     }

//     // the packet was failed to be sent to the memsideport
//     if (is_remote)
//         DPRINTF(PermissionPackets, "All permission packets for %#x on port "
//                                     "%lu are sent but the real packet failed!"
//                                     "\n", pkt->getAddr(), packet_id);
//     else
//         // business as usual!
//         DPRINTF(ClockedPermissionDebug, "Failed to send %#x on port %lu\n",
//                                                 pkt->getAddr(), packet_id);
    
//     // tell the retry logic to try again on the same port.

//     // scheduleCpuRetry();
//     retry_queue.push(packet_id);
//     return false;
// }

// make sure the event is correctly set for this simobject
void
ClockedPermission::processEvent() {
    // This is only called if the user wants to add permission checks
    // make sure that this address is scheduled with some additional latency.
    // DPRINTF(PermissionTable, "Scheduling addr %#x with %lu latency\n",
    //                                     pkt->getAddr(), class_latency);
    // the class latency must be set before the event can be called.
    DPRINTF(PermissionPackets, "kg says hi\n");
    // assert(class_latency == hitLatency || 
    //             class_latency == creationLatency + missLatency || 
    //             class_latency == missLatency);

    // if the retry queue is not empty, then call retry
    // FIXME: I should not be doing this
//     if (!retry_queue.empty())
//         recvReqRetry();
// }
}

void
ClockedPermission::processEvent(int attempt) {
    // This is only called if the user wants to add permission checks
    // make sure that this address is scheduled with some additional latency.
    // DPRINTF(PermissionTable, "Scheduling addr %#x with %lu latency\n",
    //                                     pkt->getAddr(), class_latency);
    // the class latency must be set before the event can be called.
    DPRINTF(PermissionPackets, "Scheduling this* with %lu latency\n",
                                    1);
    // assert(class_latency == hitLatency || 
    //             class_latency == creationLatency + missLatency || 
    //             class_latency == missLatency);
    scheduleNewEvent();
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
        delete pkt;
        return true;
    }
    PacketId id = pkt->id;
    return cpuSidePorts[portMap[id]].sendTimingResp(pkt);
}

bool
ClockedPermission::recvTimingReq(PacketPtr pkt, uint64_t packet_id) {

    // i'll keep a track of packets in my class!
    // i'll keep putting events to send the permission packets until i run
    // out of buffer space. let's keep an infinite queue.
    
    // queue this packet in a 
    if (pkt->getAddr() >= 0x0 && pkt->getAddr() < 0x400000000) {
        real_packets.push(pkt);
        int current = permission_packet_count[pkt->getAddr()];

        if (current < 23) {
            // need to send a permission packet!
            // queue an event and do nothing really.
            scheduleCpuRetry();
            permission_packet_count[pkt->getAddr()]++;
            retry_queue.push(packet_id);
            return false;
        }
        else {
            // try sending the real packet from the queue.
            PacketPtr packet_to_send;
            if (real_packets.size() > 0)
                packet_to_send = real_packets.front();
            else
                return false;

            // now the retry is free!
            // retry_queue.set();
            if (memSidePort.sendTimingReq(packet_to_send)) {
                // successful
                // permission_checker[pkt->getAddr()] = 0;
                // also remove the retry request from the queue. force no retries
                // as the packet has been sent.
                // if (have_i_sent_this[pkt->getAddr()] == false) {
                    // retry_queue.set(packet_id);
                // }
                // have_i_sent_this[pkt->getAddr()] = true;
                // }
                // remove the entry fomr the permission packet count
                permission_packet_count.erase(packet_to_send->getAddr());
                real_packets.pop();
                
                DPRINTF(PermissionPackets, "Finally sent %#x on port %lu!\n",
                                                    packet_to_send->getAddr(), packet_id);
                portMap[packet_to_send->id] = packet_id;
                return true;
            }
            DPRINTF(PermissionPackets, "Failed to sent %#x on port %lu!\n",
                                                pkt->getAddr(), packet_id);
            // Since we couldn't accept the CPU's req (because we can't forward it),
            // we also need to block the CPU and only call cpuSidePort.sendRetryReq()
            // once we have successfully forwarded (or have buffer space).
            // For a simple "keep asking" demo, you could still schedule another CPU retry:
            // scheduleCpuRetry();

            // this is the only time we say false to the prior simobject.

            retry_queue.push(packet_id);
            return false;
        }
    }
    else {
        if (memSidePort.sendTimingReq(pkt)) {
            // successful
            // permission_checker[pkt->getAddr()] = 0;
            // also remove the retry request from the queue. force no retries
            // as the packet has been sent.
            // if (have_i_sent_this[pkt->getAddr()] == false) {
                // retry_queue.set(packet_id);
            // }
            // have_i_sent_this[pkt->getAddr()] = true;
            // }
            // remove the entry fomr the permission packet count
            
            DPRINTF(PermissionPackets, "Finally sent %#x on port %lu!\n",
                                                pkt->getAddr(), packet_id);
            portMap[pkt->id] = packet_id;
            return true;
        }
        DPRINTF(PermissionPackets, "Failed to sent %#x on port %lu!\n",
                                            pkt->getAddr(), packet_id);
        // Since we couldn't accept the CPU's req (because we can't forward it),
        // we also need to block the CPU and only call cpuSidePort.sendRetryReq()
        // once we have successfully forwarded (or have buffer space).
        // For a simple "keep asking" demo, you could still schedule another CPU retry:
        // scheduleCpuRetry();

        // this is the only time we say false to the prior simobject.

        retry_queue.push(packet_id);
        return false;

    }
    assert(false && "unreachable code\n");
    return false;

}

void
ClockedPermission::recvReqRetry() {
    // see if i have to queue events from the past.
    DPRINTF(PermissionPackets, "recvReqRetry is called with size %lu\n", retry_queue.size());
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
