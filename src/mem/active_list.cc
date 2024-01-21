#include "mem/active_list.hh"
#include "sim/stats.hh"
#include <cstdio>

// /src/mem/message_queue.hh

namespace gem5{



    ActiveList::ActiveList(const ActiveListParams &p) :
        ClockedObject(p), queueSize(p.queueSize), myRange(p.myRange), corePorts(name() + ".cpu_side", this), fakePort(name() + ".mem_side", this),  nextReadEvent([this]{ processNextReadEvent(); }, name()),
        nextWriteEvent([this] { processNextWriteEvent(); }, name()) //, stats(*this))
        {
            DPRINTF(ActiveList, "%s: port name: %s  AddrRange: %d - %d\n", __func__,  (name() + ".cpu_side"), p.myRange.start(), p.myRange.end());
            // DPRINTF(ActiveList, "%s: Response_Port addr_range end: %s:%s\n", __func__,  corePorts.getAddrRanges().front().front, corePorts.getAddrRanges().front().end);

            //std::cout << "corePorts size: " << corePorts.getAddrRanges().front().size() << "\n";

            
        }

    bool
    ActiveList::recvTimingReq(PacketPtr pkt)
    {
        //Addr this_addr = pkt->getAddr();
        //DPRINTF(ActiveList, "%s: isWrite(): %d, Message Queue size = %d, Cmd = %s\n", __func__,  pkt->isWrite(), queueLength(), pkt->cmdString());

        if(pkt->isWrite()){

            if(queueLength() >= queueSize){
            // full queue

                DPRINTF(ActiveList, "%s: Tried to write to full queue\n", __func__);
                return false;
            }
            
            pkt_write_queue.emplace_back(pkt);

            if (!nextWriteEvent.scheduled()) {
                schedule(nextWriteEvent, nextCycle());
            }

        }

        else if(pkt->isRead()){
            //std::tuple<uint32_t, uint32_t, uint32_t, Tick> element = queue.front();

            
            //std::tuple<uint32_t, uint32_t, uint32_t> to_return = std::tuple<std::get<0>(element), std::get<1>(element), std::get<2>(element)>;
            
            // cant just check here, need to check after the read event is processed
            // if it fails in the read event how do we know to send retry?
            // Could possibly check length of read queue and write queue
            if(queueLength() == 0){
            // full queue
            // return false; // causes loop
            }

            pkt_read_queue.emplace_back(pkt);

            if (!nextReadEvent.scheduled()) {
                schedule(nextReadEvent, nextCycle());
            }
            //if(queueLength() > 0){
                // DPRINTF(ActiveList, "%s: Read Value: %d\n", __func__, std::get<0>(queue.front()));
                // // pkt->setData(std::get<0>(queue.front()) + 15);
                // //pkt->setLE(std::get<0>(queue.front()) + 15);
                // pkt->setLE(15);
                // //queue.pop_front();
                // DPRINTF(ActiveList, "%s: Returned Value: %d\n", __func__, pkt->getLE<uint32_t>());
                // checkRetryReq();
            //}

        }
        // if (pkt->needsResponse()) {
        //     pkt->makeResponse();
        //     corePorts.sendTimingResp(pkt);
        // }
        // pkt->makeResponse();
        // corePorts.sendTimingResp(pkt);
        

        return true;
    }

    // Tick 
    // ActiveList::recvAtomic(PacketPtr pkt){

    // }

    void 
    ActiveList::recvFunctional(PacketPtr pkt){

        DPRINTF(ActiveList, "%s: isWrite(): %d, Cmd = %s\n", __func__,  pkt->isWrite(), pkt->cmdString());
        
        fakePort.sendFunctional(pkt);
    
    
        //panic("recvFunctional unimpl.");

    }

    void
    ActiveList::init()
    {
        corePorts.sendRangeChange();
        // fakePort.recvRangeChange();
    }


    void
    ActiveList::processNextWriteEvent()
    {
        //DPRINTF(ActiveList, "in processNextWriteEvent \n");

        PacketPtr pkt = pkt_write_queue.front();
        pkt_write_queue.pop_front();
        //DPRINTF(ActiveList, "Before reading packet\n");

        // uint64_t this_update = pkt->getLE<Vertex>(); 
        // uint64_t this_update = pkt->getLE<uint64_t>(); 
        uint64_t this_update = pkt->getLE<uint64_t>(); 


        // DPRINTF(ActiveList, "%s: Write Value: %d, queue length: %d\n", __func__, this_update.id, queueLength());//->dst_id);
        DPRINTF(ActiveList, "%s: Write Address: %ld, Write Value %ld\n", __func__, pkt->getAddr(), this_update);//->dst_id);


        // c++ deque
        if(queueLength() >= queueSize){
            // full queue
            // send retry
            
        }
        else{
            //queue.emplace_back(std::make_tuple(this_update, this_update, this_update, curTick()));
            // coalescing could be done here
            // could check to see if the dst_id is already in the queue
            // if it is, then we need to update the valueMap and not emplace the new update to the queue

            // uncomment below block out

            // uint64_t my_dst_id = this_update.id;
            // std::tuple<uint64_t, Tick> mapAddr = std::make_tuple(my_dst_id, curTick());
            // queue.emplace_back(mapAddr); // need to figure out how to get the dst_id, I removed currtick() for now
            // valueMap[my_dst_id] = this_update; // error here
            valueMap.emplace_back((uint64_t)this_update);

        }

        if (pkt->needsResponse()) {
            pkt->makeResponse();
            corePorts.sendTimingResp(pkt);
        }



    }

    void
    ActiveList::processNextReadEvent()
    {

        // if(corePorts.needSendRetryReq){
        //     corePorts.needSendRetryReq = false;
        //     corePorts.sendRetryReq();
        // }
        PacketPtr pkt = pkt_read_queue.front();
        pkt_read_queue.pop_front();

        if (queueLength() < 1){
            //DPRINTF(ActiveList, "Tried reading empty queue\n");
            //  pkt->setLE(Vertex(0, 0, 0, 0, 0));
            // pkt->setLE((uint64_t)0);
            pkt->setLE((uint64_t)0);


            // How to send retry?
        }
        else{
            // std::tuple<uint64_t, Tick> mapAddr = queue.front();
            // queue.pop_front();
            // // pkt->setLE(valueMap.front());
            DPRINTF(ActiveList, "%s: Read Value: %d, queueLength: %d\n", __func__, valueMap.front(), queueLength());
            // DPRINTF(ActiveList, "%s: Read Value:, queueLength: %d\n", __func__,  queueLength());

            // pkt->setLE(valueMap.front());
            pkt->setLE(valueMap[alternate_r]);
            valueMap.erase(valueMap.begin()+alternate_r);// error here // get<0> is temporary

            alternate_r = alternate_r ^ 1;

            // valueMap.pop_front();// error here // get<0> is temporary
            // DPRINTF(ActiveList, "%s: Read Value: %d, queueLength: %d\n", __func__, std::get<0>(mapAddr), queueLength());

            
        }
        
        // DPRINTF(ActiveList, "%s: Returned Value: %d\n", __func__, pkt->getLE<uint32_t>());
        checkRetryReq();
        // if(corePorts.needSendRetryReq){
        //     corePorts.needSendRetryReq = false;
        //     corePorts.sendRetryReq();
        // }

        if (pkt->needsResponse()) {
            pkt->makeResponse();
            //pkt->setLE(((uint32_t)99999));

            corePorts.sendTimingResp(pkt);
        }

    }

    void
    ActiveList::checkRetryReq()
    {
        //DPRINTF(ActiveList, "%s: checking retry:\n", __func__);
        corePorts.checkRetryReq();
    }


    AddrRangeList
    ActiveList::RespPort::getAddrRanges() const
    {
        return owner->getAddrRanges();
    }


    void
    ActiveList::RespPort::checkRetryReq()
    {
        // DPRINTF(ActiveList, "%s: checking retry: %d\n", __func__, needSendRetryReq);
        if (needSendRetryReq) {
            DPRINTF(ActiveList, "%s: sending retry!\n", __func__);
            needSendRetryReq = false;
            sendRetryReq();
        }
    }

    bool
    ActiveList::RespPort::recvTimingReq(PacketPtr pkt)
    {
       // DPRINTF(ActiveList, "%s: Port Received Request\n", __func__);
        
        if (!owner->recvTimingReq(pkt)) {
            DPRINTF(ActiveList, "%s: Port failed to Receive Request\n", __func__);
            needSendRetryReq = true;
            return false;
        }

        return true;
    }

    Tick
    ActiveList::RespPort::recvAtomic(PacketPtr pkt)
    {
        panic("recvAtomic unimpl.");
    }

    void
    ActiveList::RespPort::recvFunctional(PacketPtr pkt)
    {
        owner->recvFunctional(pkt);
    }

    void
    ActiveList::RespPort::recvRespRetry()
    {
        panic("recvRespRetry from response port is called.");
    }

    Port&
    ActiveList::getPort(const std::string& if_name, PortID idx)
    {
        if (if_name == "cpu_side" || (idx == 0)){
            return corePorts;
        }
        else if (if_name == "mem_side" || (idx == 1)){
            return fakePort;
        }
        else {
            return ClockedObject::getPort(if_name, idx);
        }
       
    }



    void
    ActiveList::ReqPort::sendPacket(PacketPtr pkt)
    {
        panic_if(blockedPacket != nullptr,
                "Should never try to send if blocked!");
        // If we can't send the packet across the port, store it for later.
        if (!sendTimingReq(pkt))
        {
            DPRINTF(ActiveList, "%s: Packet is blocked.\n", __func__);
            blockedPacket = pkt;
        }
    }

    bool
    ActiveList::ReqPort::recvTimingResp(PacketPtr pkt)
    {
        panic("recvTimingResp called on the request port.");
    }

    void
    ActiveList::ReqPort::recvReqRetry()
    {
        panic_if(blockedPacket == nullptr,
                "Received retry without a blockedPacket.");

        // DPRINTF(ActiveList, "%s: rec.\n", __func__);
        PacketPtr pkt = blockedPacket;
        blockedPacket = nullptr;
        sendPacket(pkt);
        // if (blockedPacket == nullptr) {
        //     owner->recvReqRetry();
        // }
    }

//     ActiveList::ActiveListStats::ActiveListStats(ActiveList& _msgqueue):
//     statistics::Group(&_msgqueue), msgqueue(_msgqueue),
//     ADD_STAT(numMessagesReceived, statistics::units::Count::get(),
//               "Number of inocoming messages for this msgqueue."),
//     ADD_STAT(maxQueueLength, statistics::units::Count::get(),
//               "Largest value the queue reached during runtime")
// {
// }

// void
// ActiveList::ActiveListStats::regStats()
// {
//     using namespace statistics;



//     // vertexReadLatency.init(64);
//     // updateQueueLatency.init(64);

// }


}