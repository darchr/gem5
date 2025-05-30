

#ifndef __NEW_DUAL_PORT_HH__
#define __NEW_DUAL_PORT_HH__

#include <queue>

#include "mem/port.hh"
#include "params/DualPort.hh"
#include "sim/sim_object.hh"

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
// Copyright (c) 2025 The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met: redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer;
// redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution;
// neither the name of the copyright holders nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/**
 * A very simple memory object. Current implementation doesn't even cache
 * anything it just forwards requests and responses.
 * This memobj is fully blocking (not non-blocking). Only a single request can
 * be outstanding at a time.
 */
class DualPort : public SimObject
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
        DualPort *owner;

        /// True if the port needs to send a retry req.
        bool needRetry;

        // Instead of keeping a single blockedPacket, we keep a map of
        // blockedPackets.
        /// If we tried to send a packet and it was blocked, store it here
        std::queue<PacketPtr> blockedPackets;
        // PacketPtr blockedPacket;

      public:
        /**
         * Constructor. Just calls the superclass constructor.
         */
        CPUSidePort(const std::string& name, DualPort *owner) :
            ResponsePort(name), owner(owner), needRetry(false)
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
        void sendPacket(PacketPtr pkt);

        /**
         * Get a list of the non-overlapping address ranges the owner is
         * responsible for. All response ports must override this function
         * and return a populated list with at least one item.
         *
         * @return a list of ranges responded to
         */
        AddrRangeList getAddrRanges() const override;

        /**
         * Send a retry to the peer port only if it is needed. This is called
         * from the SimpleMemobj whenever it is unblocked.
         */
        void trySendRetry();

      protected:
        /**
         * Receive an atomic request packet from the request port.
         * No need to implement in this simple memobj.
         */
        Tick recvAtomic(PacketPtr pkt) override
        { panic("recvAtomic unimpl."); }

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
        DualPort *owner;

        // Make sure that this is a queue so that the SimObject is
        // non-blocking.
        /// If we tried to send a packet and it was blocked, store it here
        std::queue<PacketPtr> blockedPackets;

      public:
        /**
         * Constructor. Just calls the superclass constructor.
         */
        MemSidePort(const std::string& name, DualPort *owner) :
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
        void sendPacket(PacketPtr pkt);

      protected:
        /**
         * Receive a timing response from the response port.
         */
        bool recvTimingResp(PacketPtr pkt) override;

        bool isBlocked();
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

    /**
     * Handle the request from the CPU side
     *
     * @param requesting packet
     * @return true if we can handle the request this cycle, false if the
     *         requestor needs to retry later
     */
    bool handleRequest(PacketPtr pkt);

    /**
     * Handle the respone from the memory side
     *
     * @param responding packet
     * @return true if we can handle the response this cycle, false if the
     *         responder needs to retry later
     */
    bool handleResponse(PacketPtr pkt);

    /**
     * Handle a packet functionally. Update the data on a write and get the
     * data on a read.
     *
     * @param packet to functionally handle
     */
    void handleFunctional(PacketPtr pkt);

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

    /// True if this is currently blocked waiting for a response.
    bool blocked;

  public:

    /** constructor
     */
    DualPort(const DualPortParams &params);

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

#endif // __NEW_DUAL_PORT_HH__
