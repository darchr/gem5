
#ifndef __MEM_TIERED_MEM_TRANSLATOR_HH__
#define __MEM_TIERED_MEM_TRANSLATOR_HH__

#include <vector>

#include "base/types.hh"
#include "mem/port.hh"
#include "params/TieredMemTranslator.hh"
#include "sim/simulate.hh"

namespace gem5
{

class TieredMemTranslator : public SimObject
{
  private:
    class CPUSidePort : public ResponsePort
    {
      private:
        TieredMemTranslator* owner;

      public:
        CPUSidePort(const std::string& name, TieredMemTranslator* owner):
          ResponsePort(name, owner), owner(owner)
        {}
        virtual AddrRangeList getAddrRanges() const;

      protected:
        virtual bool recvTimingReq(PacketPtr pkt);
        virtual Tick recvAtomic(PacketPtr pkt);
        virtual void recvFunctional(PacketPtr pkt);
        virtual void recvRespRetry();
    };

    class MemSidePort : public RequestPort
    {
      private:
        TieredMemTranslator* owner;

        public:
        MemSidePort(const std::string& name, TieredMemTranslator* owner):
            RequestPort(name, owner), owner(owner)
        {}

        // bool sendPacket(PacketPtr pkt);

        protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvReqRetry();
    };

    CPUSidePort cpuSide;
    std::vector<MemSidePort> memSide;

    Addr offset;
    AddrRangeList rangeList;

  public:
    PARAMS(TieredMemTranslator);
    TieredMemTranslator(const Params& params);
    Port& getPort(const std::string &if_name,
                    PortID idx=InvalidPortID) override;

    virtual void init() override;
};

} // namespace gem5

#endif // __MEM_TIERED_MEM_TRANSLATOR_HH__