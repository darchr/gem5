#include "permissions/flat_tables.hh"


#include "debug/FlatPermissionTables.hh"
#include "debug/PermissionTableEvent.hh"
#include "debug/FlatTablesDebug.hh"
#include "debug/PermissionPackets.hh"
#include "debug/RemoteAddress.hh"
#include "debug/PermissionResponses.hh"
#include "debug/PermissionCaching.hh"

namespace gem5 {

FlatTables::FlatTables(const FlatTablesParams &params) :
    ClockedObject(params),
    memSidePort(params.name + ".mem_side_port", *this),
    // cpuSidePort(params.name + ".cpu_side_port", this),
    modelName(params.model_name),
    enablePermissionCheck(params.enable_permission_check),
    useDedicatedCaching(params.use_dedicated_caching),
    baseAddrPermissionTable(params.permission_base_addr),
    numberOfEntries(params.number_of_entries),
    binarySearch(params.binary_search),
    permissionEntrySize(params.permission_entry_size),
    creationLatency(params.creation_latency),
    hitLatency(params.hit_latency),
    missLatency(params.miss_latency),
    cacheLookupLatency(params.cache_lookup_latency),
    cacheEntryCreationLatency(params.cache_entry_creation_latency),
    remoteMemoryStart(params.remote_memory_start),
    totalMemorySize(params.total_memory_size),
    localMemoryStart(params.local_memory_start),
    localMemoryEnd(params.local_memory_end),
    cacheSize(params.cache_size),
    segmentSize(params.segment_size),
    cachePolicy(params.cache_policy),
    // event([this]{processEvent();}, name()),
    mshrCount(params.mshr_count),
    hostID(params.host_id),
    event([this]{processEvent();}, name()),
    stats(this)
{
    for (int i = 0 ; i < params.port_cpu_side_ports_connection_count; i++)
        cpuSidePorts.emplace_back(
            name() + csprintf(".cpu_side_ports[%d]", i), i, *this, i
        );

    // start by processing the name of the model first.
    if (modelName == "mondrain")
        model_state = gem5::model::MONDRIAN;
    else if (modelName == "flat-table")
        model_state = gem5::model::FLAT_TABLE;
    else if (modelName == "deact")
        model_state = gem5::model::DEACT;
    else if (modelName == "space-control")
        model_state = gem5::model::SPACE_CONTROL;
    else {
        // this check must happen at the python side.
        fatal("Model %s is not supported. Valid options are: mondrian, "
            "flat-table, deact, space-control\n", modelName);
    }

    // specific parameters are setup based on the model that the user wants to
    // set up.

    // make sure that the user has defined the totla memory size. This is
    // required to setup the tables if needed.
    panic_if(totalMemorySize == 0,
        "The ClockedPermmission needs to know the size of the memory!\n");
    
    // make sure the start of the memory range is no longer hardcoded
    panic_if(remoteMemoryStart == 0,
        "define the start of the remote memory in the permission object!\n");
    // if the user wants mondi, they need to define the segment size
    if (model_state == gem5::model::MONDRIAN) {
        // logic: there is a start and an address with permissions
        panic_if(segmentSize != 16, "Cannot simulate mondrian with segment"
                            " size set to anything other than 16B\n");
        // mondi also requires the start and end addresses of the local memory
        panic_if(localMemoryStart == 0x0, "Set the local memory's start and "
                                        "end addresses correctly!");
        panic_if(localMemoryEnd == 0x0, "Set the local memory's start and "
                                        "end addresses correctly!");
    }
    panic_if(
        model_state == gem5::model::SPACE_CONTROL && segmentSize != CACHE_LINE,
        "Entry size for space control must be the same as PPN!");

    
    // extending the PLB model, we need to implement the number of lookups
    // needed to fetch an entry from the permission table via binary
    // search

    panic_if(permissionEntrySize == 0, "Permission entry cannot be zero!");

    // this becomes relevant if simulating flat tables or worst cases for
    // mondrain or space-control. it is expected that the user will simulate
    // segmentSize as PPN
    warn_if((model_state == gem5::model::FLAT_TABLE || 
                            model_state == gem5::model::DEACT) &&
                            (segmentSize != 4096), "Flat tables are expected "
                            " to be the same as PPN 4096!\n");

    if (model_state == gem5::model::DEACT || model_state == gem5::model::FLAT_TABLE)
        // For flat tables, each 4K page has a different permission entry
        total_entries = totalMemorySize / PPN_MASK;
    else {

        // The user needs to provide the number of entries in mondrian and
        // out technique
        assert(numberOfEntries != 0);

        warn("number_of_entries is not 0!, overriding the number of entries!");
        // the user can set the total entries foe each PPN. this is the WC for
        // mondi and us
        worst_case = ((totalMemorySize / PPN_MASK) == numberOfEntries)
                     ? true: false;
        total_entries = numberOfEntries;
    }

    DPRINTF(FlatPermissionTables,
            "Permission table has %lu entries\n", total_entries);

    // All plb packets will be of size 64 bytes. This will always be 64.
    permission_block_size = 64;

    // All permission packets will be of READ type, unless the drive is trying
    // to write to the table. This will always happen in KVM/INIT state.
    permission_cmd = MemCmd::ReadReq;

    // warn the user that this will be ignored for flat tables
    if (model_state == gem5::model::FLAT_TABLE) {
        max_search_attempts = 1;
    }
    else if (model_state == gem5::model::DEACT) {
        // each memory request cannot be more than 64 bytes.
        // In this implementation, DeACT still does 1 access.
        assert(permissionEntrySize == 64);
        max_search_attempts = permissionEntrySize / permission_block_size;
    }
    else {
        // see if binry search is set.
        if (binarySearch) {
            // it must never be zero. The ceil takes care of the total number
            // of entries.
            max_search_attempts = 
                ceil(log2(total_entries)) > 0 ? ceil(log2(total_entries)) : 1;
        }
        else {
            // this is a linear search
            max_search_attempts = total_entries;
        }
    }

    // inform the user.
    DPRINTF(FlatPermissionTables,
                    "Lookups will take %d attempts\n", max_search_attempts);

    // make sure to 

    // if the user wants to simulate flat tables, the number of entries will be
    // fixed.
    

    // Calculate the total size of the memory's permission table. Each MMP
    // entry is addr + size + permission (64 + 64 + 2 = 130 bits). We're doing
    // a bit of cheating and making sure that the maximum size can be 2^62
    // instead of total 64 bits (never going to happen anyway). Converting
    // this to bytes, we have: 128 / 8 = 16 Bytes.


    // create a mask for the the segment size. Each cached entry will be 64 B
    // and each segment table size will be segmentSize.
    // TODO: Marked for deletion
    // segment_mask = 0xFFFFFFFF & !(segmentSize - 1);

    // Space control needs to model the caches.
    if (model_state == gem5::model::SPACE_CONTROL) {
        if (useDedicatedCaching) {
            fatal_if(cacheLookupLatency == 0, "Cannot have an instantaneous"
                                    " cache look up latency");
            fatal_if(cacheEntryCreationLatency == 0, "Cannot have an"
                                    " instantaneous cache creation latency");
        
            // next up, we need to fix the cache_mask (64 bytes)
            cache_mask = CACHE_LINE;
        }
        panic_if(segmentSize != CACHE_LINE, "Cannot simulate SC with segment"
                            " size set to anything other than 64B\n");
        // Now set up the cache. We don't really need a lot of cache to maintain
        // this table.
        // We maintain a couple of states of the address in the cache. I am keeping
        // a couple of values to make sure that it is compatible with all types of
        // caches.
        // address, [is_cached (bool), last_accessed (Tick), access_count (int)]
        // The map is initialized with a dummy entry in the beginning. We might
        // remove this in the future.
        // permission_table.insert({uint64_t(-1), new struct cache_entry_vector});

        // Whether an entry is cached or not is detemined by the number of
        // is_cache number.
        total_cached_entries = 0;

        // need to figure out the maximum number of cachable entries. cacheSize
        // is in Bytes.
        max_cached_entries = cacheSize / 64;
        DPRINTF(PermissionCaching, "Permission cache has %lu entries\n",
                                                        max_cached_entries);
    }
    
    // mshrs
    mshrs_occupied = 0;

    // table isze 
    if (model_state == gem5::model::FLAT_TABLE) {
        // each entry is of 2 bits. An entry is created per 4 KiB
        table_size = ((totalMemorySize) / (4096) ) * 2 / 8;   // in bytes!
    }
    else if (model_state == gem5::model::DEACT) {
        // It's 1 Byte. the table doesn't repeat per host but per process.
        table_size = (totalMemorySize) / (4096);
    }
    // for the rest of these, we'll do a best case and worst case simulation
    // only
    else if (model_state == gem5::model::MONDRIAN) {
        // the table is fixed -> just 1. the entries vary.
        table_size = ONE_G;
    }
    else if (model_state == gem5::model::SPACE_CONTROL) {
        // similar to MONDRIAN
        // the table is fixed -> just 1. the entries vary.
        table_size = ONE_G;
    }

    // cache_policy = cachePolicy;
    // total_cache_size = cacheSize;
    // permission_cache_line_size = 64;
    // number_of_entries = total_cache_size / permission_cache_line_size;

    // avoid class variable assignment
    waiting_for_cpu_retry = false;
    waiting_for_mem_retry = false;

    need_to_drain = false;

    inflight_packets = 0;

}

AddrRangeList
FlatTables::getAddrRanges() const
{
    return memSidePort.getAddrRanges();
}

Tick
        // I don't want any unforseen 
FlatTables::recvAtomic(PacketPtr pkt)
{
    memSidePort.sendAtomic(pkt);
    return Tick();
}

void
FlatTables::recvFunctional(PacketPtr pkt)
{
    memSidePort.sendFunctional(pkt);
}

// void
// FlatTables::init() {
//     requestorId = -1;
// }

void
FlatTables::createDrainEvent() {
    assert(need_to_drain);
    DPRINTF(PermissionCheckpoint, "Draing: req: %lu, resp: %lu, flight: %lu\n",
        permission_packets.size(), response_packets.size(), inflight_packets);

    if (!event.scheduled())
        schedule(event, clockEdge(Cycles(1)));
}

void
FlatTables::processEvent() {
    // To have a non-infinite cache for flat tables, we need a version with
    // process events.
    DPRINTF(PermissionPackets, "Items to process %lu, current state %d\n",
                                permission_packets.size(), waiting_for_cpu_retry);
    if (!permission_packets.empty()) {
        processPermissionRequest();
    }
    // If I am witing on a permission response, then try rescheduling the
    // resp packet!
    if (!response_packets.empty())
        processPendingResponse();
    //     // recvRespRetry(0);

    // the user wants to drain this into a checkpoint. make sure everything is
    // ready
    if (need_to_drain == true) {
        // see if there are more events
        DPRINTF(PermissionCheckpoint,
        "stats :: Draing: req: %lu, resp: %lu, flight: %lu\n",
        permission_packets.size(), response_packets.size(), inflight_packets);
        if (permission_packets.empty() && response_packets.empty() && inflight_packets == 0) {
            signalDrainDone();
        } 
        // else there are more events.
        
        if (!event.scheduled())
            schedule(event, clockEdge(Cycles(1)));
    }
}

void
FlatTables::processPendingResponse() {
    // if i have pakets in the response queue for whihc i have their permission
    // packet then send this packet to the cpu side port
    if (!response_packets.empty()) {
        PacketPtr pkt = response_packets.front();
        if (permission_response_tracker[pkt->getAddr()] > 0) {
            // now i have a response for this pending packet in the queue.
            PacketId id = pkt->id;
            if (cpuSidePorts[portMap[id]].sendTimingResp(pkt)) {
                // real response sent!
                DPRINTF(PermissionResponses, "Finally sent waiting packet %#x\n",
                                                pkt->getAddr());
                response_packets.pop();

                // note the delay
                stats.packetLatency.sample(
                                gem5::curTick() - outstanding_packets[pkt]);
                outstanding_packets.erase(pkt);
            }
            else {
                // packet sending failed!
                waiting_for_mem_retry = true;
            }
            
        }
        else {
            DPRINTF(PermissionResponses, "Still couldn't send %#x\n",
                                            pkt->getAddr());
        }
    }
    // If I couldn't clear the queue this time, then schedule another event
    if (!response_packets.empty())
        if (!event.scheduled())
            schedule(event, clockEdge(Cycles(1)));
}

// void
// FlatTables::processPermissionRequest() {
//     // i am not waiting for the retry
//     if (!waiting_for_cpu_retry) {
//         PacketPtr pkt = permission_packets.front();
//         if (memSidePort.sendTimingReq(pkt)) {
//             mshrs_occupied++;

//             gem5::Addr originalAddr = 0x0;
//             if (pkt->senderState != nullptr) {
//                 auto *state =
//                         dynamic_cast<PermissionSenderState*>(pkt->senderState);
//                 if (state) {
//                     // PacketPtr originalPkt = state->originalPkt;
//                     originalAddr = state->originalAddr;

//                     // Now you know which memory request this permission
//                     // response belongs to
//                     // You can update your permission tracker accordingly
//                 }
//                 else
//                     fatal("Sender state cannot be null\n");
//             }
//             DPRINTF(PermissionPackets, "Sent permission pkt %#x for %#x\n",
//                                 pkt->getAddr(), originalAddr);

//             permission_request_tracker[originalAddr]++;
//             permission_packets.pop();
//         }
//         else {
//             // permission packet failed
//             waiting_for_cpu_retry = true;

//         }
//     }
//     // if we have pending packets, schedule another event!
//     if (permission_packets.size() != 0) {
//         if (!event.scheduled())
//             schedule(event, clockEdge(Cycles(1)));
//     }
// }

void
FlatTables::processPermissionRequest() {
    // i am not waiting for the retry
    if (!waiting_for_cpu_retry) {
        PacketPtr pkt = permission_packets.front();
        if (memSidePort.sendTimingReq(pkt)) {

            gem5::Addr originalAddr = 0x0;
            if (pkt->senderState != nullptr) {
                auto *state =
                        dynamic_cast<PermissionSenderState*>(pkt->senderState);
                if (state) {
                    // PacketPtr originalPkt = state->originalPkt;
                    originalAddr = state->originalAddr;

                    // Now you know which memory request this permission
                    // response belongs to
                    // You can update your permission tracker accordingly
                }
                else
                    fatal("Sender state cannot be null\n");
            }
            DPRINTF(PermissionPackets, "Sent permission pkt %#x for %#x\n",
                                pkt->getAddr(), originalAddr);

            permission_request_tracker[originalAddr]++;

            // only occupy mshrs when total number of required permission
            // packets are reached.
            if (permission_request_tracker[originalAddr] == max_search_attempts)
                mshrs_occupied++;

            permission_packets.pop();

            // count this as a successful permission packet sent!
            ++stats.numOutgoingPermissionPackets;

            // since this packet went to the mem side, increase the memside
            // as well.
            ++stats.numOutgoingMemSidePackets;
        }
        else {
            // permission packet failed
            waiting_for_cpu_retry = true;

        }
    }
    // if we have pending packets, schedule another event!
    if (permission_packets.size() != 0) {
        if (!event.scheduled())
            schedule(event, clockEdge(Cycles(1)));
    }
}

Addr
FlatTables::getFlatTableAddress(Addr addr) {
    // the flat table structure keeps the copy of the table multiple times
    // we assume that the permission table is replicated.

    // for simulations, there will only be one table per host.
    // Each host's memory map is TOTAL_MEMORY_SIZE / 4 KiB Pages * 2 bits

    // each entry is of 2 bits (00 - > no, 01 -> read, 10 -> write).
    // While the permission address will remain percise, the address
    // should align with 64 Bytes The last
    // 8 bits of the address should be zero no?
    // make sure that the address within the permission table range
    //
    // permissions are per PPN. Each pernmission is of 2 bits
    Addr base = baseAddrPermissionTable + (hostID * table_size) +
                                                (addr / 0x1000);
    // Each cache line has 256 PPN's permissions.
    Addr offset = (addr / 0x1000) % 256;
    // I don't want any unforseen consequences
    assert(isInPermissionRange(base + offset));
    return base + offset;
}

Addr
FlatTables::getDeACTAddress(Addr addr, bool shared) {
    // DeACT needs two reads: one per PPN to see if the host has access (ACM)
    // The second is the shared memory bitmap. The PPN table is replicated for
    // each host.

    if (!shared) {
        // ACM table. Each entry is indexed by the PPN (same as a flat-table)
        // permissions are per PPN. Each pernmission is of 2 bits.
        // Each entry has lg(256) = 8 bits -> 1 byte for the host id and
        // 2 more bits for read write access permission (10 bits per entry)
        // Round that off to 1B
        // base is the cache_line base
        Addr base = baseAddrPermissionTable +
                                     (hostID * table_size) +
                                     (addr / 0x1000);
        // Consequtive 64 PPNs will have the same base address.
        Addr offset = (addr / 0x1000) % 64;
        // I don't want any unforseen consequences
        assert(isInPermissionRange(base + offset));

        // if we implement cache, then just return the base
        return base + offset;
    }
    else {
        // the user wants the bitmap address. the bitmap is hardcoded to
        // 500 MiB. Each entry is 256 bits (each ppn has a bitmap)
        Addr base = baseAddrPermissionTable + 0x20000000 + (addr / 0x1000);
        // Consequtive 8 PPNs will have the same base address.
        Addr offset = (addr / 0x1000) % 64;
        // I don't want any unforseen consequences
        assert(isInPermissionRange(base + offset));
        // if we implement cache, then just return the base
        return base + offset;
    }
}

Addr
FlatTables::getMondrianAddress(Addr addr) {
    // each entry is 128 bits (round off from 130 bits (start, end, permission)
    // ) and the table is repeated for each host. (16B)

    // entries could be the total number of processes, or each PPN has a
    // different set of permissions. Each 64B cache line has 4 entries.

    // this needs to a bit hand waved. there is either single entry or all ppn
    // entries
    if (!worst_case) {
        // there is just one 16B entry in the entire table. Each entry is defined
        // by the segmentSize.
        return baseAddrPermissionTable + (hostID * 16);
    }
    else {
        // make sure that the permission table is always stored in the remote
        // memory. for local addresses, just return the base address
        if (!isInRemoteRange(addr)) {
            // Estimation
            return baseAddrPermissionTable + (hostID * 16);
        }
        // based on the host, we first get first index
        // each table size = (total_entries / segmentSize)
        Addr base = baseAddrPermissionTable +                   // base
                    (hostID * (total_entries * segmentSize)) +  // which table
                    ((addr - remoteMemoryStart) / PPN_MASK) * segmentSize;
                    // entry in the table
        // The exact offset (since there are 4x16Bytes in one cache line)
        // since each entry is cache aligned, you should not send the
        // offset.
        // Addr offset = (((addr - remoteMemoryStart) / PPN_MASK) * segmentSize)
        //                 % (CACHE_LINE / segmentSize);
        // I don't want any unforseen consequences
        assert(isInPermissionRange(base));
        return base;
    }
    // the number of entries is only relevant to model lookup time as a binary
    // search.
}


Addr
FlatTables::getSpaceControlAddress(Addr addr) {
    // each entry is 128 bits (round off from 130 bits (start, end, permission)
    // ) and the table is repeated for each host. (16B)

    // entries could be the total number of processes, or each PPN has a
    // different set of permissions. Each 64B cache line has 4 entries.

    // this needs to a bit hand waved. there is either single entry or all ppn
    // entries
    if (!worst_case) {
        // there is only a single entry that starts where the table starts.
        return baseAddrPermissionTable;
    }
    else {
        // each entry is of 64 bytes (i.e. segmentSize). Every 4K page has
        // different permissions!
        Addr base = baseAddrPermissionTable + 
                    ((addr - remoteMemoryStart) / PPN_MASK) * segmentSize;
        // There is no offset in Space Control as each entry is always cache
        // aligned.
        // I don't want any unforseen consequences
        assert(isInPermissionRange(base));
        return base;
    }
    // the number of entries is only relevant to model lookup time as a binary
    // search.
}

bool
FlatTables::recvTimingReqMondrian(PacketPtr pkt, uint64_t packet_id) {
    // we're going with the simple logic
    if (enablePermissionCheck) {
        // enable permissions for all memory addresses
        if (isInMemoryRange(pkt->getAddr())) {
            // checking for permissions. assume that every 4 KiB page has
            // access bits per host.
            // queue all permission packets into the packet queue and let the
            // regular packets go through naturally

            // if (!waiting_for_cpu_retry) {
            Addr permission_addr = getMondrianAddress(pkt->getAddr());
            if (useDedicatedCaching) {
                // cache the packet
                assert(false && "Not implemented error!");
            }

            // for all tghe number of permisison lookups, there are a lot of
            // permission packets
            for (int i = 0 ; i < max_search_attempts ; i++) { 
                // Request::Flags flags;
                RequestPtr req = std::make_shared<Request>(permission_addr,
                                                            1,
                                                            pkt->req->getFlags(),
                                                            pkt->requestorId());
                // cannot send a higher memory packet than the cache-line size
                // TODO in the class contructor
                PacketPtr permission_pkt = new Packet(req, permission_cmd,
                                                            permissionEntrySize);
                // TODO:
                // make sure that the SenderState is correctly set.
                // req->setFlags(Request::VALID_SIZE);
                // permission_pkt->setSize(permissionEntrySize);

                // system caches should not be used for permission packets. This
                // should always be enabled.
                permission_pkt->req->setFlags(Request::UNCACHEABLE);

                permission_pkt->allocate();
                // what is the sender state?
                permission_pkt->senderState = new PermissionSenderState(pkt);

                // this is the additonal latency required for the packet creation.
                schedule(new EventFunctionWrapper([this, permission_pkt]{ },
                    name() + ".accessEvent", true),
                    clockEdge(static_cast<Cycles>(creationLatency)));
                
                // don't send the packet, instead create an event!

                permission_packets.push(permission_pkt);
            }

            // just schedule one event and the queue will start getting
            // processed
            // schedule an event to process this packet
            if (!permission_packets.empty())
                if (!event.scheduled())
                    schedule(event, clockEdge(Cycles(1)));

            // make an event to send this packet later.
            // back pressure must be modeled in the resp side!
        }
        // now send the real packet.
        // business as usual. if the permission packet is not sent, then this part
        // of the code will never reach.

        // now that there are two channels of sending packets, we need to make
        // sure that the xbar is ready to receive real packets.

        // this port might be filled up
        bool to_process = (!waiting_for_cpu_retry);

        // if this is a snoop request, then you have to let it go;
        if (!(pkt->isRead() || pkt->isWrite()))
            to_process = true;
        if (to_process) {
            if (memSidePort.sendTimingReq(pkt)) {
                // Send successful, keep the packet_id for later.s
                DPRINTF(PermissionPackets, "Sent pkt %#x!\n",
                                            pkt->getAddr());
                portMap[pkt->id] = packet_id;

                // since this packet was sent successfully, increase the memside
                // packets
                ++stats.numOutgoingMemSidePackets;

                // keep the time on when this packet was sent from the permission
                // checker to the memory. this is only true for real packets with
                // permission checks
                if (isInRemoteRange(pkt->getAddr()))
                    outstanding_packets[pkt] = gem5::curTick();

                return true;
            }
        }
        // either i was waiting for cpu retry or sending failed!
        
        // the actual packet was unsuccessful
        DPRINTF(PermissionPackets, "Failed to send pkt %#x!\n", pkt->getAddr());
        waiting_for_cpu_retry = true;
        retry_queue.push(packet_id);
        return false;
    }
    fatal("should be unreachable");
    return false;
}
bool
FlatTables::recvTimingReqDeACT(PacketPtr pkt, uint64_t packet_id) {
    // we're going with the simple logic
    if (enablePermissionCheck) {
        // enable permissions for remote memory addresses only
        if (isInRemoteRange(pkt->getAddr())) { // && permission_checker[pkt->getAddr()] == false) {
            // checking for permissions. assume that every 4 KiB page has
            // access bits per host.
            // queue all permission packets into the packet queue and let the
            // regular packets go through naturally

            // if (!waiting_for_cpu_retry) {
            // DeACT needs two memory lookups: one for the ACM and the other
            // for the shared bitmap
            Addr acm_addr = getDeACTAddress(pkt->getAddr(), false);
            Addr bitmap_addr = getDeACTAddress(pkt->getAddr(), true);


            if (useDedicatedCaching) {
                // cache the packet
                assert(false && "Not implemented error!");
            }
            // Request::Flags flags;
            RequestPtr acm_req = std::make_shared<Request>(acm_addr,
                                                        1,
                                                        pkt->req->getFlags(),
                                                        pkt->requestorId());
            // cannot send a higher memory packet than the cache-line size
            // TODO in the class contructor
            PacketPtr acm_pkt = new Packet(acm_req, permission_cmd,
                                                        permissionEntrySize);
            // TODO:
            // make sure that the SenderState is correctly set.
            // req->setFlags(Request::VALID_SIZE);
            // permission_pkt->setSize(permissionEntrySize);

            // system caches should not be used for permission packets. This
            // should always be enabled.
            acm_pkt->req->setFlags(Request::UNCACHEABLE);

            acm_pkt->allocate();
            // what is the sender state?
            acm_pkt->senderState = new PermissionSenderState(pkt);

            // this is the additonal latency required for the packet creation.
            schedule(new EventFunctionWrapper([this, acm_pkt]{ },
                name() + ".accessEvent", true),
                clockEdge(static_cast<Cycles>(creationLatency)));
            
            // don't send the packet, instead create an event!
            permission_packets.push(acm_pkt);

            // Now create another packet for the bitmap lookup
            // Request::Flags flags;
            RequestPtr bitmap_req = std::make_shared<Request>(bitmap_addr,
                                                        1,
                                                        pkt->req->getFlags(),
                                                        pkt->requestorId());
            // cannot send a higher memory packet than the cache-line size
            // TODO in the class contructor
            PacketPtr bitmap_pkt = new Packet(bitmap_req, permission_cmd,
                                                        permissionEntrySize);
            // TODO:
            // make sure that the SenderState is correctly set.
            // req->setFlags(Request::VALID_SIZE);
            // permission_pkt->setSize(permissionEntrySize);

            // system caches should not be used for permission packets. This
            // should always be enabled.
            bitmap_pkt->req->setFlags(Request::UNCACHEABLE);

            bitmap_pkt->allocate();
            // what is the sender state?
            bitmap_pkt->senderState = new PermissionSenderState(pkt);

            // this is the additonal latency required for the packet creation.
            schedule(new EventFunctionWrapper([this, bitmap_pkt]{ },
                name() + ".accessEvent", true),
                clockEdge(static_cast<Cycles>(creationLatency)));
            
            // don't send the packet, instead create an event!
            permission_packets.push(bitmap_pkt);



            // schedule an event to process this packet
            if (!event.scheduled())
                schedule(event, clockEdge(Cycles(1)));

            // make an event to send this packet later.
            // back pressure must be modeled in the resp side!
        }
        // now send the real packet.
        // business as usual. if the permission packet is not sent, then this part
        // of the code will never reach.

        // now that there are two channels of sending packets, we need to make
        // sure that the xbar is ready to receive real packets.
        if (memSidePort.sendTimingReq(pkt)) {
            // Send successful, keep the packet_id for later.s
            DPRINTF(PermissionPackets, "Sent pkt %#x!\n",
                                        pkt->getAddr());
            portMap[pkt->id] = packet_id;

            // since this packet was sent successfully, increase the memside
            // packets
            ++stats.numOutgoingMemSidePackets;

            // keep the time on when this packet was sent from the permission
            // checker to the memory. this is only true for real packets with
            // permission checks
            if (isInRemoteRange(pkt->getAddr()))
                outstanding_packets[pkt] = gem5::curTick();

            return true;
        }
        
        // the actual packet was unsuccessful
        DPRINTF(PermissionPackets, "Failed to send pkt %#x!\n", pkt->getAddr());
        waiting_for_cpu_retry = true;
        retry_queue.push(packet_id);
        return false;
    }
    fatal("should be unreachable");
    return false;
}
bool
FlatTables::recvTimingReqSpaceControl(PacketPtr pkt, uint64_t packet_id) {
    
    // // we're going with the simple logic
    // if (enablePermissionCheck) {
    //     // enable permissions for remote memory addresses only
    //     if (isInRemoteRange(pkt->getAddr())) { // && permission_checker[pkt->getAddr()] == false) {
    //         // checking for permissions. assume that every 4 KiB page has
    //         // access bits per host.
    //         // queue all permission packets into the packet queue and let the
    //         // regular packets go through naturally

    //         // the number of packets will be different
    //         for (int i = 0 ; i < max_search_attempts ; i++) {
    //             Addr permission_addr = getFlatTableAddress(pkt->getAddr());
    //             if (useDedicatedCaching) {
    //                 // cache the packet
    //                 assert(false && "Not implemented error!");
    //             }
    //             // Request::Flags flags;
    //             RequestPtr req = std::make_shared<Request>(permission_addr,
    //                                                         1,
    //                                                         pkt->req->getFlags(),
    //                                                         pkt->requestorId());
    //             // cannot send a higher memory packet than the cache-line size
    //             // TODO in the class contructor
    //             PacketPtr permission_pkt = new Packet(req, permission_cmd,
    //                                                         permissionEntrySize);
    //             // TODO:
    //             // make sure that the SenderState is correctly set.
    //             // req->setFlags(Request::VALID_SIZE);
    //             // permission_pkt->setSize(permissionEntrySize);

    //             // system caches should not be used for permission packets. This
    //             // should always be enabled.
    //             permission_pkt->req->setFlags(Request::UNCACHEABLE);

    //             permission_pkt->allocate();
    //             // what is the sender state?
    //             permission_pkt->senderState = new PermissionSenderState(pkt);

    //             // this is the additonal latency required for the packet creation.
    //             schedule(new EventFunctionWrapper([this, permission_pkt]{ },
    //                 name() + ".accessEvent", true),
    //                 clockEdge(static_cast<Cycles>(creationLatency)));

    //             // DPRINTF(PermissionPackets, "addr: %#x and permission pkt addr %#x "
    //             //                             "and og senderstate addr %#x\n",
    //             //                                 pkt->getAddr(),
    //             //                                 permission_pkt->getAddr(),
    //             //                                 permission_pkt->senderState->originalAddr);
                
    //             // don't send the packet, instead create an event!

    //             permission_packets.push(permission_pkt);
    //         }

    //         // schedule an event to process this packet
    //         if (!event.scheduled())
    //             schedule(event, clockEdge(Cycles(1)));

    //         // make an event to send this packet later.
    //         // back pressure must be modeled in the resp side!
    //     }
    //     // now send the real packet.
    //     // business as usual. if the permission packet is not sent, then this part
    //     // of the code will never reach.

    //     // now that there are two channels of sending packets, we need to make
    //     // sure that the xbar is ready to receive real packets.
    //     if (memSidePort.sendTimingReq(pkt)) {
    //         // Send successful, keep the packet_id for later.s
    //         DPRINTF(PermissionPackets, "Sent pkt %#x!\n",
    //                                     pkt->getAddr());
    //         portMap[pkt->id] = packet_id;
    //         return true;
    //     }
        
    //     // the actual packet was unsuccessful
    //     DPRINTF(PermissionPackets, "Failed to send pkt %#x!\n", pkt->getAddr());
    //     waiting_for_cpu_retry = true;
    //     retry_queue.push(packet_id);
    //     return false;
    // }
    // fatal("should be unreachable");
    // return false;
    // we're going with the simple logic
    if (enablePermissionCheck) {
        // enable permissions for remote memory addresses
        if (isInRemoteRange(pkt->getAddr())) {
        // if (isInMemoryRange(pkt->getAddr())) {

            // checking for permissions. assume that every 4 KiB page has
            // access bits per host.
            // queue all permission packets into the packet queue and let the
            // regular packets go through naturally

            // if (!waiting_for_cpu_retry) {
            Addr permission_addr = getSpaceControlAddress(pkt->getAddr());
            if (useDedicatedCaching) {
                // cache the packet
                // assert(false && "Not implemented error!");
                // 1. we need to find out the addr
                // 2. see if that packet is cached.
                // 3. if not, then send the memory request.
                
                // generate all the addresses to send.
                int miss_count = 0;
                for (int i = 0 ; i < max_search_attempts ; i++) {
                    // this is a binary address list.
                    permission_addr = getSpaceControlAddress(pkt->getAddr());
                    
                    // see if this is cached.
                    if (isCached(maskAddr(permission_addr))) {
                        DPRINTF(PermissionCaching,
                            "Permission add %x is cached and simulated with"
                            " hit time  %lu\n", pkt->getAddr(), hitLatency);
                        incrementCounts(maskAddr(permission_addr));
                        // schedule a lookup with cache hit
                        schedule(new EventFunctionWrapper([this, pkt]{ },
                            name() + ".accessEvent", true),
                            clockEdge(static_cast<Cycles>(hitLatency)));
                        // Important to note here that if all the lookup 
                        // packets needed to verify this request are cached,
                        // then you don't need to enforce the permission check 
                        // by stalling the packet at the response end.
                    }
                    // if this packet is not cached, create a lookup packet
                    else {
                        DPRINTF(PermissionCaching,
                            "Permission add %x is cached and simulated with"
                            " hit time  %lu\n", pkt->getAddr(), hitLatency);
                        miss_count++;
                        // need to create a permission lookup packet
                        RequestPtr req = std::make_shared<Request>(
                                                        permission_addr,
                                                        1,
                                                        pkt->req->getFlags(),
                                                        pkt->requestorId());
                        // cannot send a higher memory packet than the
                        // cache-line size: This is already addressed.
                        PacketPtr permission_pkt = new Packet(req,
                                                        permission_cmd,
                                                        permissionEntrySize);
                        // TODO:
                        // make sure that the SenderState is correctly set.
                        // req->setFlags(Request::VALID_SIZE);
                        // permission_pkt->setSize(permissionEntrySize);

                        // system caches should not be used for permission
                        // packets. This should always be enabled.
                        permission_pkt->req->setFlags(Request::UNCACHEABLE);

                        permission_pkt->allocate();
                        // what is the sender state?
                        permission_pkt->senderState =
                                                new PermissionSenderState(pkt);

                        // this is the additonal latency required for the packet
                        // creation.
                        schedule(new EventFunctionWrapper(
                            [this, permission_pkt]{ },
                            name() + ".accessEvent", true),
                            clockEdge(static_cast<Cycles>(creationLatency)));
                        
                        // don't send the packet, instead create an event!
                        permission_packets.push(permission_pkt);
                    }
                }
                if (miss_count == 0)
                    // the packet doesn't need enforcement.
                    permission_response_tracker[pkt->getAddr()] =
                                                        max_search_attempts;
            }
            else {
                // for all tghe number of permisison lookups, there are a lot of
                // permission packets
                for (int i = 0 ; i < max_search_attempts ; i++) { 
                    // Request::Flags flags;
                    RequestPtr req = std::make_shared<Request>(permission_addr,
                                                            1,
                                                            pkt->req->getFlags(),
                                                            pkt->requestorId());
                    // cannot send a higher memory packet than the cache-line size
                    // TODO in the class contructor
                    PacketPtr permission_pkt = new Packet(req, permission_cmd,
                                                            permissionEntrySize);
                    // TODO:
                    // make sure that the SenderState is correctly set.
                    // req->setFlags(Request::VALID_SIZE);
                    // permission_pkt->setSize(permissionEntrySize);

                    // system caches should not be used for permission packets. This
                    // should always be enabled.
                    permission_pkt->req->setFlags(Request::UNCACHEABLE);

                    permission_pkt->allocate();
                    // what is the sender state?
                    permission_pkt->senderState = new PermissionSenderState(pkt);

                    // this is the additonal latency required for the packet creation.
                    schedule(new EventFunctionWrapper([this, permission_pkt]{ },
                        name() + ".accessEvent", true),
                        clockEdge(static_cast<Cycles>(creationLatency)));
                    
                    // don't send the packet, instead create an event!

                    permission_packets.push(permission_pkt);
                }

                // just schedule one event and the queue will start getting
                // processed
                // schedule an event to process this packet
                if (!permission_packets.empty())
                    if (!event.scheduled())
                        schedule(event, clockEdge(Cycles(1)));

                // make an event to send this packet later.
                // back pressure must be modeled in the resp side!
            }
        }
        else {
            // our design still checks for the C-bit
            // schedule an event to enforce this check.
            if (isInMemoryRange(pkt->getAddr())) {
                // only local addresses are trapped here
                // this is the additonal latency required for the packet creation.
                gem5::Tick c_bit_comparison_latency = 1;
                schedule(new EventFunctionWrapper([this, pkt]{ },
                    name() + ".accessEvent", true),
                    clockEdge(static_cast<Cycles>(c_bit_comparison_latency)));
            }
        }
        // now send the real packet.
        // business as usual. if the permission packet is not sent, then this part
        // of the code will never reach.

        // now that there are two channels of sending packets, we need to make
        // sure that the xbar is ready to receive real packets.

        // this port might be filled up
        if (!waiting_for_cpu_retry) {
            if (memSidePort.sendTimingReq(pkt)) {
                // Send successful, keep the packet_id for later.s
                DPRINTF(PermissionPackets, "Sent pkt %#x!\n",
                                            pkt->getAddr());
                portMap[pkt->id] = packet_id;

                // for signaling drain, we need to keep a track of outstanding
                // real packets
                inflight_packets++;

                // since this packet was sent successfully, increase the memside
                // packets
                ++stats.numOutgoingMemSidePackets;

                // keep the time on when this packet was sent from the permission
                // checker to the memory. this is only true for real packets with
                // permission checks
                if (isInRemoteRange(pkt->getAddr()))
                    outstanding_packets[pkt] = gem5::curTick();

                return true;
            }
        }
        // either i was waiting for cpu retry or sending failed!
        
        // the actual packet was unsuccessful
        DPRINTF(PermissionPackets, "Failed to send pkt %#x!\n", pkt->getAddr());
        waiting_for_cpu_retry = true;
        retry_queue.push(packet_id);
        return false;
    }
    fatal("should be unreachable");
    return false;
}

bool
FlatTables::recvTimingReqFlatTables(PacketPtr pkt, uint64_t packet_id) {
    // we're going with the simple logic
    if (enablePermissionCheck) {
        // enable permissions for remote memory addresses only
        if (isInRemoteRange(pkt->getAddr())) { // && permission_checker[pkt->getAddr()] == false) {
            // checking for permissions. assume that every 4 KiB page has
            // access bits per host.
            // queue all permission packets into the packet queue and let the
            // regular packets go through naturally

            // if (!waiting_for_cpu_retry) {
            Addr permission_addr = getFlatTableAddress(pkt->getAddr());
            if (useDedicatedCaching) {
                // cache the packet
                assert(false && "Not implemented error!");
            }
            // Request::Flags flags;
            RequestPtr req = std::make_shared<Request>(permission_addr,
                                                        1,
                                                        pkt->req->getFlags(),
                                                        pkt->requestorId());
            // cannot send a higher memory packet than the cache-line size
            // TODO in the class contructor
            PacketPtr permission_pkt = new Packet(req, permission_cmd,
                                                        permissionEntrySize);
            // TODO:
            // make sure that the SenderState is correctly set.
            // req->setFlags(Request::VALID_SIZE);
            // permission_pkt->setSize(permissionEntrySize);

            // system caches should not be used for permission packets. This
            // should always be enabled.
            permission_pkt->req->setFlags(Request::UNCACHEABLE);

            permission_pkt->allocate();
            // what is the sender state?
            permission_pkt->senderState = new PermissionSenderState(pkt);

            // this is the additonal latency required for the packet creation.
            schedule(new EventFunctionWrapper([this, permission_pkt]{ },
                name() + ".accessEvent", true),
                clockEdge(static_cast<Cycles>(creationLatency)));
            
            // don't send the packet, instead create an event!

            permission_packets.push(permission_pkt);

            // schedule an event to process this packet
            if (!event.scheduled())
                schedule(event, clockEdge(Cycles(1)));

            // make an event to send this packet later.
            // back pressure must be modeled in the resp side!
        }
        // now send the real packet.
        // business as usual. if the permission packet is not sent, then this part
        // of the code will never reach.

        // now that there are two channels of sending packets, we need to make
        // sure that the xbar is ready to receive real packets.
        if (memSidePort.sendTimingReq(pkt)) {
            // Send successful, keep the packet_id for later.s
            DPRINTF(PermissionPackets, "Sent pkt %#x!\n",
                                        pkt->getAddr());
            portMap[pkt->id] = packet_id;

            // since this packet was sent successfully, increase the memside
            // packets
            ++stats.numOutgoingMemSidePackets;

            // keep the time on when this packet was sent from the permission
            // checker to the memory. this is only true for real packets with
            // permission checks
            if (isInRemoteRange(pkt->getAddr()))
                outstanding_packets[pkt] = gem5::curTick();

            return true;
        }
        
        // the actual packet was unsuccessful
        DPRINTF(PermissionPackets, "Failed to send pkt %#x!\n", pkt->getAddr());
        waiting_for_cpu_retry = true;
        retry_queue.push(packet_id);
        return false;
    }
    fatal("should be unreachable");
    return false;
}

bool
FlatTables::recvTimingReq(PacketPtr pkt, uint64_t packet_id) {
    // count the number of incoming packets from the CPU as a stat. 
    ++stats.numIncomingCPUSidePackets;

    // // TODO
    // We want different helper functions for different techqniues.
    if (model_state == gem5::model::SPACE_CONTROL) {
        return recvTimingReqSpaceControl(pkt, packet_id);
    } 
    else if (model_state == gem5::model::MONDRIAN) {
        return recvTimingReqMondrian(pkt, packet_id);
    }
    else if (model_state == gem5::model::DEACT) {
        // it has 2 KiB permission lookups! This is directly taken from their
        // paper. If there are less than 16K hosts then the size of the table
        // also goes down. But we'll assume that the table is replicated 128
        // times for each process.

        // can deact do a single access to the permission? I think yes, why:
        // the number of hosts determine the metadata lookup. Then there needs
        // to be another lookup to figure out the process ID. Why shall we
        // optimize their work?
        return recvTimingReqDeACT(pkt, packet_id);
    }
    else if (model_state == gem5::model::FLAT_TABLE) {
        // the simplest implementation
        return recvTimingReqFlatTables(pkt, packet_id);
    }
    else {
        fatal("unsupported model");
    }
    return false;

}


void
FlatTables::recvReqRetry() {
    // reset the table: looks like this is the same for all the different

    switch(model_state) {
        case gem5::model::MONDRIAN: 
        case gem5::model::SPACE_CONTROL: 
        case gem5::model::FLAT_TABLE:
            // techniques
            waiting_for_cpu_retry = false;
            // regular stuff!
            while (!retry_queue.empty()) {

                uint64_t id = retry_queue.front();
                cpuSidePorts[id].sendRetryReq();
                retry_queue.pop();
                // retry_queue.unset(id);
                DPRINTF(FlatTablesDebug,
                                    "Found the retry Issue! Port %lu\n", id);
            }

            // schedule an event for the permission packets too
            if (!permission_packets.empty())
                if (!event.scheduled())
                    schedule(event, clockEdge(Cycles(1)));
            // return;
            break;

        case gem5::model::DEACT: break;

        default: fatal("unsupported model!");
    }
}

// void
// FlatTables::scheduleNewEvent() {
//     schedule(event, curTick() + 1);
// }

Addr
FlatTables::getPLBAddr(Addr addr) {
    // return the base of the permission table address + the entry.
    // XXX: This is unimplemented with the ID
    assert(false && "call getMondrianAddress()");
    return baseAddrPermissionTable;
}


Addr
FlatTables::getBinarySearchPermissionTableAddr(int attempt) {
    // return the address of the permission table by binary search without the
    // actual map. uncached entries will work perfectly.
    // XXX: Assumption: the permission table is always 1 GiB with
    // 2 less entries
    Addr table_start = 0x80;        // 128 bytes
    Addr table_end = ONE_G;    // 1 GiB

    // since we have a table size of 32 GiB, we need ~0.49 GiB of permission
    // table.
    table_end = 0x1FFFFF80; 

    // hardcoding this because i am lazy

    Addr addr = baseAddrPermissionTable + ((table_end - table_start) / (attempt + 1));

    // must always be cache-line sized 
    assert(addr % 0x40 == 0 && (addr >= baseAddrPermissionTable &&
            addr < baseAddrPermissionTable + ONE_G));

    // XXX: This is unimplemented with the ID
    return addr;
}

void
FlatTables::recvRespRetry(const PortID id) {
    waiting_for_mem_retry = false;
    memSidePort.sendRetryResp();
    DPRINTF(FlatTablesDebug, "recvRespRetry Found the issue! Retry called for %lu\n",
                                                                        id);
}

bool
FlatTables::recvTimingRespMondrian(PacketPtr pkt) {
    // check if this is a permission packet
    if (isInPermissionRange(pkt->getAddr())) {
        
        gem5::Addr originalAddr = 0x0;
        if (pkt->senderState != nullptr) {
            auto *state =
                    dynamic_cast<PermissionSenderState*>(pkt->senderState);
            if (state) {
                // PacketPtr originalPkt = state->originalPkt;
                originalAddr = state->originalAddr;

                // Now you know which memory request this permission
                // response belongs to
                // You can update your permission tracker accordingly
            }
            else
                fatal("Sender state cannot be null\n");
        }
        DPRINTF(PermissionResponses, "Got response for pkt %#x and permission"
                                    " pkt %#x and count %d\n", originalAddr,
                                                                pkt->getAddr(),
                                    permission_response_tracker[originalAddr]);

        // there are outstanding packets that were sent by the requestor
        if (permission_request_tracker[originalAddr] > 0) {
            // keep a track that the response for this packet is received.
            permission_response_tracker[originalAddr]++;
            // FIXME:
            // For caching, this matters a lot

            // make the request packet 0 or decrease by 1

            fatal_if(permission_request_tracker[originalAddr]-- == 0,
                        "There cannot be more responses than requeusts!");

            // Release the MSHR
            // if we have the total number of permission responses required,
            // then release the MSHR
            if (permission_response_tracker[originalAddr] == max_search_attempts) {
                fatal_if(mshrs_occupied-- == 0, "Cannot have -ve MSHRs!");

                // sample occupied mshr count here
                stats.maxPermissionMSHROcuppied.sample(mshrs_occupied);
            }
        }
        else {
            // received a response for a request never made?
            fatal("You should not see permission responses for requests "
                    "never made! details\n"
                    "  og addr %#x\n"
                    "  permission addr %#x\n"
                    "  req count %d\n"
                    "  resp count %d\n"
                    "  mshr count %d\n",
                    originalAddr, pkt->getAddr(),
                    permission_request_tracker[originalAddr],
                    permission_response_tracker[originalAddr],
                    mshrs_occupied);
        }
        // We don't really do anything else with the permission response!
        delete pkt;
        return true;
    }
 
    // if this is a remote memory packet then there must be a comparison
    // with the ACM
    if (isInMemoryRange(pkt->getAddr())) {
        Tick comparison_latency = 1;
        schedule(new EventFunctionWrapper([this, pkt]{ },
            name() + ".accessEvent", true),
            clockEdge(static_cast<Cycles>(comparison_latency)));

        // do we have it's corresponding permission packet?
        if (permission_response_tracker[pkt->getAddr()] > 0) {
            // at least one response has been received!
            // ++num
            // TODO: Cache it! for future generations!
            if (permission_response_tracker[pkt->getAddr()] == max_search_attempts) {
                // so, all permission stuff is done and this packet is ready to
                // be sampled
                stats.packetLatency.sample(
                                    gem5::curTick() - outstanding_packets[pkt]);
                outstanding_packets.erase(pkt);
            }
            // not all packets are here
                
        }
        else {
            // ++error_margin. just tell the memsideport to send this packet
            // again?
            // waiting_for_mem_retry = true;
            DPRINTF(PermissionResponses, "Response received before all "
                " permission packets were sent for addr %#x with count %d!"
                " -- req count %lu\n", pkt->getAddr(),
                                permission_response_tracker[pkt->getAddr()],
                                permission_request_tracker[pkt->getAddr()]);

            // push this packet into the queue until it's permission response
            // is received.

            // keep the packet but do not send it upstream
            response_packets.push(pkt);

            // we need to sample the response packet queue. we only store real
            // responses
            stats.maxStoredResponses.sample(response_packets.size());


            // what if i dont keep this and let it pass?
            // return false;

            // response_packets.push(pkt);
            if (!event.scheduled())
                // try sending this packet again
                schedule(event, clockEdge(Cycles(1)));
            return true;

            // I can't accept this packet rn but I'll create an event to call
            // resp retry -> This is the last problem in this implementation!.
        }
    }
    // DPRINTF(PermissionResponses, "Response received before all "
    //     " permission packets were sent for addr %#x with count %d!"
    //     " -- req count %lu\n", pkt->getAddr(),
    // business as usual

    // there could be packets with weird addresses (maybe instructions)
    PacketId id = pkt->id;
    return cpuSidePorts[portMap[id]].sendTimingResp(pkt);
}

bool
FlatTables::recvTimingRespDeACT(PacketPtr pkt) {
    assert(false && "Not implemented error!\n");
    return false;
}

bool
FlatTables::recvTimingRespSpaceControl(PacketPtr pkt) {
    // check if this is a permission packet
    if (isInPermissionRange(pkt->getAddr())) {
        
        gem5::Addr originalAddr = 0x0;
        if (pkt->senderState != nullptr) {
            auto *state =
                    dynamic_cast<PermissionSenderState*>(pkt->senderState);
            if (state) {
                // PacketPtr originalPkt = state->originalPkt;
                originalAddr = state->originalAddr;

                // Now you know which memory request this permission
                // response belongs to
                // You can update your permission tracker accordingly
            }
            else
                fatal("Sender state cannot be null\n");
        }
        DPRINTF(PermissionResponses, "Got response for pkt %#x and permission"
                                    " pkt %#x and count %d\n", originalAddr,
                                                                pkt->getAddr(),
                                    permission_response_tracker[originalAddr]);

        // there are outstanding packets that were sent by the requestor
        if (permission_request_tracker[originalAddr] > 0) {
            // keep a track that the response for this packet is received.
            permission_response_tracker[originalAddr]++;
            // FIXME:
            // For caching, this matters a lot
            if (useDedicatedCaching) {
                assert(false && "not implemented yet!\n");
                // store the permission packet in the cache. there will be a
                // cache access
                ++stats.numPermissionTableAccesses;

                // see if there is space
                if (doesCacheHaveSpace()) {
                    // get the permission entry's 64 byte length address
                    addCacheEntry(maskAddr(pkt->getAddr()));
                    // there needs to be a cache lookup time implemented for
                    // accurate modeling
                    
                }
                else {
                    // replace an entry based on the policy and store the cache
                    // line
                    assert(replaceEntry(pkt->getAddr()));
                }
            }

            // make the request packet 0 or decrease by 1

            fatal_if(permission_request_tracker[originalAddr]-- == 0,
                        "There cannot be more responses than requeusts!");

            // Release the MSHR
            // if we have the total number of permission responses required,
            // then release the MSHR
            if (permission_response_tracker[originalAddr] == max_search_attempts) {
                fatal_if(mshrs_occupied-- == 0, "Cannot have -ve MSHRs!");

                // sample occupied mshr count here
                stats.maxPermissionMSHROcuppied.sample(mshrs_occupied);
            }
        }
        else {
            // received a response for a request never made?
            fatal("You should not see permission responses for requests "
                    "never made! details\n"
                    "  og addr %#x\n"
                    "  permission addr %#x\n"
                    "  req count %d\n"
                    "  resp count %d\n"
                    "  mshr count %d\n",
                    originalAddr, pkt->getAddr(),
                    permission_request_tracker[originalAddr],
                    permission_response_tracker[originalAddr],
                    mshrs_occupied);
        }
        // We don't really do anything else with the permission response!
        delete pkt;
        return true;
    }
 
    // if this is a remote memory packet then there must be a comparison
    // with the ACM
    if (isInRemoteRange(pkt->getAddr())) {
        Tick comparison_latency = 1;
        schedule(new EventFunctionWrapper([this, pkt]{ },
            name() + ".accessEvent", true),
            clockEdge(static_cast<Cycles>(comparison_latency)));

        // do we have it's corresponding permission packet?
        if (permission_response_tracker[pkt->getAddr()] > 0) {
            // at least one response has been received!
            // ++num
            // TODO: Cache it! for future generations!
            if (permission_response_tracker[pkt->getAddr()] == max_search_attempts) {
                // so, all permission stuff is done and this packet is ready to
                // be sampled
                stats.packetLatency.sample(
                                    gem5::curTick() - outstanding_packets[pkt]);
                outstanding_packets.erase(pkt);
            }
            // not all packets are here
                
        }
        else {
            // ++error_margin. just tell the memsideport to send this packet
            // again?
            // waiting_for_mem_retry = true;
            DPRINTF(PermissionResponses, "Response received before all "
                " permission packets were sent for addr %#x with count %d!"
                " -- req count %lu\n", pkt->getAddr(),
                                permission_response_tracker[pkt->getAddr()],
                                permission_request_tracker[pkt->getAddr()]);

            // push this packet into the queue until it's permission response
            // is received.

            // keep the packet but do not send it upstream

            // for signaling drain, we need to keep a track of outstanding
            // real packets
            inflight_packets--;
            response_packets.push(pkt);

            // we need to sample the response packet queue. we only store real
            // responses
            stats.maxStoredResponses.sample(response_packets.size());


            // what if i dont keep this and let it pass?
            // return false;

            // response_packets.push(pkt);
            if (!event.scheduled())
                // try sending this packet again
                schedule(event, clockEdge(Cycles(1)));
            return true;

            // I can't accept this packet rn but I'll create an event to call
            // resp retry -> This is the last problem in this implementation!.
        }
    }
    // DPRINTF(PermissionResponses, "Response received before all "
    //     " permission packets were sent for addr %#x with count %d!"
    //     " -- req count %lu\n", pkt->getAddr(),
    // business as usual

    // for signaling drain, we need to keep a track of outstanding
    // real packets
    inflight_packets--;
    // there could be packets with weird addresses DMA :)
    PacketId id = pkt->id;
    return cpuSidePorts[portMap[id]].sendTimingResp(pkt);
}

bool
FlatTables::recvTimingRespFT(PacketPtr pkt) {
    // check if this is a permission packet
    if (isInPermissionRange(pkt->getAddr())) {
        
        gem5::Addr originalAddr = 0x0;
        if (pkt->senderState != nullptr) {
            auto *state =
                    dynamic_cast<PermissionSenderState*>(pkt->senderState);
            if (state) {
                // PacketPtr originalPkt = state->originalPkt;
                originalAddr = state->originalAddr;

                // Now you know which memory request this permission
                // response belongs to
                // You can update your permission tracker accordingly
            }
            else
                fatal("Sender state cannot be null\n");
        }
        DPRINTF(PermissionResponses, "Got response for pkt %#x and permission"
                                    " pkt %#x and count %d\n", originalAddr,
                                                                pkt->getAddr(),
                                    permission_response_tracker[originalAddr]);

        // there are outstanding packets that were sent by the requestor
        if (permission_request_tracker[originalAddr] > 0) {
            // keep a track that the response for this packet is received.
            permission_response_tracker[originalAddr]++;
            // FIXME:
            // For caching, this matters a lot

            // do we need to reset the permission_response_tracker?

            // so sample the buffering time for stats.
            stats.stallTime.sample(gem5::curTick() - stall_time[pkt]);
            stall_time.erase(pkt);

            // make the request packet 0 or decrease by 1
            fatal_if(permission_request_tracker[originalAddr]-- == 0,
                        "There cannot be more responses than requeusts!");
            // Release the MSHR
            fatal_if(mshrs_occupied-- == 0, "Cannot have -ve MSHRs!");

            // sample occupied mshr count here
            stats.maxPermissionMSHROcuppied.sample(mshrs_occupied);
        }
        else {
            // received a response for a request never made?
            fatal("You should not see permission responses for requests "
                    "never made! details\n"
                    "  og addr %#x\n"
                    "  permission addr %#x\n"
                    "  req count %d\n"
                    "  resp count %d\n"
                    "  mshr count %d\n",
                    originalAddr, pkt->getAddr(),
                    permission_request_tracker[originalAddr],
                    permission_response_tracker[originalAddr],
                    mshrs_occupied);
        }
        // We don't really do anything else with the permission response!
        delete pkt;
        return true;
    }
 
    // if this is a remote memory packet then there must be a comparison
    // with the ACM
    if (isInRemoteRange(pkt->getAddr())) {
        Tick comparison_latency = 1;
        schedule(new EventFunctionWrapper([this, pkt]{ },
            name() + ".accessEvent", true),
            clockEdge(static_cast<Cycles>(comparison_latency)));

        // do we have it's corresponding permission packet?
        if (permission_response_tracker[pkt->getAddr()] > 0) {
            // at least one response has been received!
            // ++num
            // TODO: Cache it! for future generations!
            // so, all permission stuff is done and this packet is ready to
            // be sampled
            stats.packetLatency.sample(
                                gem5::curTick() - outstanding_packets[pkt]);
            outstanding_packets.erase(pkt);

            // if this address has enough responses, then you need to decrease
            // the response count
            // otherwise, this is an infinite cache
            fatal_if(permission_response_tracker[pkt->getAddr()]-- < 0,
                        "cannot have more responses than requests!\n");
        }
        else {
            // ++error_margin. just tell the memsideport to send this packet
            // again?
            // waiting_for_mem_retry = true;
            DPRINTF(PermissionResponses, "Response received before all "
                " permission packets were sent for addr %#x with count %d!"
                " -- req count %lu\n", pkt->getAddr(),
                                permission_response_tracker[pkt->getAddr()],
                                permission_request_tracker[pkt->getAddr()]);

            // push this packet into the queue until it's permission response
            // is received.

            // keep the packet but do not send it upstream
            response_packets.push(pkt);
            // keep the current time to track how long the packet was buffered
            stall_time[pkt] = gem5::curTick();
            // we need to sample the response packet queue. we only store real
            // responses
            stats.maxStoredResponses.sample(response_packets.size());


            // what if i dont keep this and let it pass?
            // return false;

            // response_packets.push(pkt);
            if (!event.scheduled())
                // try sending this packet again
                schedule(event, clockEdge(Cycles(1)));
            return true;

            // I can't accept this packet rn but I'll create an event to call
            // resp retry -> This is the last problem in this implementation!.
        }
    }
    // DPRINTF(PermissionResponses, "Response received before all "
    //     " permission packets were sent for addr %#x with count %d!"
    //     " -- req count %lu\n", pkt->getAddr(),
    // business as usual
    PacketId id = pkt->id;
    return cpuSidePorts[portMap[id]].sendTimingResp(pkt);
}

bool
FlatTables::recvTimingResp(PacketPtr pkt) {
    /// this needs to be modular
    if (model_state == gem5::model::SPACE_CONTROL) {
        return recvTimingRespSpaceControl(pkt);

    }
    else if (model_state == gem5::model::MONDRIAN) {
        return recvTimingRespMondrian(pkt);
    }
    else if (model_state == gem5::model::DEACT) {

    }
    else if (model_state == gem5::model::FLAT_TABLE) {
        return recvTimingRespFT(pkt);
    }
    else {
        fatal("unsupported model");
        return false;
    }
    assert(false && "unreachable code\n");
    return false;
}

void
FlatTables::recvRangeChange() {
    for (auto p : cpuSidePorts)
        p.sendRangeChange();
}

Port&
FlatTables::getPort(const std::string &if_name, PortID idx) {
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
FlatTables::startup() {
    // do nothing!
}

// void
// FlatTables::printErrorStats() {
//     DPRINTF("Printing last known values\n"
//             "  og addr %#x\n"
//             "  permission addr %#x\n"
//             "  req count %d\n"
//             "  resp count %d\n"
//             "  mshr count %d\n",
//             originalAddr, pkt->getAddr(),
//             permission_request_tracker[originalAddr],
//             permission_response_tracker[originalAddr],
//             mshrs_occupied);
// }

// ------------------------------- caches ---------------------------------- //

bool
FlatTables::isCached(gem5::Addr masked_addr) {
    auto entry = cache_map.find(masked_addr);
    if (entry == cache_map.end())
        // not found!
        return false;
    // cache hit!
    ++stats.numPermissionTableCacheHits;
    return true;
}
bool
FlatTables::replaceEntry(gem5::Addr masked_addr) {
    if (cache_policy == "lru") {
        // find the entry with the minimum 
        gem5::Tick oldest = gem5::MaxTick;
        gem5::Addr key = 0;
        for (auto &it : cache_map) {
            if (cache_map[it.first].last_accessed < oldest) {
                key = it.first;
                oldest = cache_map[it.first].last_accessed;
            }
        }
        // replace the oldest entry
        cache_map.erase(key);
        number_of_entries--;
        addCacheEntry(masked_addr);
    }
    else if (cache_policy == "mru") {
        // find the entry with the minimum 
        gem5::Tick youngest = 0;
        gem5::Addr key = 0;
        for (auto &it : cache_map) {
            if (cache_map[it.first].last_accessed > youngest) {
                key = it.first;
                youngest = cache_map[it.first].last_accessed;
            }
        }
        // replace the oldest entry
        cache_map.erase(key);
        number_of_entries--;
        addCacheEntry(masked_addr);
    }
    else if (cache_policy == "counter") {
        // this replacement policy kicks out the least used entry from the
        // table
        gem5::Tick least_used = 0;
        gem5::Addr key = 0;
        for (auto &it : cache_map) {
            if (cache_map[it.first].access_count > least_used) {
                key = it.first;
                least_used = cache_map[it.first].access_count;
            }
        }
        // replace the oldest entry
        cache_map.erase(key);
        number_of_entries--;
        addCacheEntry(masked_addr);
    }
    else {
        fatal("unknown caching policy\n");
        return false;
    }
    return true;
}


FlatTables::StatGroup::StatGroup(statistics::Group *parent)
    : statistics::Group(parent),
    ADD_STAT(numIncomingCPUSidePackets, statistics::units::Count::get(),
        "Number of LLC incoming packets"),
    ADD_STAT(numOutgoingMemSidePackets, statistics::units::Count::get(),
        "Count the number of outgoing memory packets"),
    ADD_STAT(numOutgoingPermissionPackets, statistics::units::Count::get(),
        "Count the number of permission packets"),
    ADD_STAT(numPermissionTableEntries, statistics::units::Count::get(),
        "Number of entries in the permission table"),
    ADD_STAT(numPermissionTableCacheHits, statistics::units::Count::get(),
        "Number of hits in the permission table cache"),
    ADD_STAT(numPermissionTableAccesses, statistics::units::Count::get(),
        "total number of accesses into the permission table (redundant!)"),
    ADD_STAT(maxPermissionMSHROcuppied, statistics::units::Count::get(),
            "Histogram of the occupied MSHRs for permissions."),
    ADD_STAT(packetLatency, statistics::units::Count::get(),
            "Histogram of the latency incurred for permission lookups"),
    ADD_STAT(stallTime, statistics::units::Count::get(),
            "Histogram of stalling latency.")
{
    using namespace statistics;
    // Initialize any histogram stats here
    maxPermissionMSHROcuppied
        .init(2)
        .flags(pdf);
    packetLatency
        .init(2)
        .flags(pdf);
    maxStoredResponses
        .init(2)
        .flags(pdf);
    stallTime
        .init(2)
        .flags(pdf);
}


} // namespace gem5
