#ifndef __ACCL_GRAPH_SEGA_ACCL_ROUTER_HH__
#define __ACCL_GRAPH_SEGA_ACCL_ROUTER_HH__

#include <vector>

#include "accl/graph/sega/enums.hh"
#include "base/addr_range.hh"
#include "base/addr_range_map.hh"
#include "base/statistics.hh"
#include "base/stats/group.hh"
#include "mem/port.hh"
#include "params/AcclRouter.hh"
#include "sim/clocked_object.hh"

namespace gem5
{

class MPU;

class AcclRouter : public ClockedObject
{
  private:
    std::vector<MPU*> mpuVector;
    AddrRangeMap<MPU*> mpuAddrMap;
    std::vector<MPU*> inPortToMPU;  // Maps input ports to MPUs
    std::vector<MPU*> outPortToMPU; // Maps output ports to MPUs

    RouterMode mode;

    // SRNoC specific parameters
    double crosspointDelay;
    double mergerDelay;
    double splitterDelay;
    double circuitVariability;
    double crosspointSetupTime;
    double variabilityCountingNetwork;

    Tick timeSlot;
    Tick connectionWindow;
    uint64_t rlTimeSlots;
    uint64_t radix;

    statistics::Scalar packetsProcessed;
    statistics::Histogram valueLatency;

    size_t activeIndex = 0;
    void rotateActiveOutPort();
    EventFunctionWrapper rotateActiveOutPortEvent;

    PortID getMappedOutPort(PortID in_id) const;
    bool checkPortMapping(PortID in_id, PortID out_id) const;
    void printPortMappings() const;
    Tick ticksUntilPortActive(PortID in_id, PortID out_id) const;

    void assignTimeSlot();
    void computeTimingParameters();
    // Statistics for the AcclRouter

    // struct AcclRouterStats: public statistics::Group {
    //   // Constructor that links stats to the Router instance
    //   AcclRouterStats(AcclRouter& router);

    //   // Registers the statistics with the simulator
    //   void regStats() override;

    //   AcclRouter &router;

    //   // Latency distribution
    //   // statistics::Histogram valueLatency;
    //   // Number of packets processed
    //   statistics::Scalar packetsProcessed;

    //   // virtual ~AcclRouterStats();
    // };

    //AcclRouterStats stats;

  public:
    AcclRouter(const AcclRouterParams &params);
    ~AcclRouter();
    void assignInPortToMPU(PortID portId, MPU* mpu);
    void assignOutPortToMPU(PortID portId, MPU* mpu);
    MPU* getMPUForInPort(PortID portId) const;
    MPU* getMPUForOutPort(PortID portId) const;
    int getOutPortIndexForMPU(MPU* mpu) const;
    void notifyMPUDone(MPU* mpu);

    void setStaticDelayMode() { mode = RouterMode::STATIC_DELAY; }
    void setSRNoCMode() { mode = RouterMode::SRNOC; }

    class AcclRouterResponsePort: public ResponsePort
    {
      private:
        AcclRouter* owner;
        bool needRetry;
        PacketPtr blockedPacket;

        PortID _id;

      public:
        AcclRouterResponsePort(AcclRouter* owner,
            const std::string& name, PortID idx):
            ResponsePort(name),
            owner(owner),
            needRetry(false),
            blockedPacket(nullptr),
            _id(idx)
        {}

        PortID id() const { return _id; }
        void sendPacket(PacketPtr pkt);
        bool blocked() const { return blockedPacket != nullptr; }
        bool needRetryReq() const { return needRetry; }
        void sendRetryReq();

        virtual AddrRangeList getAddrRanges() const override;
        virtual bool recvTimingReq(PacketPtr pkt) override;
        virtual Tick recvAtomic(PacketPtr pkt) override;
        virtual void recvFunctional(PacketPtr pkt) override;
        virtual void recvRespRetry() override;
    };

    class AcclRouterRequestPort: public RequestPort
    {
      private:
        AcclRouter* owner;
        bool needRetry;
        PacketPtr blockedPacket;
        PortID _id;

      public:
        AcclRouterRequestPort(AcclRouter* owner,
            const std::string& name, PortID idx):
            RequestPort(name),
            owner(owner),
            needRetry(false),
            blockedPacket(nullptr),
            _id(idx)
        {}

        PortID id() { return _id; }

        void sendPacket(PacketPtr pkt);
        bool blocked() const { return blockedPacket != nullptr; }
        bool needRetryResp() const { return needRetry; }
        void sendRetryResp();

        virtual bool recvTimingResp(PacketPtr pkt) override;
        virtual void recvReqRetry() override;
    };

    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

    // Vector of ports for incoming connections
    std::vector<AcclRouterResponsePort> inPorts;

    // Vector of ports for outgoing connections
    std::vector<AcclRouterRequestPort> outPorts;

  protected:

    void init() override;
    void startup() override;

    //AddrRangeList getAddrRanges();
};

} // namespace gem5

#endif // __ACCL_GRAPH_SEGA_ACCL_ROUTER_HH__
