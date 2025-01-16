#ifndef __MEM_CXL_CXL_CONTROLLER_HH__
#define __MEM_CXL_CXL_CONTROLLER_HH__

#include "mem/cxl/cxl_host_port.hh"
#include "mem/port.hh"
#include "sim/sim_object.hh"

namespace gem5
{

class CXLController: public SimObject
{
  public:
    PARAMS(CXLController);
    CXLController(const Params &p);

  private:

    class HostSidePort: public ResponsePort
    {
      private:
        CXLController *owner;

      public:
        HostSidePort(const std::string& name, CXLController *owner) :
            ResponsePort(name), owner(owner)
        { }

        void sendPacket(PacketPtr pkt);
        AddrRangeList getAddrRanges() const override;

      protected:
        Tick recvAtomic(PacketPtr pkt) override;
        void recvFunctional(PacketPtr pkt) override;
        bool recvTimingReq(PacketPtr pkt) override;
        void recvRespRetry() override;
    };

    CXLHostPort *cxlOutputPort;
    HostSidePort *hostInputPort;

    AddrRangeList memRanges;

    void handleInvalidation(Addr addr);

  public:
    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;
};

} // namespace gem5

#endif //__MEM_CXL_CXL_CONTROLLER_HH__
