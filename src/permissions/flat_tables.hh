/*
 * Copyright (c) 2025 Regents of the University of California
 * All rights reserved.
 * 
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

#ifndef __PERMISSIONS_FLAT_TABLES_HH__
#define __PERMISSIONS_FLAT_TABLES_HH__

#include <cmath>
#include <queue>
#include <map>

#include "sim/eventq.hh"
#include "base/statistics.hh"
#include "base/trace.hh"
#include "mem/port.hh"

#include "params/FlatTables.hh"

#include "sim/clocked_object.hh"
#include "sim/sim_object.hh"
#include "sim/stats.hh"
#include "sim/sim_exit.hh"

#include "permissions/permission_cache.hh"
#include "permissions/meta.hh"

namespace gem5
{
/**
 * This Clocked SimObject is exteded from ClockedObject and 
 * FlatTablesTable. The goal of this class to implement a specific
 * flat-table implementations like border-control or DeACT.
 *
 * This SimObject is created from the simple memory object that can have
 * multiple requests coming in and going out with and without additional
 * latency. This extra latency is added for the permission checks against a
 * given memory address. The SimObject will be extended (TODO) to cache some of
 * these requests.
 *
 * Features:
 * A single domain MMP with multiple segments.
 * Each segment can be cached in the PLB.
 *
 *
 * Limitations:
 * This class DOES not create a memory request and sends it to the memory to
 * implement the lookup. Instead it schedules the lookup and PLB miss times
 * as a new event in gem5.
 *
 * The original Mondrian paper doesn't do any of the stuff they mention in
 * their paper. They "create" permission tables from a trace with marked mmaped
 * instructions.
 *
 * In this version of the code, the segment sizes are fixed to 4 KiB.
 *
 * We are annotating mmaps to create the permission table. The
 * creation latency is ignored but the lookup latency is added to every memory
 * request.
 */


// FIXME:
// We need a template for a queue and a set. There can be multiple entries in
// the template for a given address to figure out where is the ID.

class FlatTables : public ClockedObject
{
    // Using boiler-plate code for the initialization part.
    /**
     * Port on the CPU-side that receives requests.
     * Mostly just forwards requests to the owner.
     * Part of a vector of ports. One for each CPU port (e.g., data, inst)
     */
    private:
        class CPUSidePort : public ResponsePort
        {
            private:
                // need a pointer to the owner
                FlatTables &owner;
                // Need to maintain the packet_id to keep a track of where
                // to respond back for a packet.
                uint64_t packet_id;

            public:
                CPUSidePort(const std::string& name_,
                            PortID id_,
                            FlatTables &owner_,
                            uint64_t packet_id_) : ResponsePort(name_, id_),
                                                    owner(owner_),
                                                    packet_id(packet_id_)
                { }
            protected:
                AddrRangeList getAddrRanges() const override {
                    return owner.getAddrRanges();
                }

                Tick recvAtomic(PacketPtr pkt) override {
                    return owner.recvAtomic(pkt);
                }
                void recvFunctional(PacketPtr pkt) override {
                    owner.recvFunctional(pkt);
                }

                bool recvTimingReq(PacketPtr pkt) override {
                    return owner.recvTimingReq(pkt, packet_id);
                }

                void recvRespRetry() override {
                    owner.recvRespRetry(id);
                }
        };
        class MemSidePort: public RequestPort
        {
            /**
             * Port on the memory-side that receives responses.
             * Mostly just forwards requests to the owner
             */
            private:
                FlatTables &owner;
            public:
                MemSidePort(const std::string& name_,
                            FlatTables &owner_) : RequestPort(name_),
                                                        owner(owner_)
                { }

            protected:
                bool recvTimingResp(PacketPtr pkt) override {
                    return owner.recvTimingResp(pkt);
                }

                void recvReqRetry() override {
                    owner.recvReqRetry();
                }

                void recvRangeChange() override {
                    owner.recvRangeChange();
                }
        };
    private:
    


        // Instantiation of the memory port
        MemSidePort memSidePort;
        // Instantiation of the cpu side ports.
        std::vector<CPUSidePort> cpuSidePorts;

        // the permission model to simulate
        std::string modelName;
        // just need to maintain the state.
        int model_state; 

        // To enable or disable permission checks. If this is not set, this
        // SimObject is a simple packet forwarder.
        bool enablePermissionCheck;

        // To enable system caches, we need to provide another option to the
        // user
        bool useDedicatedCaching;
        // The base of the permission table must be a valid memory address.
        Addr baseAddrPermissionTable;

        unsigned int numberOfEntries;

        bool binarySearch;

        unsigned int permissionEntrySize;

        // make sure to use these variables for the MMP lookup
        Tick creationLatency;
        // The hit Latency is for the PLb cache hit.
        Tick hitLatency;
        // The miss goes down to the memory. Ideally, we need to create a
        // packet that accesses this memory.
        Tick missLatency;

        // TODO:
        // In the future, we need to have packet creator here.

        // Here are some of the other variables that we need the user to define

        // We may need the total memory size as well :(
        uint64_t totalMemorySize;
        // Size of the cache. The table is calculated as the total size of the
        // memory
        int cacheSize;
        // The segment size is defined by the user. We simulate everyrhing with
        // a fixed segment.
        int segmentSize;
        std::string cachePolicy;

        unsigned int mshrCount;
        unsigned int mshrs_occupied;

        // table math
        int hostID;
        uint64_t table_size;

        // We need a couple of more variables to keep a track of
        // total_cached_entries and the maximum number of cached entiers possi
        // ble to be stored in the cache.
        uint64_t total_cached_entries;
        // We need to know that max cached entry size to make sure that the
        // eviction policies are kicking in.
        uint64_t max_cached_entries;
        // We need to know the max cached entries so that the variable for a
        // single lookup is correctly implemented as a log2(N).
        uint64_t permission_table_entries;

        // We need a couple of masks to lookup the cache and permissions
        // efficiently
        uint64_t segment_mask;
        // uint64_t cache_mask;

        // We need a  variable for the total number of enteies
        uint64_t total_entries;

        // for lookup numberof packets
        int max_search_attempts;

        // PLb specific values are here.
        int permission_block_size;
        int permission_cmd;

        // keep a permissions checker to ensure that permissions aren't sent
        // more than once for the same address.
        std::unordered_map<gem5::Addr, int> permission_checker;

        std::unordered_map<gem5::PacketPtr, gem5::Tick> outstanding_packets;

        // std::unordered_map<gem5::Addr, bool> permission_packet_tracker;


        // we need a class variable for the additional latency until we find a
        // way to pass method parameters
        Tick class_latency;

        // an infinite queue that stores all the incoming packets and it's
        // corresponding permission packets.
        std::queue<gem5::PacketPtr> permission_packets;
        std::queue<gem5::PacketPtr> response_packets;
        // std::queue<gem5::PacketPtr> failedPermissionPackets;
        // std::unordered_map<gem5::Addr, int> permission_packet_tracker;
        std::unordered_map<gem5::Addr, bool> have_i_seen_this; 

        // for every address, we need to keep a track of whether we have sent
        // all the permission packets or not.
        std::unordered_map<gem5::Addr, int> permission_request_tracker;
        std::unordered_map<gem5::Addr, int> permission_response_tracker;

        // For the ports to work correctly
        std::unordered_map<PacketId, uint64_t> portMap;
        // CustomQueue<uint64_t> retry_queue;
        SimpleQueue<uint64_t> retry_queue;
        // OriginalQueue<uint64_t> retry_queue;

        // For the response port
        AddrRangeList getAddrRanges() const;
        Tick recvAtomic(PacketPtr pkt);
        void recvFunctional(PacketPtr pkt);
        bool recvTimingReq(PacketPtr pkt, uint64_t packet_id);
        void recvRespRetry(const PortID id);

        // for the individual implementations
        bool recvTimingReqMondrian(PacketPtr pkt, uint64_t packet_id);
        bool recvTimingReqDeACT(PacketPtr pkt, uint64_t packet_id);
        bool recvTimingReqSpaceControl(PacketPtr pkt, uint64_t packet_id);
        bool recvTimingReqFlatTables(PacketPtr pkt, uint64_t packet_id);

        bool recvTimingRespMondrian(PacketPtr pkt);
        bool recvTimingRespDeACT(PacketPtr pkt);
        bool recvTimingRespSpaceControl(PacketPtr pkt);
        bool recvTimingRespFT(PacketPtr pkt);

        // For the request port
        bool recvTimingResp(PacketPtr pkt);
        void recvReqRetry();
        void recvRangeChange();

        bool waitingForMemRetry = false;

        // gem5::EventWrapper delayEvent;
        // The permission table needs to schedule events
        void processEvent();
        // void processEvent(int attempt);
        void processPermissionRequest();
        void processPendingResponse();

        // void scheduleNewEvent();
        // This event is responsible for queueing the permission lookup and
        // creation latency
        // EventWrapper<FlatTables, &FlatTables::processEvent> event;
        EventFunctionWrapper event;

        // std::queue<gem5::PacketPtr> packets;


        // Do we owe the CPU a retry right now?
        bool waitingForCpuRetry = false;

        Addr getFlatTableAddress(Addr addr);
        Addr getDeACTAddress(Addr addr);
        Addr getMondrianAddress(Addr addr);

        // Event to notify the CPU to retry later
        // EventFunctionWrapper cpuRetryEvent;

        // Helper to schedule cpuSidePort.sendRetryReq()
        // void scheduleCpuRetry();

        // void printErrorStats();

        inline bool isInPermissionRange(gem5::Addr addr) {
            return (addr >= baseAddrPermissionTable &&
                addr < baseAddrPermissionTable + 0x40000000) ? true : false;
        }

        inline bool isInRemoteRange(gem5::Addr addr) {
            return (addr >= 0x100000000 &&
                        addr < totalMemorySize + 0x100000000) ? true : false;
        }

        inline bool isInMemoryRange(gem5::Addr addr) {
            // a method that is needed to for mondrian. this is true for the
            // system memory ranges.
            return ((addr >= 0x0 && addr < 0xC0000000) || 
                                        isInRemoteRange(addr)) ? true : false;
        }

    public:

        // send permission packets
        bool sendPermissionPackets(PacketPtr pkt);

        // To implement the new packet stuff, here are the mothods
        Addr getPLBAddr(Addr addr);
        // To implement binary lookup i nthe worst case scenario.
        Addr getBinarySearchPermissionTableAddr(int attempt);

        // We need dual port stats for verification and results.
        struct StatGroup : public statistics::Group
        {
            StatGroup(statistics::Group *parent);
            /** Count the number of incoming LLC packets */
            statistics::Scalar numIncomingCPUSidePackets;

            /** Count the number of outgoing memory packets, including both
             * real and permission packets and the local memory */
            statistics::Scalar numOutgoingMemSidePackets;

            // since gem5 already counts the number of memory packets per
            // memory object, we don't need to maintain the number of memory
            // requests for each memory type.

            /** Count the number of permission packets. These are synthetic
             * packets that the permission table object sends.
             */
            statistics::Scalar numOutgoingPermissionPackets;

            /** Number of entries in the permission table */
            statistics::Scalar numPermissionTableEntries;

            /** Number of hits in the permission table cache */
            statistics::Scalar numPermissionTableCacheHits;

            /** total number of accesses into the permission table
             * (redundant!) */
            statistics::Scalar numPermissionTableAccesses;

            // max number of MSHRs occupied needs to be studied as a histogram
            statistics::Histogram maxPermissionMSHROcuppied;

            // we need to keep a track of max number of responses that are
            // stored in the checker.
            statistics::Histogram maxStoredResponses;

            // /** Count the number of incoming read packets */
            // statistics::Scalar numReadIncomingPackets;

            // /** Count the number of incoming write packets */
            // statistics::Scalar numWriteIncomingPackets;

            // /** Create a histogram of the latencies of packets sent via this
            // port*/
            statistics::Histogram packetLatency;

            // /** Create a histogram of the total outstanding packets */
            // statistics::Histogram outstandingPackets;
        } stats;

    public:
        // class constructor
        FlatTables(const FlatTablesParams &params);
        void startup() override;
        // void init() override;
        Port& getPort(const std::string &if_name, PortID idx) override;

};      // class FlatTables

} // namespace
#endif  // __NEW_CLOCKED_PERMISSION_HH__