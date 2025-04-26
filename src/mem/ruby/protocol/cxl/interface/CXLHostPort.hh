#ifndef __MEM_CXL_CXL_HOST_PORT_HH__
#define __MEM_CXL_CXL_HOST_PORT_HH__

#include <limits>
#include <queue>
#include <unordered_map>

#include "mem/packet.hh"
#include "mem/port.hh"
#include "mem/ruby/network/MessageBuffer.hh"
#include "mem/ruby/slicc_interface/AbstractController.hh"
#include "params/CXLHostPort.hh"
#include "sim/eventq.hh"
#include "sim/clocked_object.hh"

// Generated from SLICC
#include "mem/ruby/protocol/CXL/CXLMemRequestMsg.hh"

namespace gem5
{

namespace ruby
{

namespace CXL
{

class CXLHostPort: public ClockedObject
{
  public:
    PARAMS(CXLHostPort);
    CXLHostPort(const Params& params);

    void init() override;

    // API to send/receive CXL requests
    // This looks a lot like a generic mem side port

    bool recvTimingReq(PacketPtr pkt);

    virtual Port &getPort(const std::string &if_name, PortID idx=InvalidPortID) override;

    void setController(AbstractController* controller)
    {
        rubyController = controller;
    }
    AddrRangeList getAddrRanges() const;
    void responseCallback(Addr addr, DataBlock data);

  private:
    RubySystem* rubySystemPtr;

    void initiateMemoryRequest(PacketPtr pkt);
    
    std::unordered_map<Addr, PacketPtr> outstandingRequests;

    class HostSidePort : public ResponsePort 
    {
      private:
        CXLHostPort* owner;
        //Are these needed? Probably needToSendRetry
        bool needToSendRetry;
        PacketPtr blockedPacket;

      public:
      HostSidePort(CXLHostPort* owner, const std::string& name):
            ResponsePort(name), owner(owner), needToSendRetry(false), blockedPacket(nullptr)
        {}
        virtual AddrRangeList getAddrRanges() const override;
        virtual bool recvTimingReq(PacketPtr pkt) override;
        virtual void recvRespRetry() override;
        virtual Tick recvAtomic(PacketPtr pkt) override;
        virtual void recvFunctional(PacketPtr pkt) override;
    };

    // class DeviceSidePort: public RequestPort
    // {
    //   private:
    //     CXLHostPort* owner;
    //     //Are these needed? Probably needToSendRetry
    //     bool needToSendRetry;
    //     PacketPtr blockedPacket;

    //   public:
    //     DeviceSidePort(CXLHostPort* owner, const std::string& name):
    //         RequestPort(name), owner(owner), needToSendRetry(false), blockedPacket(nullptr)
    //     {}
    //     bool needRetry() const { return needToSendRetry; }
    //     bool blocked() const { return blockedPacket != nullptr; }
    //     void sendPacket(PacketPtr pkt);

    //     virtual bool recvTimingResp(PacketPtr pkt) override;
    //     virtual void recvReqRetry() override;
    // };

    template<typename T>
    class TimedQueue
    {
      private:
        Tick latency;

        std::queue<T> items;
        std::queue<Tick> insertionTimes;

      public:
        TimedQueue(Tick latency): latency(latency) {}

        void push(T item, Tick insertion_time)
        {
            items.push(item);
            insertionTimes.push(insertion_time);
        }

        void pop()
        {
            items.pop();
            insertionTimes.pop();
        }

        T front() const { return items.front(); }

        bool empty() const { return items.empty(); }

        size_t size() const { return items.size(); }

        bool hasReady(Tick current_time) const
        {
            if (empty()) {
                return false;
            }
            return (current_time - insertionTimes.front()) >= latency;
        }

        Tick firstReadyTime() 
        {
          if (empty()) {
            return MaxTick;
          }
          return insertionTimes.front() + latency;
        }
    };

    HostSidePort hostSidePort;

    AddrRangeList memRanges;

    AbstractController* rubyController = nullptr;
    std::function<void(Addr)> invalidationCallback;

    TimedQueue<PacketPtr> requests;
    int reqQueueSize;

    TimedQueue<PacketPtr> responses;

    int requestIssueWidth;
    EventFunctionWrapper requestEvent;
    void scheduleNextProcessRequestEvent(Tick when);
    void processRequestEvent();

    EventFunctionWrapper responseEvent;
    void scheduleNextProcessResponseEvent(Tick when);
    void processResponseEvent();
  
};

} // namespace CXL

} // namespace ruby

} // namespace gem5

#endif //__MEM_CXL_CXL_HOST_PORT_HH__
