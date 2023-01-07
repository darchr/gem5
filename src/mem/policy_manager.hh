/**
 * @file
 * DCacheCtrl declaration
 */

#ifndef __POLICY_MANAGER_HH__
#define __POLICY_MANAGER_HH__

#include <cstdint>
#include <queue>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/callback.hh"
#include "base/compiler.hh"
#include "base/logging.hh"
#include "base/statistics.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "enums/Policy.hh"
#include "mem/mem_ctrl.hh"
#include "mem/mem_interface.hh"
#include "mem/packet.hh"
#include "mem/qport.hh"
#include "mem/request.hh"
#include "params/PolicyManager.hh"
#include "sim/clocked_object.hh"
#include "sim/cur_tick.hh"
#include "sim/eventq.hh"
#include "sim/system.hh"

namespace gem5
{

namespace memory
{
class PolicyManager : public AbstractMemory
{
  protected:

    class RespPortPolManager : public QueuedResponsePort
    {
      private:

        RespPacketQueue queue;
        PolicyManager& polMan;

      public:

        RespPortPolManager(const std::string& name, PolicyManager& _polMan)
            : QueuedResponsePort(name, &_polMan, queue),
              queue(_polMan, *this, true),
              polMan(_polMan)
        { }

      protected:

        Tick recvAtomic(PacketPtr pkt) override
                            {return polMan.recvAtomic(pkt);}

        Tick recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &backdoor) override
                            {return polMan.recvAtomicBackdoor(pkt, backdoor);}

        void recvFunctional(PacketPtr pkt) override
                            {polMan.recvFunctional(pkt);}

        bool recvTimingReq(PacketPtr pkt) override
                            {return polMan.recvTimingReq(pkt);}

        AddrRangeList getAddrRanges() const override
                            {return polMan.getAddrRanges();}

    };

    class ReqPortPolManager : public RequestPort
    {
      public:

        ReqPortPolManager(const std::string& name, PolicyManager& _polMan)
            : RequestPort(name, &_polMan), polMan(_polMan)
        { }

      protected:

        void recvReqRetry();

        bool recvTimingResp(PacketPtr pkt);

      private:

        PolicyManager& polMan;

    };

    RespPortPolManager port;
    ReqPortPolManager locReqPort;
    ReqPortPolManager farReqPort;

    unsigned locBurstSize;
    unsigned farBurstSize;

    enums::Policy locMemPolicy;

    /**
     * The following are basic design parameters of the unified
     * DRAM cache controller, and are initialized based on parameter values.
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
    bool alwaysHit;
    bool alwaysDirty;

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

    Tick tRP;
    Tick tRCD_RD;
    Tick tRL;

    unsigned numColdMisses;
    float cacheWarmupRatio;
    bool resetStatsWarmup;

    Tick prevArrival;

    std::unordered_set<Addr> isInWriteQueue;

    struct tagMetaStoreEntry
    {
      // DRAM cache related metadata
      Addr tagDC;
      Addr indexDC;
      // constant to indicate that the cache line is valid
      bool validLine = false;
      // constant to indicate that the cache line is dirty
      bool dirtyLine = false;
      Addr farMemAddr;
    };

    /** A storage to keep the tag and metadata for the
     * DRAM Cache entries.
     */
    std::vector<tagMetaStoreEntry> tagMetadataStore;

    /** Different states a packet can transition from one
     * to the other while it's process in the DRAM Cache
     * Controller.
     */
    enum reqState
    {
      start,
      locMemRead,  waitingLocMemReadResp,
      locMemWrite, waitingLocMemWriteResp,
      farMemRead,  waitingFarMemReadResp,
      farMemWrite, waitingFarMemWriteResp
    };

    /**
     * A class for the entries of the
     * outstanding request buffer (ORB).
     */
    class reqBufferEntry
    {
      public:

        bool validEntry;
        Tick arrivalTick;

        // DRAM cache related metadata
        Addr tagDC;
        Addr indexDC;

        // pointer to the outside world (ow) packet received from llc
        const PacketPtr owPkt;

        enums::Policy pol;
        reqState state;

        bool issued;
        bool isHit;
        bool conflict;

        Addr dirtyLineAddr;
        bool handleDirtyLine;

        // recording the tick when the req transitions into a new stats.
        // The subtract between each two consecutive states entrance ticks,
        // is the number of ticks the req spent in the proceeded state.
        // The subtract between entrance and issuance ticks for each state,
        // is the number of ticks for waiting time in that state.
        Tick locRdEntered;
        Tick locRdIssued;
        Tick locRdExit;
        Tick locWrEntered;
        Tick locWrIssued;
        Tick locWrExit;
        Tick farRdEntered;
        Tick farRdIssued;
        Tick farRdExit;

        reqBufferEntry(
          bool _validEntry, Tick _arrivalTick,
          Addr _tagDC, Addr _indexDC,
          PacketPtr _owPkt,
          enums::Policy _pol, reqState _state,
          bool _issued, bool _isHit, bool _conflict,
          Addr _dirtyLineAddr, bool _handleDirtyLine,
          Tick _locRdEntered, Tick _locRdIssued, Tick _locRdExit,
          Tick _locWrEntered, Tick _locWrIssued, Tick _locWrExit,
          Tick _farRdEntered, Tick _farRdIssued, Tick _farRdExit)
        :
        validEntry(_validEntry), arrivalTick(_arrivalTick),
        tagDC(_tagDC), indexDC(_indexDC),
        owPkt( _owPkt),
        pol(_pol), state(_state),
        issued(_issued), isHit(_isHit), conflict(_conflict),
        dirtyLineAddr(_dirtyLineAddr), handleDirtyLine(_handleDirtyLine),
        locRdEntered(_locRdEntered), locRdIssued(_locRdIssued), locRdExit(_locRdExit),
        locWrEntered(_locWrEntered), locWrIssued(_locWrIssued), locWrExit(_locWrExit),
        farRdEntered(_farRdEntered), farRdIssued(_farRdIssued), farRdExit(_farRdExit)
        { }
    };

    /**
     * This is the outstanding request buffer (ORB) data
     * structure, the main DS within the DRAM Cache
     * Controller. The key is the address, for each key
     * the map returns a reqBufferEntry which maintains
     * the entire info related to that address while it's
     * been processed in the DRAM Cache controller.
     */
    std::map<Addr,reqBufferEntry*> ORB;

    typedef std::pair<Tick, PacketPtr> timeReqPair;
    /**
     * This is the second important data structure
     * within the DRAM cache controller which holds
     * received packets that had conflict with some
     * other address(s) in the DRAM Cache that they
     * are still under process in the controller.
     * Once thoes addresses are finished processing,
     * Conflicting Requets Buffre (CRB) is consulted
     * to see if any packet can be moved into the
     * outstanding request buffer and start being
     * processed in the DRAM cache controller.
     */
    std::vector<timeReqPair> CRB;

    /**
     * This is a unified retry flag for both reads and writes.
     * It helps remember if we have to retry a request when available.
     */
    bool retryLLC;
    bool retryLLCFarMemWr;
    bool retryLocMemRead;
    bool retryFarMemRead;
    bool retryLocMemWrite;
    bool retryFarMemWrite;

    /**
     * A queue for evicted dirty lines of DRAM cache,
     * to be written back to the backing memory.
     * These packets are not maintained in the ORB.
     */
    std::deque <timeReqPair> pktFarMemWrite;

    // Maintenance Queues
    std::deque <Addr> pktLocMemRead;
    std::deque <Addr> pktLocMemWrite;
    std::deque <Addr> pktFarMemRead;

    // Maintenance variables
    unsigned maxConf;

    AddrRangeList getAddrRanges();

    // events
    void processLocMemReadEvent();
    EventFunctionWrapper locMemReadEvent;

    void processLocMemWriteEvent();
    EventFunctionWrapper locMemWriteEvent;

    void processFarMemReadEvent();
    EventFunctionWrapper farMemReadEvent;

    void processFarMemWriteEvent();
    EventFunctionWrapper farMemWriteEvent;

    // management functions
    void setNextState(reqBufferEntry* orbEntry);
    void handleNextState(reqBufferEntry* orbEntry);
    void sendRespondToRequestor(PacketPtr pkt, Tick static_latency);
    void printQSizes() {}
    void handleRequestorPkt(PacketPtr pkt);
    void checkHitOrMiss(reqBufferEntry* orbEntry);
    bool checkDirty(Addr addr);
    void handleDirtyCacheLine(reqBufferEntry* orbEntry);
    bool checkConflictInDramCache(PacketPtr pkt);
    void checkConflictInCRB(reqBufferEntry* orbEntry);
    bool resumeConflictingReq(reqBufferEntry* orbEntry);
    void logStatsPolMan(reqBufferEntry* orbEntry);
    void accessAndRespond(PacketPtr pkt, Tick static_latency);
    PacketPtr getPacket(Addr addr, unsigned size, const MemCmd& cmd, Request::FlagsType flags = 0);
    Tick accessLatency();

    unsigned countLocRdInORB();
    unsigned countFarRdInORB();
    unsigned countLocWrInORB();
    unsigned countFarWr();

    Addr returnIndexDC(Addr pkt_addr, unsigned size);
    Addr returnTagDC(Addr pkt_addr, unsigned size);

    // port management
    void locMemRecvReqRetry();
    void farMemRecvReqRetry();

    //void locMemRetryReq() {}
    //void farMemRetryReq() {}

    bool locMemRecvTimingResp(PacketPtr pkt);
    bool farMemRecvTimingResp(PacketPtr pkt);
    struct PolicyManagerStats : public statistics::Group
    {
      PolicyManagerStats(PolicyManager &polMan);

      void regStats() override;

      const PolicyManager &polMan;

      // All statistics that the model needs to capture
      statistics::Scalar readReqs;
      statistics::Scalar writeReqs;

      statistics::Scalar servicedByWrQ;
      statistics::Scalar mergedWrBursts;

      statistics::Scalar numRdRetry;
      statistics::Scalar numWrRetry;

      statistics::Vector readPktSize;
      statistics::Vector writePktSize;

      statistics::Scalar bytesReadWrQ;
      statistics::Scalar bytesReadSys;
      statistics::Scalar bytesWrittenSys;

      // Average bandwidth
      statistics::Formula avgRdBWSys;
      statistics::Formula avgWrBWSys;

      statistics::Scalar totGap;
      statistics::Formula avgGap;

      // DRAM Cache Specific Stats
      statistics::Average avgORBLen;
      statistics::Average avgLocRdQLenStrt;
      statistics::Average avgLocWrQLenStrt;
      statistics::Average avgFarRdQLenStrt;
      statistics::Average avgFarWrQLenStrt;

      statistics::Average avgLocRdQLenEnq;
      statistics::Average avgLocWrQLenEnq;
      statistics::Average avgFarRdQLenEnq;
      statistics::Average avgFarWrQLenEnq;

      Stats::Scalar numWrBacks;
      Stats::Scalar totNumConf;
      Stats::Scalar totNumORBFull;
      Stats::Scalar totNumCRBFull;

      Stats::Scalar maxNumConf;

      Stats::Scalar sentLocRdPort;
      Stats::Scalar sentLocWrPort;
      Stats::Scalar failedLocRdPort;
      Stats::Scalar failedLocWrPort;
      Stats::Scalar recvdRdPort;
      Stats::Scalar sentFarRdPort;
      Stats::Scalar sentFarWrPort;
      Stats::Scalar failedFarRdPort;
      Stats::Scalar failedFarWrPort;

      Stats::Scalar totPktsServiceTime;
      Stats::Scalar totPktsORBTime;
      Stats::Scalar totTimeFarRdtoSend;
      Stats::Scalar totTimeFarRdtoRecv;
      Stats::Scalar totTimeFarWrtoSend;
      Stats::Scalar totTimeInLocRead;
      Stats::Scalar totTimeInLocWrite;
      Stats::Scalar totTimeInFarRead;

      Stats::Scalar numTotHits;
      Stats::Scalar numTotMisses;
      Stats::Scalar numColdMisses;
      Stats::Scalar numHotMisses;
      Stats::Scalar numRdMissClean;
      Stats::Scalar numRdMissDirty;
      Stats::Scalar numRdHit;
      Stats::Scalar numWrMissClean;
      Stats::Scalar numWrMissDirty;
      Stats::Scalar numWrHit;
      Stats::Scalar numRdHitDirty;
      Stats::Scalar numRdHitClean;
      Stats::Scalar numWrHitDirty;
      Stats::Scalar numWrHitClean;

    };

    PolicyManagerStats polManStats;

  public:

    PolicyManager(const PolicyManagerParams &p);

    void init();

    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID);

    // For preparing for checkpoints
    DrainState drain() override;

    // Serializes the tag state so that we don't have to warm up each time.
    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;

    protected:

      Tick recvAtomic(PacketPtr pkt);
      Tick recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &backdoor);
      void recvFunctional(PacketPtr pkt);
      bool recvTimingReq(PacketPtr pkt);
};

} // namespace memory
} // namespace gem5

#endif //__POLICY_MANAGER_HH__
