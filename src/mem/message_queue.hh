#ifndef __MESSAGE_QUEUE_HH__
#define __MESSAGE_QUEUE_HH__

#include "params/MessageQueue.hh"
#include "sim/clocked_object.hh"
#include "mem/port.hh"
#include "base/addr_range.hh"
#include "sim/system.hh"
#include "mem/packet_access.hh"
#include "debug/MessageQueue.hh"
#include "dev/io_device.hh"

#include <deque>
#include <stdlib.h>
#include <tuple>
// #include <iostream>


namespace gem5
{

// struct __attribute__ ((packed)) Update
struct Update
{
    // uint16_t weight : 16;
    // uint64_t dst_id : 48;
    //uint64_t src_id : 48;

    uint16_t weight : 16;
    uint64_t dst_id : 48;

    std::string to_string()
    {
        return csprintf("Update{weight: %u, dst_id: %lu}", weight, dst_id);
    }

    Update(): weight(0), dst_id(0) {}

    Update(uint16_t weight, uint64_t dst_id):
        weight(weight),
        dst_id(dst_id)
    {}
};


class MessageQueue : public ClockedObject
{
    

    private:

        System* system;
        std::deque<PacketPtr> pkt_read_queue;
        std::deque<PacketPtr> pkt_write_queue;

         


        class ReqPort : public RequestPort
        {
            private:
                MessageQueue* owner;
                PacketPtr blockedPacket;
                PortID _id = 1;

            public:
                ReqPort(const std::string& name, MessageQueue* owner) :
                RequestPort(name, owner),
                owner(owner)
                {}
                void sendPacket(PacketPtr pkt);
                bool blocked() { return (blockedPacket != nullptr); }

            protected:
                virtual bool recvTimingResp(PacketPtr pkt);
                virtual void recvReqRetry();
        };

        class RespPort : public ResponsePort
        {
            private:
                MessageQueue *owner;
                bool needSendRetryReq = false;
                PortID _id = 0;

            public:
                RespPort(const std::string& name, MessageQueue* owner):
                ResponsePort(name, owner), owner(owner)// was name , owner
                {}
                virtual AddrRangeList getAddrRanges() const;

                PortID id() { return _id; }
                void checkRetryReq();
                

            protected:
                virtual bool recvTimingReq(PacketPtr pkt);
                virtual Tick recvAtomic(PacketPtr pkt);
                virtual void recvFunctional(PacketPtr pkt);
                virtual void recvRespRetry();


            
        };

        // Design decision: 
        // map the address of each vertex to its own PA in the queue(would need to call map for every vertex)
        // or
        // map a set of vertices to a single VA(only need to call map once per queue)
        // if this is chosen, we cannot use the address to determine the vertex, we would need to use the vertexID
        std::deque<std::tuple<uint64_t, Tick>> queue; // Addr may need to be changed to dst_id i.e. uint64_t 
        std::unordered_map<uint64_t, Update> valueMap; // change Addr to uint64_t
    

       
        // std::deque<std::tuple<uint32_t, uint32_t, uint32_t, Tick>> queue; //Could be address or vertexID, vertexID might be easier
        
        uint32_t queueSize;
        AddrRange myRange;
        // std::vector<RespPort> corePorts; 
        RespPort corePorts;//= new RespPort();

        ReqPort fakePort;

        uint32_t queueLength(){return queue.size();}


        EventFunctionWrapper nextReadEvent;
        void processNextReadEvent();

        EventFunctionWrapper nextWriteEvent;
        void processNextWriteEvent();

    protected:

        //Tick recvAtomic(PacketPtr pkt);
        //Tick recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &backdoor);
        void recvFunctional(PacketPtr pkt);
        bool recvTimingReq(PacketPtr pkt);

    public:
        MessageQueue(const MessageQueueParams &p);
        Port& getPort(const std::string &if_name, PortID idx) override;
        AddrRangeList getAddrRanges(){return {myRange};};
        void init();
        void checkRetryReq();

};


class MessageQueueIO : public PioDevice
{

};


}

#endif // __MESSAGE_QUEUE_HH__