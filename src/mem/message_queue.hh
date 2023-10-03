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
#include <iostream>


namespace gem5
{

class update_item{
    public:
        uint32_t src_id;
        uint32_t dst_id;
        uint32_t dist;

        update_item(uint32_t src, uint32_t dst, uint32_t dist):
        src_id(src), dst_id(dst), dist(dist)
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

        std::deque<std::tuple<uint32_t, uint32_t, uint32_t, Tick>> queue; //Could be address or vertexID, vertexID might be easier
        
        uint32_t queueSize;
        AddrRange my_range;
        // std::vector<RespPort> corePorts; 
        RespPort corePorts;//= new RespPort();

        ReqPort fake_port;

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
        AddrRangeList getAddrRanges(){return {my_range};};
        void init();
        void checkRetryReq();

};


class MessageQueueIO : public PioDevice
{

};


}

#endif // __MESSAGE_QUEUE_HH__