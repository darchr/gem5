#include "permissions/flat_tables.hh"


#include "debug/FlatPermissionTables.hh"
#include "debug/PermissionTableEvent.hh"
#include "debug/FlatTablesDebug.hh"
#include "debug/PermissionPackets.hh"
#include "debug/RemoteAddress.hh"
#include "debug/PermissionResponses.hh"

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
    remoteMemoryStart(params.remote_memory_start),
    totalMemorySize(params.total_memory_size),
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
    if (model_state == gem5::model::MONDRIAN)
        panic_if(segmentSize == 0, "Cannot simulate mondrian with segment"
                            " size set to 0\n");

    
    // extending the PLB model, we need to implement the number of lookups
    // needed to fetch an entry from the permission table via binary
    // search

    panic_if(permissionEntrySize == 0, "Permission entry cannot be zero!");

    // this becomes relevant if simulating flat tables or worst cases for
    // mondrain or space-control.
    total_entries = totalMemorySize / segmentSize;

    // the user can override the max attempts providing a number_of_entries.
    if (numberOfEntries != 0) {
        warn("number_of_entries is not 0!, overriding the number of entries!");
        total_entries = numberOfEntries;
        fatal_if(model_state == gem5::model::FLAT_TABLE || 
                            model_state == gem5::model::DEACT,
                "cannot have number of entries specified for flat tables\n");
    }

    DPRINTF(FlatPermissionTables,
            "MMP table has %lu entries\n", total_entries);

    // All plb packets will be of size 64 bytes. This will always be 64.
    permission_block_size = 64;

    // FIXME:
    // All plb packets will be of READ type.
    permission_cmd = MemCmd::ReadReq;

    // warn the user that this will be ignored for flat tables
    if (model_state == gem5::model::FLAT_TABLE) {
        max_search_attempts = 1;
    }
    else if (model_state == gem5::model::DEACT) {
        // each memory request cannot be more than 64 bytes.
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
    segment_mask = 0xFFFFFFFF & !(segmentSize - 1);

    // cache_mask = 0xFFFFFFF0;

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

    DPRINTF(FlatPermissionTables, "MMP cache has %lu entries\n",
                                                        max_cached_entries);
    
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
        table_size = 0x40000000;
    }
    else if (model_state == gem5::model::SPACE_CONTROL) {
        // similar to MONDRIAN
        // the table is fixed -> just 1. the entries vary.
        table_size = 0x40000000;
    }

    // cache_policy = cachePolicy;
    // total_cache_size = cacheSize;
    // permission_cache_line_size = 64;
    // number_of_entries = total_cache_size / permission_cache_line_size;


    // auto permission_cache = new gem5::PermissionCache();

}

AddrRangeList
FlatTables::getAddrRanges() const
{
    return memSidePort.getAddrRanges();
}

Tick
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
FlatTables::processEvent() {
    // To have a non-infinite cache for flat tables, we need a version with
    // process events.
    DPRINTF(PermissionPackets, "Items to process %lu, current state %d\n",
                                permission_packets.size(), waitingForCpuRetry);
    if (!permission_packets.empty()) {
        processPermissionRequest();
    }
    // If I am witing on a permission response, then try rescheduling the
    // resp packet!
    if (!response_packets.empty())
        processPendingResponse();
    //     // recvRespRetry(0);
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
                waitingForMemRetry = true;
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
//     if (!waitingForCpuRetry) {
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
//             waitingForCpuRetry = true;

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
    if (!waitingForCpuRetry) {
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
            waitingForCpuRetry = true;

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
    Addr permission_addr = baseAddrPermissionTable + (hostID * table_size) +
                                                (addr / 0xFFF);
    // I don't want any unforseen consequences
    assert(isInPermissionRange(permission_addr));
    return permission_addr;
    // We assume that N * 512 MiB is reserved fo

    // each flat table is of some size
    // return baseAddrPermissionTable + hostID * (processID * tableSize));
    // return baseAddrPermissionTable;
    
}

Addr
FlatTables::getDeACTAddress(Addr addr) {
    assert(false && "not impleemnted error");
    return baseAddrPermissionTable;
}

Addr
FlatTables::getMondrianAddress(Addr addr) {
    return baseAddrPermissionTable;
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

            // if (!waitingForCpuRetry) {
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
        bool to_process = (!waitingForCpuRetry);

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
        waitingForCpuRetry = true;
        retry_queue.push(packet_id);
        return false;
    }
    fatal("should be unreachable");
    return false;
}
bool
FlatTables::recvTimingReqDeACT(PacketPtr pkt, uint64_t packet_id) {
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
    //     waitingForCpuRetry = true;
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

            // if (!waitingForCpuRetry) {
            Addr permission_addr = getFlatTableAddress(pkt->getAddr());
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
        if (!waitingForCpuRetry) {
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
        waitingForCpuRetry = true;
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

            // if (!waitingForCpuRetry) {
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
        waitingForCpuRetry = true;
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
            waitingForCpuRetry = false;
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
    return baseAddrPermissionTable;
}


Addr
FlatTables::getBinarySearchPermissionTableAddr(int attempt) {
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
FlatTables::recvRespRetry(const PortID id) {
    waitingForMemRetry = false;
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
            // waitingForMemRetry = true;
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
            // waitingForMemRetry = true;
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
            // waitingForMemRetry = true;
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
