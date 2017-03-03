
#ifndef __MEM_RANDOM_DELAY_BRIDGE_HH__
#define __MEM_RANDOM_DELAY_BRIDGE_HH__

#include "mem/mem_object.hh"
#include "params/RandomDelayBridge.hh"

class RandomDelayBridge : public MemObject
{
    Tick maxDelay;

  public:

    /** Parameters of communication monitor */
    typedef RandomDelayBridgeParams Params;

    /**
     * Constructor based on the Python params
     *
     * @param params Python parameters
     */
    RandomDelayBridge(Params* params) :
        MemObject(params), maxDelay(params->max_delay),
        masterPort(name() + "-master", *this),
        slavePort(name() + "-slave", *this)
    { }

    BaseMasterPort& getMasterPort(const std::string& if_name,
                                  PortID idx = InvalidPortID) override
    {
        if (if_name == "master") {
            return masterPort;
        } else {
            return MemObject::getMasterPort(if_name, idx);
        }
    }

    BaseSlavePort& getSlavePort(const std::string& if_name,
                                PortID idx = InvalidPortID) override
    {
        if (if_name == "slave") {
            return slavePort;
        } else {
            return MemObject::getSlavePort(if_name, idx);
        }
    }

  private:
    class RDBMasterPort : public MasterPort
    {
        RandomDelayBridge& rdb;

      public:
        RDBMasterPort(const std::string& name, RandomDelayBridge &rdb) :
            MasterPort(name, &rdb), rdb(rdb)
        { }

      protected:
        void recvFunctionalSnoop(PacketPtr pkt)
            { rdb.recvFunctionalSnoop(pkt); }

        Tick recvAtomicSnoop(PacketPtr pkt)
            { return rdb.recvAtomicSnoop(pkt); }

        bool recvTimingResp(PacketPtr pkt)
            { return rdb.recvTimingResp(pkt); }

        void recvTimingSnoopReq(PacketPtr pkt)
            { rdb.recvTimingSnoopReq(pkt); }

        void recvRangeChange()
            { rdb.recvRangeChange(); }

        bool isSnooping() const
            { return rdb.isSnooping(); }

        void recvReqRetry()
            { rdb.recvReqRetry(); }

        void recvRetrySnoopResp()
            { rdb.recvRetrySnoopResp(); }
    };

    class RDBSlavePort : public SlavePort
    {
        RandomDelayBridge& rdb;

      public:

        RDBSlavePort(const std::string& name, RandomDelayBridge &rdb) :
            SlavePort(name, &rdb), rdb(rdb)
        { }

      protected:

        void recvFunctional(PacketPtr pkt)
            { rdb.recvFunctional(pkt); }

        Tick recvAtomic(PacketPtr pkt)
            { return rdb.recvAtomic(pkt); }

        bool recvTimingReq(PacketPtr pkt)
            { return rdb.recvTimingReq(pkt); }

        bool recvTimingSnoopResp(PacketPtr pkt)
            { return rdb.recvTimingSnoopResp(pkt); }

        AddrRangeList getAddrRanges() const
            { return rdb.getAddrRanges(); }

        void recvRespRetry()
            { rdb.recvRespRetry(); }
    };

    RDBMasterPort masterPort;
    RDBSlavePort slavePort;

    void recvFunctional(PacketPtr pkt)
        { masterPort.sendFunctional(pkt); }

    void recvFunctionalSnoop(PacketPtr pkt)
        { slavePort.sendFunctionalSnoop(pkt); }

    Tick recvAtomic(PacketPtr pkt)
        { return masterPort.sendAtomic(pkt); }

    Tick recvAtomicSnoop(PacketPtr pkt)
        { return slavePort.sendAtomicSnoop(pkt); }

    bool recvTimingReq(PacketPtr pkt);

    bool recvTimingResp(PacketPtr pkt)
        { return slavePort.sendTimingResp(pkt); }

    void recvTimingSnoopReq(PacketPtr pkt)
        { slavePort.sendTimingSnoopReq(pkt); }

    bool recvTimingSnoopResp(PacketPtr pkt)
        { return masterPort.sendTimingSnoopResp(pkt); }

    void recvRetrySnoopResp()
        { slavePort.sendRetrySnoopResp(); }

    AddrRangeList getAddrRanges() const
        { return masterPort.getAddrRanges(); }

    bool isSnooping() const
        { return slavePort.isSnooping(); }

    void recvReqRetry()
        { slavePort.sendRetryReq(); }

    void recvRespRetry()
        { masterPort.sendRetryResp(); }

    void recvRangeChange()
        { slavePort.sendRangeChange(); }

};

#endif
