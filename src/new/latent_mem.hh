#ifndef __MEM_LATENT_MEM_HH__
#define __MEM_LATENT_MEM_HH__

#include "mem/mem_object.hh"
#include "params/LatentMem.hh"
#include "mem/packet.hh"
#include "mem/port.hh"

class LatentMem : public MemObject
{
  public:
    LatentMem(const LatentMemParams *p);

    BaseSlavePort& getSlavePort(const std::string &name, PortID idx = InvalidPortID) override;
    BaseMasterPort& getMasterPort(const std::string &name, PortID idx = InvalidPortID) override;

  private:
    class CPUSidePort;
    class MemSidePort;

    CPUSidePort *cpuPort;
    MemSidePort *memPort;

    Tick latency;

    void handleRequest(PacketPtr pkt);
    void handleResponse(PacketPtr pkt);

    class CPUSidePort : public SlavePort {
      public:
        CPUSidePort(const std::string& name, LatentMem* owner);
        Tick recvAtomic(PacketPtr pkt) override;
        void recvFunctional(PacketPtr pkt) override;
        void recvTimingReq(PacketPtr pkt) override;
        AddrRangeList getAddrRanges() const override;

      private:
        LatentMem* owner;
    };

    class MemSidePort : public MasterPort {
      public:
        MemSidePort(const std::string& name, LatentMem* owner);
        bool recvTimingResp(PacketPtr pkt) override;

      private:
        LatentMem* owner;
    };
};

#endif // __MEM_LATENT_MEM_HH__
