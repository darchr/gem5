#ifndef __ACCL_GRAPH_SEGA_ROUTER_HH__
#define __ACCL_GRAPH_SEGA_ROUTER_HH__

#include "mem/port.hh"
#include "params/Router.hh"
#include "sim/clocked_object.hh"

namespace gem5
{

class MPU;

class Router : public ClockedObject
{
  private:
    MPU* owner;

  public:
    Router(const RouterParams &params);
    ~Router();

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

    AddrRangeList getAddrRanges();

    void registerMPU(MPU* mpu);

  protected:

    void init() override;
};

} // namespace gem5

#endif // __ACCL_GRAPH_SEGA_ROUTER_HH__
