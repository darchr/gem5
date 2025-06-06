/*
 * Copyright (c) 2017 Jason Lowe-Power
 * All rights reserved.
 *
 * Copyright (c) 2025 Regents of the University of California
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

#ifndef __NEW_SIMPLE_BLOCKING_PORT_HH__
#define __NEW_SIMPLE_BLOCKING_PORT_HH__

#include <queue>
#include <map>

#include "sim/eventq.hh"
#include "base/statistics.hh"
#include "base/trace.hh"
#include "mem/port.hh"
#include "params/SimpleBlockingPort.hh"
#include "sim/sim_object.hh"
#include "sim/stats.hh"
#include "sim/sim_exit.hh"

namespace gem5
{
/**
 * This SimObject is created from the simple memory object that can have
 * multiple requests coming in and going out with and without additional
 * latency. This extra latency is added for the permission checks against a
 * given memory address. The SimObject will be extended (TODO) to cache some of
 * these requests.
 *
 * Features:
 *
 *
 * Limitations:
 * The only thing that is missing is the extra memory access to access the
 * permission table. This can be done using a simple traffic generator that
 * accesses the permission table without the OS getting involved.
 *
 * The original Mondrian paper doesn't do any of the stuff they mention in
 * their paper. They "create" permission tables from a trace with marked mmaped
 * instructions. We are annotating mmaps to create the permission table. The
 * creation latency is ignored but the lookup latency is added to every memory
 * request.
 */

/**
 * A very simple memory object. Current implementation doesn't even cache
 * anything it just forwards requests and responses.
 * This memobj is fully blocking (not non-blocking). Only a single request can
 * be outstanding at a time.
 */
class SimpleBlockingPort : public SimObject
{
  private:

    /**
     * Port on the CPU-side that receives requests.
     * Mostly just forwards requests to the owner.
     * Part of a vector of ports. One for each CPU port (e.g., data, inst)
     */
    class CPUSidePort : public ResponsePort
    {
      private:
        /// The object that owns this object (SimpleMemobj)
        SimpleBlockingPort *owner;

        // Instead of keeping a single blockedPacket, we keep a map of
        // blockedPackets.
        /// If we tried to send a packet and it was blocked, store it here

      public:
        /**
         * Constructor. Just calls the superclass constructor.
         */
        CPUSidePort(const std::string& name, SimpleBlockingPort *owner) :
            ResponsePort(name), owner(owner)
        {
            // TODO
            // Ideally we also want to initialize a traffic generator object
            // that creates memory packets going to the memory to read the
            // permission table.
        }

        /**
         * Send a packet across this port. This is called by the owner and
         * all of the flow control is hanled in this function.
         *
         * @param packet to send.
         */
        // void sendPacket(PacketPtr pkt);

        /**
         * Get a list of the non-overlapping address ranges the owner is
         * responsible for. All response ports must override this function
         * and return a populated list with at least one item.
         *
         * @return a list of ranges responded to
         */
        AddrRangeList getAddrRanges() const override;

        std::queue<PacketPtr> blockedRequests;
        std::unordered_map<gem5::Addr, gem5::PacketPtr> requestMap;
        /**
         * Send a retry to the peer port only if it is needed. This is called
         * from the SimpleMemobj whenever it is unblocked.
         */
        // void trySendRetry();

      protected:
        /**
         * Receive an atomic request packet from the request port.
         * No need to implement in this simple memobj.
         */
        Tick recvAtomic(PacketPtr pkt) override;

        /**
         * Receive a functional request packet from the request port.
         * Performs a "debug" access updating/reading the data in place.
         *
         * @param packet the requestor sent.
         */
        void recvFunctional(PacketPtr pkt) override;

        /**
         * Receive a timing request from the request port.
         *
         * @param the packet that the requestor sent
         * @return whether this object can consume the packet. If false, we
         *         will call sendRetry() when we can try to receive this
         *         request again.
         */
        bool recvTimingReq(PacketPtr pkt) override;

        /**
         * Called by the request port if sendTimingResp was called on this
         * response port (causing recvTimingResp to be called on the request
         * port) and was unsuccesful.
         */
        void recvRespRetry() override;
    };

    /**
     * Port on the memory-side that receives responses.
     * Mostly just forwards requests to the owner
     */
    class MemSidePort : public RequestPort
    {
      private:
        /// The object that owns this object (SimpleMemobj)
        SimpleBlockingPort *owner;

        // Make sure that this is a queue so that the SimObject is
        // non-blocking.
        /// If we tried to send a packet and it was blocked, store it here

      public:
        /**
         * Constructor. Just calls the superclass constructor.
         */
        MemSidePort(const std::string& name, SimpleBlockingPort *owner) :
            RequestPort(name), owner(owner)
        {
            //
            // TODO
            // Add host_id for the disaggregated memory changes.
        }

        /**
         * Send a packet across this port. This is called by the owner and
         * all of the flow control is hanled in this function.
         *
         * @param packet to send.
         */
        // void sendPacket(PacketPtr pkt);

        std::queue<PacketPtr> blockedResponses;
        std::unordered_map<gem5::Addr, gem5::PacketPtr> responseMap;
      protected:
        /**
         * Receive a timing response from the response port.
         */
        bool recvTimingResp(PacketPtr pkt) override;

        /**
         * Called by the response port if sendTimingReq was called on this
         * request port (causing recvTimingReq to be called on the responder
         * port) and was unsuccesful.
         */
        void recvReqRetry() override;

        /**
         * Called to receive an address range change from the peer responder
         * port. The default implementation ignores the change and does
         * nothing. Override this function in a derived class if the owner
         * needs to be aware of the address ranges, e.g. in an
         * interconnect component like a bus.
         */
        void recvRangeChange() override;
    };

    // void trySendRetry();

    /**
     * Handle a packet functionally. Update the data on a write and get the
     * data on a read.
     *
     * @param packet to functionally handle
     */
    void handleFunctional(PacketPtr pkt);

    /**
     * Handle a packet atomically. Update the data on a write and get the
     * data on a read.
     *
     * @param packet to functionally handle
     */
    Tick handleAtomic(PacketPtr pkt);
    /**
     * Return the address ranges this memobj is responsible for. Just use the
     * same as the next upper level of the hierarchy.
     *
     * @return the address ranges this memobj is responsible for
     */
    AddrRangeList getAddrRanges() const;

    /**
     * Tell the CPU side to ask for our memory ranges.
     */
    void sendRangeChange();

    // The permission table needs to schedule events
    void processEvent();

    void scheduleNewEvent();
    // This event is responsible for queueing the permission lookup and
    // creation latency
    EventWrapper<SimpleBlockingPort, &SimpleBlockingPort::processEvent> event;
    // We need dual port stats for verification and results.
    struct StatGroup : public statistics::Group
    {
        StatGroup(statistics::Group *parent);
        /** Count the number of incoming LLC packets */
        statistics::Scalar numIncomingCPUSidePackets;

        /** Count the number of outgoing memory packets */
        statistics::Scalar numOutgoingMemSidePackets;

        /** Count the number of traffic packets */
        statistics::Scalar numOutgoingTrafficPackets;

        /** Number of entries in the permission table */
        statistics::Scalar numPermissionTableEntries;

        /** Number of hits in the permission table cache */
        statistics::Scalar numPermissionTableCacheHits;

        /** total number of accesses into the permission table (redundant!) */
        statistics::Scalar numPermissionTableAccesses;

        // /** Count the number of incoming read packets */
        // statistics::Scalar numReadIncomingPackets;

        // /** Count the number of incoming write packets */
        // statistics::Scalar numWriteIncomingPackets;

        // /** Create a histogram of the latencies of packets sent via this port*/
        // statistics::Histogram packetLatency;

        // /** Create a histogram of the total outstanding packets */
        // statistics::Histogram outstandingPackets;
    } stats;
    // Instantiation of the CPU-side ports. Unlike the instruction and data
    // ports of the original SimObject, we are only interested in a single
    // port. Is this a bottle-neck? Yes, but this only connects the L3 cache
    // to the membus
    //
    CPUSidePort cpuSidePort;
    // TODO
    // We'd also need a traffic generator port that will start creating traffic
    // in the memory.

    /// Instantiation of the memory-side port
    MemSidePort memSidePort;

    // To enable or disable permission checks
    bool enablePermissionCheck;

    /// True if this is currently blocked waiting for a response.
    // bool blocked;

    // make sure to use these variables for the MMP lookup
    gem5::Tick creationLatency;
    gem5::Tick hitLatency;
    gem5::Tick missLatency;

    // we need a class variable for the additional latency until we find a way
    // to pass method parameters
    gem5::Tick class_latency;

    // We may need the total memory size as well :(
    uint64_t totalMemorySize;
    // Size of the cache. The table is calculated as the total size of the
    // memory
    int cacheSize;
    // The segment size is defined by the user. We simulate everyrhing with
    // a fixed segment.
    int segmentSize;

    // Each entry in the MMP permission will have these values. There are
    // implementational details.
    struct cache_entry_vector {
      bool is_cached;
      gem5::Tick last_accessed;
      int access_count;
    };

    // We need a couple of more variables to keep a track of
    // total_cached_entries and the maximum number of cached entiers possible
    // to be stored in the cache.
    uint64_t total_cached_entries;
    uint64_t max_cached_entries;

    // We need a  variable for the total number of enteies
    uint64_t total_entries;
    std::string cachePolicy;

    std::map<gem5::Addr, cache_entry_vector*> permission_table;

    // for non-blocking memory requests. these objects must be of the owner




    /**
     * We need a caching implementation and policies. there can be multiple
     * latencies: lookup if there is an MMP entry (new address)
     *            create the entry if needed
     *            lookup in the cache (hit or miss)
     */
    gem5::Tick isCachedRequest(gem5::Addr addr);
    gem5::Tick simpleLRU(gem5::Addr addr);
    gem5::Tick simpleMRU(gem5::Addr addr);
    gem5::Tick simpleRandom(gem5::Addr addr);


  public:

    /** constructor
     */
    SimpleBlockingPort(const SimpleBlockingPortParams &params);
    void startup() override;

    /**
     * Get a port with a given name and index. This is used at
     * binding time and returns a reference to a protocol-agnostic
     * port.
     *
     * @param if_name Port name
     * @param idx Index in the case of a VectorPort
     *
     * @return A reference to the given port
     */
    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;


};

} // namespace gem5

#endif // __NEW_SIMPLE_BLOCKING_PORT_HH__
