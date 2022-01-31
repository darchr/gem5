#ifndef __ACCL_PUSH_ENGINE_HH__
#define __ACCL_PUSH_ENGINE_HH__

#include <queue>
#include <unordered_map>

#include "base/addr_range_map.hh"
#include "base/statistics.hh"
#include "mem/port.hh"
#include "mem/packet.hh"
#include "params/PushEngine.hh"
#include "sim/clocked_object.hh"

class PushEngine : public ClockedObject
{
  private:

    class PushRespPort : public ResponsePort
    {
      private:
        bool _blocked;
        PacketPtr blockedPacket;

      public:
        PushRespPort(const std::string& name, SimObject* _owner,
              PortID id=InvalidPortID);

        virtual AddrRangeList getAddrRanges();
        virtual bool recvTimingReq(PacketPtr pkt);
    }

    class PushReqPort : public RequestPort
    {
      private:
        bool _blocked;
        PacketPtr blockedPacket;

      public:
        PushReqPort(const std::string& name, SimObject* _owner,
              PortID id=InvalidPortID);

        virtual bool recvTimingResp(PacketPtr pkt);
    }

    class PushMemPort : public RequestPort
    {
      private:
        bool _blocked;
        PacketPtr blockedPacket;

      public:
        PushMemPort(const std::string& name, SimObject* _owner,
              PortID id=InvalidPortID);
        bool sendPacket(PacktPtr pkt);
        virtual bool recvTimingResp(PacketPtr pkt);
    }

    PushRespPort respPort;
    PushReqPort reqPort;
    PushMemPort memPort;

    std::queue<PacketPtr> vertexQueue;
    std::queue<PacketPtr> updateQueue;

    std::pair<Addr, int> interpretPackPtr(PacketPtr pkt);

};

#endif // __ACCL_PUSH_ENGINE_HH__
