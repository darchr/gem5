#ifndef __ACCL_GRAPH_SEGA_ROUTER_HH__
#define __ACCL_GRAPH_SEGA_ROUTER_HH__

#include <vector>

#include "base/addr_range.hh"
#include "base/addr_range_map.hh"
#include "mem/port.hh"
#include "params/Router.hh"
#include "sim/clocked_object.hh"

namespace gem5
{

class MPU;

class Router : public ClockedObject
{
  private:
    std::vector<MPU*> mpuVector;
    AddrRangeMap<MPU*> mpuAddrMap;
    std::vector<MPU*> inPortToMPU;  // Maps input ports to MPUs
    std::vector<MPU*> outPortToMPU; // Maps output ports to MPUs

  public:
    Router(const RouterParams &params);
    ~Router();
    void assignInPortToMPU(PortID portId, MPU* mpu);
    void assignOutPortToMPU(PortID portId, MPU* mpu);
    MPU* getMPUForInPort(PortID portId) const;
    MPU* getMPUForOutPort(PortID portId) const;

    class RouterResponsePort: public ResponsePort
    {
      private:
        Router* owner;
        bool needRetry;
        PacketPtr blockedPacket;

        PortID _id;

      public:
        RouterResponsePort(Router* owner, const std::string& name, PortID idx):
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

    class RouterRequestPort: public RequestPort
    {
      private:
        Router* owner;
        bool needRetry;
        PacketPtr blockedPacket;
        PortID _id;

      public:
        RouterRequestPort(Router* owner, const std::string& name, PortID idx):
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
    std::vector<RouterResponsePort> inPorts;

    // Vector of ports for outgoing connections
    std::vector<RouterRequestPort> outPorts;

  protected:

    void init() override;
    void startup() override;

    //AddrRangeList getAddrRanges();
};

} // namespace gem5

#endif // __ACCL_GRAPH_SEGA_ROUTER_HH__
