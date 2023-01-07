/// The copyright needs be modified for UCD/DArchR/the names of the writers

/*
 * Copyright (c) 2012-2020 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2013 Amin Farmahini-Farahani
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

/**
 * @file
 * DcacheCtrl declaration
 */

#ifndef __DCACHE_CTRL_HH__
#define __DCACHE_CTRL_HH__

#include <queue>

#include "mem/mem_ctrl.hh"
#include "params/DcacheCtrl.hh"

class DRAMInterface;
class NVMInterface;

class DcacheCtrl : public QoS::MemCtrl
{
  private:

   bool stallRds = false;
   bool drainDramWrite = false;
   bool drainNvmWrite = false;


   unsigned maxConf = 0,
   maxDrRdEv = 0, maxDrRdRespEv = 0,
   maxDrWrEv = 0,
   maxNvRdIssEv = 0, maxNvRdEv = 0, maxNvRdRespEv = 0,
   maxNvWrEv = 0;

   unsigned numDirtyLinesInDrRdRespQ = 0;

    // For now, make use of a queued response port to avoid dealing with
    // flow control for the responses being sent back
    class MemoryPort : public QueuedResponsePort
    {
        RespPacketQueue queue;
        DcacheCtrl& ctrl;
      public:
        MemoryPort(const std::string& name, DcacheCtrl& _ctrl);
      protected:
        Tick recvAtomic(PacketPtr pkt) override;
        Tick recvAtomicBackdoor(
                PacketPtr pkt, MemBackdoorPtr &backdoor) override;
        void recvFunctional(PacketPtr pkt) override;
        bool recvTimingReq(PacketPtr) override;
        AddrRangeList getAddrRanges() const override;
    };


    /**
     * Our incoming port, for a multi-ported controller add a crossbar
     * in front of it
     */
    MemoryPort port;

    /**
     * Remember if the memory system is in timing mode
     */
    bool isTimingMode;

    /**
     * Remember if we have to retry a request when available.
     */
    bool retry;

    void printORB();
    void printCRB();
    void printAddrInitRead();
    void printAddrDramRespReady();
    void printNvmWritebackQueue();
    Addr returnTagDC(Addr pkt_addr, unsigned size);
    Addr returnIndexDC(Addr pkt_addr, unsigned size);

    template <class Q>
    void clearQueue(Q & q) {
        q = Q();
    }

    /**
     * Bunch of things requires to setup "events" in gem5
     * When event "respondEvent" occurs for example, the method
     * processRespondEvent is called; no parameters are allowed
     * in these methods
     */
    void processNextReqEvent();
    EventFunctionWrapper nextReqEvent;

    void processRespondEvent();
    EventFunctionWrapper respondEvent;

    /**
     * processDramReadEvent() is an event handler which
     * schedules the initial DRAM read accesses for every
     * received packet by the DRAM Cache Controller.
    */
    void processDramReadEvent();
    EventFunctionWrapper dramReadEvent;

    /**
     * processRespDramReadEvent() is an event handler which
     * handles the responses of the initial DRAM read accesses
     * for the received packets by the DRAM Cache Controller.
    */
    void processRespDramReadEvent();
    EventFunctionWrapper respDramReadEvent;

    /**
     * processWaitingToIssueNvmReadEvent() is an event handler which
     * handles the satte in which the packets that missed in DRAM cache
     * will wait before being issued, if the NVM read has reached to the
     * maximum number allowed for pending reads.
    */
    void processWaitingToIssueNvmReadEvent();
    EventFunctionWrapper waitingToIssueNvmReadEvent;

    /**
     * processNvmReadEvent() is an event handler which
     * schedules the NVM read accesses in the DRAM Cache Controller.
    */
    void processNvmReadEvent();
    EventFunctionWrapper nvmReadEvent;

    /**
     * processRespNvmReadEvent() is an event handler which
     * handles the responses of the NVM read accesses in
     * the DRAM Cache Controller.
    */
    void processRespNvmReadEvent();
    EventFunctionWrapper respNvmReadEvent;

    /**
     * processOverallWriteEvent() is an event handler which
     * handles all write accesses to DRAM and NVM.
    */

    void processOverallWriteEvent();
    EventFunctionWrapper overallWriteEvent;

    /**
     * Actually do the burst based on media specific access function.
     * Update bus statistics when complete.
     *
     * @param mem_pkt The memory packet created from the outside world pkt
     * returns cmd_at tick
     */
    Tick doBurstAccess(MemPacket* mem_pkt);

    /**
     * When a packet reaches its "readyTime" in the response Q,
     * use the "access()" method in AbstractMemory to actually
     * create the response packet, and send it back to the outside
     * world requestor.
     *
     * @param pkt The packet from the outside world
     * @param static_latency Static latency to add before sending the packet
     */
    void accessAndRespond(PacketPtr pkt, Tick static_latency, bool in_dram);

    /**
     * Determine if there is a packet that can issue.
     *
     * @param pkt The packet to evaluate
     */
    bool packetReady(MemPacket* pkt);

    /**
     * Calculate the minimum delay used when scheduling a read-to-write
     * transision.
     * @param return minimum delay
     */
    Tick minReadToWriteDataGap();

    /**
     * Calculate the minimum delay used when scheduling a write-to-read
     * transision.
     * @param return minimum delay
     */
    Tick minWriteToReadDataGap();


    /**
     * The memory schduler/arbiter - picks which request needs to
     * go next, based on the specified policy such as FCFS or FR-FCFS
     * and moves it to the head of the queue.
     * Prioritizes accesses to the same rank as previous burst unless
     * controller is switching command type.
     *
     * @param queue Queued requests to consider
     * @param extra_col_delay Any extra delay due to a read/write switch
     * @return an iterator to the selected packet, else queue.end()
     */
    MemPacketQueue::iterator chooseNext(MemPacketQueue& queue,
        Tick extra_col_delay, bool is_dram);

    /**
     * For FR-FCFS policy reorder the read/write queue depending on row buffer
     * hits and earliest bursts available in memory
     *
     * @param queue Queued requests to consider
     * @param extra_col_delay Any extra delay due to a read/write switch
     * @return an iterator to the selected packet, else queue.end()
     */
    MemPacketQueue::iterator chooseNextFRFCFS(MemPacketQueue& queue,
            Tick extra_col_delay, bool is_dram);

    /**
     * Calculate burst window aligned tick
     *
     * @param cmd_tick Initial tick of command
     * @return burst window aligned tick
     */
    Tick getBurstWindow(Tick cmd_tick);

    /**
     * Burst-align an address.
     *
     * @param addr The potentially unaligned address
     * @param is_dram Does this packet access DRAM?
     *
     * @return An address aligned to a memory burst
     */
    Addr burstAlign(Addr addr, bool is_dram) const;

    /**
     * To avoid iterating over the outstanding requests buffer
     *  to check for overlapping transactions, maintain a set
     * of burst addresses that are currently queued.
     * Since we merge writes to the same location we never
     * have more than one address to the same burst address.
     */
    std::unordered_set<Addr> isInWriteQueue;

    struct tagMetaStoreEntry {
      // DRAM cache related metadata
      Addr tagDC;
      Addr indexDC;
      // constant to indicate that the cache line is valid
      bool validLine = false;
      // constant to indicate that the cache line is dirty
      bool dirtyLine = false;
      Addr nvmAddr;
    };

    /** A storage to keep the tag and metadata for the
     * DRAM Cache entries.
     */
    std::vector<tagMetaStoreEntry> tagMetadataStore;

    /** Different states a packet can transition from one
     * to the other while it's process in the DRAM Cache
     * Controller.
     */
    enum reqState { dramRead, dramWrite,
                    waitingToIssueNvmRead, nvmRead, nvmWrite};

    /**
     * A class for the entries of the
     * outstanding request buffer.
     */
    class reqBufferEntry {
      public:
      bool validEntry;
      Tick arrivalTick;

      // DRAM cache related metadata
      Addr tagDC;
      Addr indexDC;

      // pointer to the outside world (ow) packet received from llc
      const PacketPtr owPkt;
      // pointer to the dram cache controller (dcc) packet
      MemPacket* dccPkt;

      reqState state;
      bool isHit;
      bool conflict;

      Addr dirtyLineAddr;
      bool handleDirtyLine;

      Tick drRd;
      Tick drWr;
      Tick nvWait;
      Tick nvRd;
      Tick nvWr;

      Tick nvmIssueReadyTime;

      // Tick dramRdCmdAt;
      // Tick dramWrCmdAt;
      // Tick nvmRdCmdAt;
      // Tick nvmWrCmdAt;

      Tick dramRdDevTime;
      Tick dramWrDevTime;
      Tick nvmRdDevTime;
      //Tick nvmWrDevTime;

      reqBufferEntry(
        bool _validEntry, Tick _arrivalTick,
        Addr _tagDC, Addr _indexDC,
        PacketPtr _owPkt, MemPacket* _dccPkt,
        reqState _state, bool _isHit, bool _conflict,
        Addr _dirtyLineAddr, bool _handleDirtyLine,
        Tick _drRd, Tick _drWr, Tick _nvWait, Tick _nvRd, Tick _nvWr,
        Tick _nvmIssueReadyTime,
        Tick _dramRdDevTime, Tick _dramWrDevTime, Tick _nvmRdDevTime)
      :
      validEntry(_validEntry), arrivalTick(_arrivalTick),
      tagDC(_tagDC), indexDC(_indexDC),
      owPkt( _owPkt), dccPkt(_dccPkt),
      state(_state), isHit(_isHit), conflict(_conflict),
      dirtyLineAddr(_dirtyLineAddr), handleDirtyLine(_handleDirtyLine),
      drRd(_drRd), drWr(_drWr),
      nvWait(_nvWait), nvRd(_nvRd), nvWr(_nvWr),
      nvmIssueReadyTime(_nvmIssueReadyTime),
      dramRdDevTime(_dramRdDevTime), dramWrDevTime(_dramWrDevTime),
      nvmRdDevTime( _nvmRdDevTime)
      { }
    };

    /**
     * This is the outstanding request buffer data
     * structure, the main DS within the DRAM Cache
     * Controller. The key is the address, for each key
     * the map returns a reqBufferEntry which maintains
     * the entire info related to that address while it's
     * been processed in the DRAM Cache controller.
     */
    std::map<Addr,reqBufferEntry*> reqBuffer;


    typedef std::pair<Tick, PacketPtr> confReqBufferPair;
    /**
     * This is the second important data structure
     * within the Dram Cache controller which hold
     * received packets that had conflict with some
     * other address(s) in the DRAM Cache that they
     * are still under process in the controller.
     * Once thoes addresses are finished processing,
     * confReqBufferPair is consulted to see if any
     * packet can be moved into outstanding request
     * buffer and start processing in the DRAM Cache
     * controller.
     */
    std::vector<confReqBufferPair> confReqBuffer;

    /**
     * To avoid iterating over the outstanding requests
     * buffer for dramReadEvent handler, we maintain the
     * required addresses in a fifo queue.
     */
    std::deque <Addr> addrInitRead;
    // std::vector <MemPacket*> pktInitRead;
    // MemPacketQueue pktInitRead;
    std::vector<MemPacketQueue> pktDramRead;

    /**
     * To avoid iterating over the outstanding requests
     * buffer for respDramReadEvent handler, we maintain the
     * required addresses in a fifo queue.
     */
    std::deque <Addr> addrDramRespReady;

    //priority queue ordered by earliest tick
    typedef std::pair<Tick, Addr> addrNvmReadPair;

    /**
     * To maintain the packets missed in DRAM cache and
     * now require to read NVM, this queue holds them in order,
     * incase they can't be issued due to reaching to the maximum
     * pending number of reads for NVM.
     */
    std::priority_queue<addrNvmReadPair, std::vector<addrNvmReadPair>,
            std::greater<addrNvmReadPair> > addrWaitingToIssueNvmRead;
    std::vector<MemPacketQueue> pktNvmReadWaitIssue;

    /**
     * To avoid iterating over the outstanding requests
     * buffer for nvmReadEvent handler, we maintain the
     * required addresses in a priority queue.
     */
    std::priority_queue<addrNvmReadPair, std::vector<addrNvmReadPair>,
            std::greater<addrNvmReadPair> > addrNvmRead;

    std::vector<MemPacketQueue> pktNvmRead;

    /**
     * To avoid iterating over the outstanding requests
     * buffer for respNvmReadEvent handler, we maintain the
     * required addresses in a fifo queue.
     */
    std::deque <Addr> addrNvmRespReady;

    /**
     * To avoid iterating over the outstanding requests
     * buffer for dramWriteEvent handler, we maintain the
     * required addresses in a fifo queue.
     */
    std::deque <Addr> addrDramFill;
    std::vector<MemPacketQueue> pktDramWrite;

    /**
     * To avoid iterating over the outstanding requests
     * buffer for nvmWriteEvent handler, we maintain the
     * required addresses in a fifo queue.
     */
    typedef std::pair<Tick, MemPacket*> nvmWritePair;
    std::priority_queue<nvmWritePair, std::vector<nvmWritePair>,
                        std::greater<nvmWritePair> > nvmWritebackQueue;
    std::vector<MemPacketQueue> pktNvmWrite;


    void handleRequestorPkt(PacketPtr pkt);
    void checkHitOrMiss(reqBufferEntry* orbEntry);
    bool checkDirty(Addr addr);
    void handleDirtyCacheLine(reqBufferEntry* orbEntry);
    bool checkConflictInDramCache(PacketPtr pkt);
    void checkConflictInCRB(reqBufferEntry* orbEntry);
    bool resumeConflictingReq(reqBufferEntry* orbEntry);
    void logStatsDcache(reqBufferEntry* orbEntry);
    Tick earliestDirtyLineInDrRdResp();

    /**
     * Holds count of commands issued in burst window starting at
     * defined Tick. This is used to ensure that the command bandwidth
     * does not exceed the allowable media constraints.
     */
    std::unordered_multiset<Tick> burstTicks;

    /**
     * Create pointer to interface of the actual dram media when connected
     */
    DRAMInterface* const dram;

    /**
     * Create pointer to interface of the actual nvm media when connected
     */

    NVMInterface* const nvm;

    /**
     * The following are basic design parameters of the memory
     * controller, and are initialized based on parameter values.
     * The rowsPerBank is determined based on the capacity, number of
     * ranks and banks, the burst size, and the row buffer size.
     */
    unsigned long long dramCacheSize;
    unsigned blockSize;
    unsigned addrSize;
    unsigned orbMaxSize;
    unsigned orbSize;
    unsigned crbMaxSize;
    unsigned crbSize;

    unsigned writeHighThreshold;
    unsigned writeLowThreshold;
    unsigned minWritesPerSwitch;
    float dramWrDrainPerc;
    unsigned minDrWrPerSwitch;
    unsigned minNvWrPerSwitch;
    unsigned drWrCounter;
    unsigned nvWrCounter;

    /**
     * Memory controller configuration initialized based on parameter
     * values.
     */
    Enums::MemSched memSchedPolicy;

    /**
     * Pipeline latency of the controller frontend. The frontend
     * contribution is added to writes (that complete when they are in
     * the write buffer) and reads that are serviced the write buffer.
     */
    const Tick frontendLatency;

    /**
     * Pipeline latency of the backend and PHY. Along with the
     * frontend contribution, this latency is added to reads serviced
     * by the memory.
     */
    const Tick backendLatency;

    /**
     * Length of a command window, used to check
     * command bandwidth
     */
    const Tick commandWindow;

    /**
     * Till when must we wait before issuing next RD/WR burst?
     */
    Tick nextBurstAt;

    Tick prevArrival;

    /**
     * The soonest you have to start thinking about the next request
     * is the longest access time that can occur before
     * nextBurstAt. Assuming you need to precharge, open a new row,
     * and access, it is tRP + tRCD + tCL.
     */
    Tick nextReqTime;

    struct CtrlStats : public Stats::Group
    {
        CtrlStats(DcacheCtrl &ctrl);

        void regStats() override;

        DcacheCtrl &ctrl;

        // All statistics that the model needs to capture
        Stats::Scalar readReqs;
        Stats::Scalar writeReqs;
        Stats::Scalar readBursts;
        Stats::Scalar writeBursts;
        Stats::Scalar servicedByWrQ;
        Stats::Scalar mergedWrBursts;
        //Stats::Scalar neitherReadNorWriteReqs;
        // Average queue lengths
        Stats::Average avgRdQLen;
        Stats::Average avgWrQLen;

        Stats::Scalar numRdRetry;
        Stats::Scalar numWrRetry;
        Stats::Vector readPktSize;
        Stats::Vector writePktSize;
        //Stats::Vector rdQLenPdf;
        //Stats::Vector wrQLenPdf;
        //Stats::Histogram rdPerTurnAround;
        //Stats::Histogram wrPerTurnAround;
        Stats::Scalar rdToWrTurnAround;
        Stats::Scalar wrToRdTurnAround;

        Stats::Scalar bytesReadWrQ;
        Stats::Scalar bytesReadSys;
        Stats::Scalar bytesWrittenSys;
        // Average bandwidth
        Stats::Formula avgRdBWSys;
        Stats::Formula avgWrBWSys;

        Stats::Scalar totGap;
        Stats::Formula avgGap;

        // per-requestor bytes read and written to memory
        Stats::Vector requestorReadBytes;
        Stats::Vector requestorWriteBytes;

        // per-requestor bytes read and written to memory rate
        Stats::Formula requestorReadRate;
        Stats::Formula requestorWriteRate;

        // per-requestor read and write serviced memory accesses
        Stats::Vector requestorReadAccesses;
        Stats::Vector requestorWriteAccesses;

        // per-requestor read and write total memory access latency
        Stats::Vector requestorReadTotalLat;
        Stats::Vector requestorWriteTotalLat;

        // per-requestor raed and write average memory access latency
        Stats::Formula requestorReadAvgLat;
        Stats::Formula requestorWriteAvgLat;

        Stats::Scalar numHits;
        Stats::Scalar numMisses;
        Stats::Scalar numRdHits;
        Stats::Scalar numWrHits;
        Stats::Scalar numRdMisses;
        Stats::Scalar numWrMisses;
        Stats::Scalar numColdMisses;
        Stats::Scalar numHotMisses;
        Stats::Scalar numWrBacks;
        Stats::Scalar totNumConf;
        Stats::Scalar totNumConfBufFull;

        Stats::Scalar timeInDramRead;
        Stats::Scalar timeInDramWrite;
        Stats::Scalar timeInWaitingToIssueNvmRead;
        Stats::Scalar timeInNvmRead;
        Stats::Scalar timeInNvmWrite;

        Stats::Scalar drRdQingTime;
        Stats::Scalar drWrQingTime;
        Stats::Scalar nvmRdQingTime;
        Stats::Scalar nvmWrQingTime;

        Stats::Scalar drRdDevTime;
        Stats::Scalar drWrDevTime;
        Stats::Scalar nvRdDevTime;
        Stats::Scalar nvWrDevTime;

        Stats::Scalar totNumPktsDrRd;
        Stats::Scalar totNumPktsDrWr;
        Stats::Scalar totNumPktsNvmRdWait;
        Stats::Scalar totNumPktsNvmRd;
        Stats::Scalar totNumPktsNvmWr;

        Stats::Scalar maxNumConf;
        Stats::Scalar maxDrRdEvQ;
        Stats::Scalar maxDrRdRespEvQ;
        Stats::Scalar maxDrWrEvQ;
        Stats::Scalar maxNvRdIssEvQ;
        Stats::Scalar maxNvRdEvQ;
        Stats::Scalar maxNvRdRespEvQ;
        Stats::Scalar maxNvWrEvQ;
      };

    CtrlStats stats;

    /**
     * Upstream caches need this packet until true is returned, so
     * hold it for deletion until a subsequent call
     */
    std::unique_ptr<Packet> pendingDelete;

    /**
     * Remove commands that have already issued from burstTicks
     */
    void pruneBurstTick();

  public:

    DcacheCtrl(const DcacheCtrlParams &p);


    /**
     * Ensure that all interfaced have drained commands
     *
     * @return bool flag, set once drain complete
     */
    bool allIntfDrained() const;

    DrainState drain() override;

    /**
     * Check for command bus contention for single cycle command.
     * If there is contention, shift command to next burst.
     * Check verifies that the commands issued per burst is less
     * than a defined max number, maxCommandsPerWindow.
     * Therefore, contention per cycle is not verified and instead
     * is done based on a burst window.
     *
     * @param cmd_tick Initial tick of command, to be verified
     * @param max_cmds_per_burst Number of commands that can issue
     *                           in a burst window
     * @return tick for command issue without contention
     */
    Tick verifySingleCmd(Tick cmd_tick, Tick max_cmds_per_burst);

    /**
     * Check for command bus contention for multi-cycle (2 currently)
     * command. If there is contention, shift command(s) to next burst.
     * Check verifies that the commands issued per burst is less
     * than a defined max number, maxCommandsPerWindow.
     * Therefore, contention per cycle is not verified and instead
     * is done based on a burst window.
     *
     * @param cmd_tick Initial tick of command, to be verified
     * @param max_multi_cmd_split Maximum delay between commands
     * @param max_cmds_per_burst Number of commands that can issue
     *                           in a burst window
     * @return tick for command issue without contention
     */
    Tick verifyMultiCmd(Tick cmd_tick, Tick max_cmds_per_burst,
                        Tick max_multi_cmd_split = 0);

    /**
     * Is there a respondEvent scheduled?
     *
     * @return true if event is scheduled
     */
    bool respondEventScheduled() const { return respondEvent.scheduled(); }

    /**
     * Is there a read/write burst Event scheduled?
     *
     * @return true if event is scheduled
     */
    bool requestEventScheduled() const { return nextReqEvent.scheduled(); }

    /**
     * restart the controller
     * This can be used by interfaces to restart the
     * scheduler after maintainence commands complete
     *
     * @param Tick to schedule next event
     */
    void restartScheduler(Tick tick) { schedule(nextReqEvent, tick); }

    /**
     * Check the current direction of the memory channel
     *
     * @param next_state Check either the current or next bus state
     * @return True when bus is currently in a read state
     */
    bool inReadBusState(bool next_state) const;

    /**
     * Check the current direction of the memory channel
     *
     * @param next_state Check either the current or next bus state
     * @return True when bus is currently in a write state
     */
    bool inWriteBusState(bool next_state) const;

    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

    virtual void init() override;
    virtual void startup() override;
    virtual void drainResume() override;

  protected:

    Tick recvAtomic(PacketPtr pkt);
    Tick recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &backdoor);
    void recvFunctional(PacketPtr pkt);
    bool recvTimingReq(PacketPtr pkt);

};

#endif //__DCACHE_CTRL_HH__
