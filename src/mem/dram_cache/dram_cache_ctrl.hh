/* Copyright (c) 2016 Mark D. Hill and David A. Wood
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
 *
 * Authors: Jason Lowe-Power
 */

#ifndef __MEM_DRAM_CACHE_DRAM_CACHE_CTRL_HH__
#define __MEM_DRAM_CACHE_DRAM_CACHE_CTRL_HH__

#include <queue>
#include <set>
#include <vector>

#include "base/statistics.hh"
#include "mem/dram_cache/dirty_list.hh"
#include "mem/dram_cache/dram_cache_storage.hh"
#include "mem/mem_object.hh"
#include "params/DRAMCacheCtrl.hh"
#include "sim/system.hh"

class DRAMCtrl;

/**

 */
class DRAMCacheCtrl : public MemObject
{
  public:
    typedef DRAMCacheCtrlParams Params;

    /**
     * Constructor based on the Python params
     *
     * @param params Python parameters
     */
    DRAMCacheCtrl(Params* params);

    void init() override;

    BaseMasterPort& getMasterPort(const std::string& if_name,
                                  PortID idx = InvalidPortID) override;

    BaseSlavePort& getSlavePort(const std::string& if_name,
                                PortID idx = InvalidPortID) override;

    void regStats() override;

    /**
     * This function is called from the storage controllers.
     * It is called once the storage controller is done with the access.
     * If it was a hit then the pkt now has valid data and just needs a
     * response. If it was a miss, then we need to forward the packet to
     * memory.
     * Note: this function is only called for reads. Writes always succeed.
     *
     * @param pkt the packet which initiated this request.
     * @param hit whether or not the extra tag check in the storage controller
     *            was a hit or miss.
     */
    virtual void recvStorageResponse(PacketPtr pkt, bool hit);

    /**
     * This function is called from the storage controllers.
     * This function must be called before recvStorageResponse to make sure
     * that the memory side is free if it is needed.
     *
     * @param pkt the packet which initiated this request.
     * @param hit whether or not the extra tag check in the storage controller
     *            was a hit or miss.
     * @param dirty whether the block is dirity (the pkt member isn't set yet)
     * @return true if we can receive the request, false if it needs to be
     *         sent later.
     */
    virtual bool canRecvStorageResp(PacketPtr pkt, bool hit, bool dirty);

    /**
     * Tries to clean a line. This is called from the dirty list when it is
     * trying to clean a line.
     *
     * @param block_addr address to clean. Only used for the line number so
     *                   it doesn't have to be in the cache.
     * @return true if the replace was sent, false otherwise
     */
    bool startCleaningReplace(Addr block_addr);

    /**
     * Cleans a region atomically
     *
     * @param start_addr the address to start cleaning
     * @param size the size of the region to clean
     */
    void atomicClean(Addr start_addr, int regionSize);

    /**
     * Storage has a slot available, so we can retry any blocked packets
     */
    void retryStorageReq();

    DrainState drain() override;

  protected:
    /**

     */
    class MemSideMasterPort : public MasterPort
    {
        DRAMCacheCtrl& cache;
        bool waitingForRetry;

      public:
        MemSideMasterPort(const std::string& name, DRAMCacheCtrl &cache) :
            MasterPort(name, &cache), cache(cache), waitingForRetry(false)
        { }

      protected:

        bool recvTimingResp(PacketPtr pkt) override
            { return cache.recvTimingResp(pkt); }

        void recvReqRetry() override
            { cache.recvReqRetry(); }

      public:
        void trySendRetry() {
            if (waitingForRetry) {
                waitingForRetry = false;
                sendRetryResp();
            }
        }

        void setWaitingForRetry()
            { assert(!waitingForRetry); waitingForRetry = true; }
    };

    virtual bool recvTimingResp(PacketPtr pkt);

    void recvReqRetry();

    /**
     ********************************************************************
     */
    class CacheSlavePort : public QueuedSlavePort
    {
        DRAMCacheCtrl& cache;
        RespPacketQueue queue;
        bool waitingForRetry;

      public:

        CacheSlavePort(const std::string& name, DRAMCacheCtrl &cache) :
            QueuedSlavePort(name, &cache, queue), cache(cache),
            queue(cache, *this), waitingForRetry(false)
        { }

      protected:

        void recvFunctional(PacketPtr pkt) override
            { cache.recvFunctional(pkt); }

        Tick recvAtomic(PacketPtr pkt) override
            { return cache.recvAtomic(pkt); }

        bool recvTimingReq(PacketPtr pkt) override
            { return cache.recvTimingReq(pkt); }

        AddrRangeList getAddrRanges() const override
            { return cache.getAddrRanges(); }

      public:
        void trySendRetry() {
            if (waitingForRetry) {
                waitingForRetry = false;
                sendRetryReq();
            }
        }

        void setWaitingForRetry()
            // Note: for invalidates it's possible this is already stalled
            { waitingForRetry = true; }

        /// Used for when you get an invalidate but the mem port is blocked
        void unsetWaitingForRetry()
            { assert(waitingForRetry); waitingForRetry = false; }
    };

    void recvFunctional(PacketPtr pkt);

    Tick recvAtomic(PacketPtr pkt);

    /**
     * Receive a packet from the cache side (slavePort) and take the needed
     * actions. Note: This function is also used to drain racy accesses for
     * invalidation addresses if using backprobes,
     *
     * @param pkt that we're receiveing
     * @param from_cache true if we're processing a request as normal from the
     *        cache. False if we're processing a request for the queued up racy
     *        probes and we should not request a retry from the cache port
     */
    virtual bool recvTimingReq(PacketPtr pkt, bool from_cache = true);

    AddrRangeList getAddrRanges() const
        { return addrRanges; }

    void recvRespRetry();

    /// This is for cheating to keep inclusion between the on-chip cache and
    /// the DRAM cache. This will send invalidates to the on-chip cache and
    /// tracks some other things.
    class InvalidationPort : public QueuedMasterPort
    {
        DRAMCacheCtrl& cache;
        ReqPacketQueue queue;
        SnoopRespPacketQueue snoopRespQueue;
        bool waitingForRetry;

      public:
        InvalidationPort(const std::string& name, DRAMCacheCtrl &cache) :
            QueuedMasterPort(name, &cache, queue, snoopRespQueue),
            cache(cache), queue(cache, *this), snoopRespQueue(cache, *this),
            waitingForRetry(false)
        { }

      protected:

        bool recvTimingResp(PacketPtr pkt) override
            { return cache.recvInvResp(pkt); }

      public:
        void trySendRetry() {
            if (waitingForRetry) {
                waitingForRetry = false;
                sendRetryResp();
            }
        }

        void setWaitingForRetry()
        { assert(!waitingForRetry); waitingForRetry = true; }

        bool isSnooping() const override { return true; }

        void recvTimingSnoopReq(PacketPtr pkt) override
        { cache.recvInvSnoop(pkt); }

    };

    bool recvInvResp(PacketPtr pkt);

    void recvInvSnoop(PacketPtr pkt);

    /**
     * This event is created when we recieve a timing request.
     */
    class HandleRequestEvent : public Event
    {
      private:
        DRAMCacheCtrl* ctrl;
        PacketPtr pkt;
      public:
        HandleRequestEvent(DRAMCacheCtrl* ctrl, PacketPtr pkt)
            : Event(EventBase::Default_Pri, EventBase::AutoDelete),
              ctrl(ctrl), pkt(pkt)
        { }
        void process() override {
            if (pkt->isRead()) {
                ctrl->handleRead(pkt);
            } else {
                ctrl->handleWrite(pkt);
            }
        }
    };

    /**
     * Event wrapper for sending blocked packets to storage
     */
    class TrySendStorageBlockedEvent : public Event
    {
      private:
        DRAMCacheCtrl* ctrl;
      public:
        TrySendStorageBlockedEvent(DRAMCacheCtrl* ctrl)
            : Event(), ctrl(ctrl)
            { }
        void process() override {
            ctrl->trySendStorageBlockedHead();
        }
    };

    /**
     * Wrapper event to drain the blocked packet queues.
     * This event will continue to reschedule itself until the queue is
     * empty.
     */
    class DrainBlockedPacketsEvent : public Event
    {
      private:
        DRAMCacheCtrl *ctrl;
        unsigned lineNumber;
        // This will force a copy of the contents
        std::deque<PacketPtr> packetQueue;
      public:
        DrainBlockedPacketsEvent(DRAMCacheCtrl *ctrl, unsigned line_no,
                                 std::deque<PacketPtr> queue) :
            ctrl(ctrl), lineNumber(line_no), packetQueue(queue)
            { assert(!queue.empty()); }

        void process() override;
    };

    /**
     * Small wrapper so we can save the original packet on replacements
     */
    class ReplaceSenderState : public Packet::SenderState
    {
      public:
        PacketPtr pkt;
        Addr origAddr;
        // True if this was started because of a clean replace
        bool cleanReplace;
        ReplaceSenderState(PacketPtr pkt, Addr orig_addr, bool clean_replace) :
            pkt(pkt), origAddr(orig_addr), cleanReplace(clean_replace)
        { }
    };

    Addr blockAlign(Addr addr) const
        { return (addr & ~(Addr(blockSize - 1))); }

    /**
     * Return the address in the storage DRAM for the given physical addr
     */
    Addr getStorageAddr(Addr block_addr) {
        // Put this back into a block aligned address
        return getLineNumber(block_addr) * blockSize;
    }

    /**
     * Get the global line number across all of the controllers.
     * This is used for blocking things on replacements.
     */
    unsigned getLineNumber(Addr block_addr) {
        // Global cache line number
        Addr line_no = block_addr / blockSize / banks;
        // Line number across all banks of the cache
        return line_no % bankLines;
    }

    /**
     * Check if there is an outstanding replacement for the line this packet
     * maps to. If there is a replacement, then queue this request.
     *
     * @param pkt the packet that we are processing
     * @return true if we blocked the request (do not process further)
     */
    bool checkBlockForReplacement(PacketPtr pkt);

    /**
     * Actually process the incoming read. This is called from events to
     * decouple it from the initial processing. See HandleRequestEvent.
     * This simply sends the read to the storage
     *
     * @param pkt The packet that is incoming
     */
    virtual void handleRead(PacketPtr pkt);

    /**
     * Actually process the incoming write. This is called from events to
     * decouple it from the initial processing. See HandleRequestEvent.
     * This actually does the dirty list check
     * (functionally) and then forwards the packet onto the next stage.
     * If the line is dirty in the cache, this will start the replacment
     * process, otherwise it calls handleWriteback to send the writeback on.
     *
     * @param pkt The packet that is incoming
     */
    virtual void handleWrite(PacketPtr pkt);

    /**
     * Send the writeback to the storage (and possibly write it through too).
     * This also updates the dirty list
     *
     * @param pkt the requesting packet
     * @param mark_dirty if false, then don't mark this line as dirty in the
     *                   dirty list, even if it seems like we should
     * @return true if something was sent to memory, false otherwise.
     *         Note: Used for tracking outstanding reqeusts
     */
    bool handleWriteback(PacketPtr pkt, bool mark_dirty = true);

    /**
     * Send a replace request to the storage for this packet.
     * This function constructs a new packet and updates the blockedForRepl.
     * structure then sends the new packet to memory.
     */
    void sendReplace(PacketPtr pkt);

    /**
     * Actually send the request. This assumes that the port is NOT blocked.
     * There is one corner case where it's ok for it to be blocked, but that
     * can only occur for a single pair of packets. No more than two!
     * If sending fails, the packet is stored until it can be sent.
     *
     * @param pkt the requesting packet
     * @return true on success
     */
    bool sendMemSideReq(PacketPtr pkt);

    /**
     * Try to send the packet on the storage port. If it fails, then queue
     * it to be sent later. Otherwise, decrement it from its queue.
     *
     * @param pkt the requesting packet
     */
    void trySendStorageReq(PacketPtr pkt);

    /**
     * Actually send the request. This assumes that the port is NOT blocked.
     *
     * @param pkt the requesting packet
     * @param storage_addr the actual address in the storage controller
     * @return true on success
     */
    bool sendStorageReq(PacketPtr pkt, Addr storage_addr);

    /**
     * Try to send the head of the queued packets
     */
    void trySendStorageBlockedHead();

    /**
     * Decrement the current number of packets in the buffers
     */
    void decrementOutstandingQueue() {
        outstandingRequestQueueSize--;
        if (outstandingRequestQueueSize == 127) {
            slavePort.trySendRetry();
        }
        assert(outstandingRequestQueueSize >= 0);
    }

    /**
     * Unblock this line that was blocked because of a replace
     * This also kicks off the events to empty the queue of any requests that
     * were blocked because of this request
     * This removed the line from the blockedForReplacement, but not
     * blockedPackets (IIRC...)
     *
     * @param block_addr the line address that finished an unblock
     */
    void unblockReplace(Addr block_addr);

    /**
     * Returns whether or not we should write through for a given request
     * This is mostly based off of the policy (writethrough vs writeback)
     * but for the adaptive policy this does some kind of logic
     *
     * @param addr the address that we're trying to decide on
     * @return true if the pkt should be written through to memory
     */
    bool shouldWriteThrough(Addr addr);

    /**
     * Drain any packets that were waiting for memory after checking dirty list
     */
    void drainWritebacksWaitingForMem();

    bool canInsertDL(Addr addr) {
        if (dirtyList) {
            return dirtyList->canInsert(addr);
        } else {
            // Assume that without a DL, we can always mark as dirty.
            return true;
        }
    }

    /**
     * Send backprobe to the on-chip caches.
     *
     * @return if true, then no need to send a dirty writeback. There was a
     *         writeback dirty in the stalled queue and we're sending that.
     */
    bool sendBackprobe(Addr addr);
    void sendAtomicBackprobe(Addr addr);

    System *system;

    MemSideMasterPort memSidePort;
    CacheSlavePort slavePort;
    InvalidationPort invPort;

    const AddrRangeList addrRanges;

    // Use multiple master IDs for different kinds of requests so it makes
    // it easier to track them.
    // The demand ID is used for writebacks from the cache and hit lookups
    MasterID demandID;
    // The other ID is used for inserts and lookups for dirty replacements
    MasterID otherID;

    AbstractDirtyList* dirtyList;

    // This is just a lower-level interface than the ports that I used before
    DRAMCacheStorage* storageCtrl;

    // This is the writeback policy for the cache. If it's write-through
    // then we don't actually need a dirty list (everything is always clean)
    // Otherwise, we need to check the dirty list on writes and (maybe) on
    // read misses.
    Enums::WritebackPolicy writebackPolicy;

    // If this is true, then we need to send backprobes ot the on-chip caches
    // every time we evict a line. This is used in BEAR to keep the DCP
    // up-to-date and in KNL to keep the modified-inclusive property
    bool sendBackprobes;

    // Maxiumum number of oustanding requests to track (this just for storage
    // requests, and mostly used for replacements)
    int maxOutstanding;

    // For tracking blocked packets when sending requests/responses
    bool memSideBlocked;
    // This stores the packet pointer while port is blocked
    PacketPtr memSideBlockedPkt;
    // This extra variable is needed for the (rare) case that two slots are
    // needed. This happens for replacements + misses, sometimes.
    PacketPtr memSideBlockedPkt2;

    // For solving the two packet problem above.
    EventWrapper<DRAMCacheCtrl, &DRAMCacheCtrl::recvReqRetry>
        retrySecondBlockedPacketEvent;

    // Stores packets that need to do a writeback after the dirty list check
    // but were stalled because the memory side was blocked
    std::queue<PacketPtr> writebacksWaitingForMem;

    // Event is scheduled on a recvRetry from memory when there are
    // more than one packets in the waiting queue and one is popped off
    EventWrapper<DRAMCacheCtrl, &DRAMCacheCtrl::drainWritebacksWaitingForMem>
        drainWritebacksWaitingForMemEvent;

    bool storageCtrlBlocked;
    // These store the packet pointers while port is blocked
    std::queue<PacketPtr> waitingForStorage;

    // True when the storage is waiting for an unblock request
    bool storageNeedsUnblock;

    // Event is scheduled on a recvRetry from storage when there are
    // more than one packets in the waiting queue and one is popped off
    TrySendStorageBlockedEvent trySendStorageBlockedEvent;

    const uint64_t size;
    const unsigned banks;

    unsigned blockSize;
    unsigned bankLines;

    const Cycles dirtyListLatency;

    // Tracks the total number of packets that are at the controller
    // This is the number of buffers that we need for outstanding requests
    // This is incrememented when a request enters the controller, then it
    // is decremented when a write is sent to the storage, when a read is
    // sent to the memory, or when a read response comes back from storage
    // and it is a hit.
    int outstandingRequestQueueSize;

    // Each line number that is blocked is added to this set until it is done
    // with the writeback portion of the replacement
    std::set<unsigned> blockedForReplacement;

    // Put packets here (tagged on the line num) if they are conflicting with
    // a currently outstanding replacement
    std::map<unsigned, std::deque<PacketPtr> > blockedPackets;

    // If true, we need send a retry request to the dirty list once we've
    // finished a replacement
    bool retryCleaningReplacement;

    // For tracking how warm the cache is
    int coldMissesInterval;
    int missesInterval;

    // For tracking when the time we last had a memory block
    Cycles memSideBlockedCycle;

    // What bank number is this. used to know what address offset we are
    // expecting
    int bankNumber;

    // A queue for internal requests that need to be processed before any
    // external requests. I hope this is OK to do and won't cause deadlocks
    std::queue<PacketPtr> internalQueuedPackets;

    // For tracking outstanding invalidates so we don't try to respond with
    // data if it is an on-chip cache miss.
    std::map<Addr, PacketPtr> outstandingInvalidates;
    // This tracks requests that we receive when a conflicting probe is
    // currently outstanding. replay these packets when the probe is complete.
    std::map<Addr, PacketPtr> racyProbes;

    // This tracks the things in the cache that are no longer in the DRAM cache
    // It's like a fake directory. Note: I think this may become inconsistent
    // if there are no clean writebacks.
    std::unordered_set<Addr> fakeDirectory;

    // This holds the packets that requested data as we are getting the data
    // from memory. This happens on invalidates since it's what KNL does.
    std::map<Addr, PacketPtr> outstandingRefillForUpgrades;

    Stats::Scalar memSideBlockedCount;
    Stats::Scalar storageBlockedCount;
    Stats::Scalar replacementBlockedCount;
    Stats::Scalar outstandingQueueBlockedCount;
    Stats::Scalar responseBlockedCount;

    Stats::Scalar readPackets;
    Stats::Scalar cleanWBPackets;
    Stats::Scalar dirtyWBPackets;

    Stats::Scalar cleaningReplaces;
    Stats::Scalar outstandingQueueBlockedCleaningCount;
    Stats::Scalar cleaningReplaceBlockedForReplacement;
    Stats::Scalar cleaningReplaceDirty;
    Stats::Scalar cleaningReplaceWasted;

    Stats::Scalar missMapReadMiss;
    Stats::Scalar missMapReadHit;
    Stats::Scalar missMapWriteMiss;
    Stats::Scalar missMapWriteHit;

    Stats::Scalar dirtyListReadHit;
    Stats::Scalar dirtyListReadMiss;
    Stats::Scalar dirtyListWriteHit;
    Stats::Scalar dirtyListWriteMiss;

    Stats::Scalar realReadMiss;
    Stats::Scalar realReadHit;
    Stats::Scalar realReplaceMiss;
    Stats::Scalar realReplaceColdMiss;
    Stats::Scalar realReplaceHit;
    Stats::Scalar realReplaceCleanHit;

    Stats::Scalar writeThroughCount;
    Stats::Scalar dirtyListFull;

    Stats::Scalar adaptiveWritebackRead;
    Stats::Scalar adaptiveWritebackBlocked;
    Stats::Scalar adaptiveWritebackRecentBlocked;
    Stats::Scalar adaptiveWritebackWrite;

    Stats::Scalar storageRead;
    Stats::Scalar storageReadForRepl;
    Stats::Scalar storageCleanWrite;
    Stats::Scalar storageDirtyWrite;

    Stats::Scalar couldClean;
    Stats::Scalar proactiveCleans;

    Stats::Scalar cleanSinks;

    Stats::Scalar probes;
    Stats::Scalar uselessProbes;
    Stats::Scalar numRacyProbes;
    Stats::Scalar numRacyProbeWriteback;
    Stats::Scalar refillOnUpgrade;

    // These are for debugging purposes only. This writes all of the data
    // we've seen into a map and checks it on every read.
    bool checkData;
    std::map<Addr, uint8_t*> dataCheck;
    void updateCanonicalData(PacketPtr pkt);
    void checkCanonicalData(PacketPtr pkt);

    DRAMCtrl* dramctrl;

};

#endif
