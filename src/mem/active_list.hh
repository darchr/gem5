#ifndef __MESSAGE_QUEUE_HH__
#define __MESSAGE_QUEUE_HH__

#include "params/ActiveList.hh"
#include "sim/clocked_object.hh"
#include "mem/port.hh"
#include "base/addr_range.hh"
#include "sim/system.hh"
#include "mem/packet_access.hh"
#include "debug/ActiveList.hh"
#include "dev/io_device.hh"
#include "base/statistics.hh"


#include <deque>
#include <stdlib.h>
#include <tuple>
// #include <iostream>


namespace gem5
{


struct __attribute__ ((packed)) Vertex
{
    uint16_t dist : 16;
    uint64_t id : 32;
    uint64_t EL_start : 48;
    uint32_t EL_size : 31; // Degree
    bool active : 1;

    Vertex(uint64_t dist, uint64_t id, uint64_t EL_start, uint32_t EL_size, bool active):
        dist(dist),
        id(id),
        EL_start(EL_start),
        EL_size(EL_size),
        active(active)
    {} 

    std::string to_string() {
      std::string ret = "";
      ret += "Vertex{dist: " + std::to_string(dist) + ", id: " +
              std::to_string(id) + "}";
      return ret;
    }
};
// struct __attribute__ ((packed)) Update
// struct __attribute__ ((packed)) Update
// {
//     // uint16_t weight : 16;
//     // uint64_t dst_id : 48;
//     //uint64_t src_id : 48;

//     uint16_t weight : 16;
//     uint64_t dst_id : 48;

//     std::string to_string()
//     {
//         return csprintf("Update{weight: %u, dst_id: %lu}", weight, dst_id);
//     }

//     Update(): weight(0), dst_id(0) {}

//     Update(uint16_t weight, uint64_t dst_id):
//         weight(weight),
//         dst_id(dst_id)
//     {}
// };


class ActiveList : public ClockedObject
{
    

    private:

        System* system;
        std::deque<PacketPtr> pkt_read_queue;
        std::deque<PacketPtr> pkt_write_queue;

         


        class ReqPort : public RequestPort
        {
            private:
                ActiveList* owner;
                PacketPtr blockedPacket;
                PortID _id = 1;

            public:
                ReqPort(const std::string& name, ActiveList* owner) :
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
                ActiveList *owner;
                bool needSendRetryReq = false;
                PortID _id = 0;

            public:
                RespPort(const std::string& name, ActiveList* owner):
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
        // std::unordered_map<uint64_t, Vertex> valueMap; // change Addr to uint64_t
        std::deque<uint64_t> valueMap; // 

       
        // std::deque<std::tuple<uint32_t, uint32_t, uint32_t, Tick>> queue; //Could be address or vertexID, vertexID might be easier
        
        uint32_t queueSize;
        AddrRange myRange;
        // std::vector<RespPort> corePorts; 
        RespPort corePorts;//= new RespPort();

        ReqPort fakePort;
        int alternate_w = 0;
        int alternate_r = 1;
        Vertex vw = Vertex(0,0,0,0,0);
        Vertex vr = Vertex(0,0,0,0,0);


        uint32_t queueLength(){return valueMap.size();}


        EventFunctionWrapper nextReadEvent;
        void processNextReadEvent();

        EventFunctionWrapper nextWriteEvent;
        void processNextWriteEvent();





    //     struct ActiveListStats : public statistics::Group
    //     {
    //         ActiveListStats(ActiveList& _msgqueue);

    //         void regStats() override;

    //         ActiveList &msgqueue;
    //         statistics::Scalar numMessagesReceived;
    //         statistics::Scalar maxQueueLength;
   
    //     };

    // ActiveListStats stats;

    protected:

        //Tick recvAtomic(PacketPtr pkt);
        //Tick recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &backdoor);
        void recvFunctional(PacketPtr pkt);
        bool recvTimingReq(PacketPtr pkt);

    public:
        ActiveList(const ActiveListParams &p);
        Port& getPort(const std::string &if_name, PortID idx) override;
        AddrRangeList getAddrRanges(){return {myRange};};
        void init();
        void checkRetryReq();

};


class ActiveListIO : public PioDevice
{

};


}

#endif // __MESSAGE_QUEUE_HH__