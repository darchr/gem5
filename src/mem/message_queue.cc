#include "mem/message_queue.hh"
#include "sim/stats.hh"
#include <cstdio>

// /src/mem/message_queue.hh

namespace gem5{



    MessageQueue::MessageQueue(const MessageQueueParams &p) :
        ClockedObject(p), queueSize(p.queueSize), myRange(p.myRange), corePorts(name() + ".cpu_side", this), fakePort(name() + ".mem_side", this), finished_addr(p.finished_addr),  nextReadEvent([this]{ processNextReadEvent(); }, name()),
        nextWriteEvent([this] { processNextWriteEvent(); }, name()) //, stats(*this))
        {
            DPRINTF(MessageQueue, "%s: port name: %s  AddrRange: %d - %d, Addr of finished:%d\n", __func__,  (name() + ".cpu_side"), p.myRange.start(), p.myRange.end(), p.finished_addr);
            // DPRINTF(MessageQueue, "%s: Response_Port addr_range end: %s:%s\n", __func__,  corePorts.getAddrRanges().front().front, corePorts.getAddrRanges().front().end);

            //std::cout << "corePorts size: " << corePorts.getAddrRanges().front().size() << "\n";

            
        }

        PacketPtr
        MessageQueue::createWritePacket(Addr addr, const uint8_t data)
        {
            // Create new request
            int requestorID = 101;

            RequestPtr req = std::make_shared<Request>(addr, sizeof(uint8_t), 0,
                                                    requestorID);
            req->setPC(((Addr)requestorID) << 2);
            auto cmd = MemCmd::WriteReq;
            
            PacketPtr pkt = new Packet(req, cmd);
            pkt->allocate();
    
            pkt->setLE<uint8_t>(data);
            // uint8_t* pkt_data = new uint8_t[req->getSize()];
            // pkt_data[0] = data;
            
            // pkt->dataDynamic(pkt_data);
            // printf("finished packet created: Constptr = %p, size = %d\n", pkt->getConstPtr<uint8_t>(), req->getSize());
            // printf("pkt_data pointer = %p, data = %d\n", pkt_data, data);
            // printf("printed all necessary data\n");
            // uint8_t* pkt_data = new uint8_t[req->getSize()];
            // std::fill_n(pkt_data, req->getSize(), data[0]);
            // pkt->dataDynamic(pkt_data);

            return pkt;

        }

    bool
    MessageQueue::recvTimingReq(PacketPtr pkt)
    {
        //Addr this_addr = pkt->getAddr();
        //DPRINTF(MessageQueue, "%s: isWrite(): %d, Message Queue size = %d, Cmd = %s\n", __func__,  pkt->isWrite(), queueLength(), pkt->cmdString());

        if(pkt->isWrite()){

            if(queueLength() >= queueSize){
            // full queue

                DPRINTF(MessageQueue, "%s: Tried to write to full queue\n", __func__);
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
                // DPRINTF(MessageQueue, "%s: Read Value: %d\n", __func__, std::get<0>(queue.front()));
                // // pkt->setData(std::get<0>(queue.front()) + 15);
                // //pkt->setLE(std::get<0>(queue.front()) + 15);
                // pkt->setLE(15);
                // //queue.pop_front();
                // DPRINTF(MessageQueue, "%s: Returned Value: %d\n", __func__, pkt->getLE<uint32_t>());
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
    // MessageQueue::recvAtomic(PacketPtr pkt){

    // }

    void 
    MessageQueue::recvFunctional(PacketPtr pkt){

        DPRINTF(MessageQueue, "%s: isWrite(): %d, Cmd = %s\n", __func__,  pkt->isWrite(), pkt->cmdString());
        
        // Commented out to try having port write to done variable 
        // fakePort.sendFunctional(pkt);
    
    
        //panic("recvFunctional unimpl.");

    }

    void
    MessageQueue::init()
    {
        corePorts.sendRangeChange();
        // fakePort.recvRangeChange();
    }


    void
    MessageQueue::processNextWriteEvent()
    {
        //DPRINTF(MessageQueue, "in processNextWriteEvent \n");

        PacketPtr pkt = pkt_write_queue.front();
        pkt_write_queue.pop_front();
        //DPRINTF(MessageQueue, "Before reading packet\n");

        Update this_update = pkt->getLE<Update>(); 

        DPRINTF(MessageQueue, "%s: Write Value: %d, queue length: %d\n", __func__, this_update.dst_id, queueLength());//->dst_id);


        // c++ deque
        if(queueLength() >= queueSize){
            // full queue
            // send retry
            
        }
        else{
            //queue.emplace_back(std::make_tuple(this_update, this_update, this_update, curTick()));
             if(queueLength() == 0){
                // DPRINTF(MessageQueue, "%s: finished_addr = %d\n", __func__, finished_addr);
                // uint8_t* finished;
                
                // uint8_t* finished;
                // finished = 0;
                PacketPtr pkt = createWritePacket(finished_addr, 0);
                // printf("finished packet created: Constptr = %p\n", pkt->getConstPtr<uint8_t>());
                fakePort.sendPacket(pkt);
                DPRINTF(MessageQueue, "%s: queue is newly non-empty writing 0 to finished\n", __func__);
                
             }

            // coalescing could be done here
            // could check to see if the dst_id is already in the queue
            // if it is, then we need to update the valueMap and not emplace the new update to the queue

            // uncomment below block out
            uint64_t my_dst_id = this_update.dst_id;
            std::tuple<uint64_t, Tick> mapAddr = std::make_tuple(my_dst_id, curTick());
            queue.emplace_back(mapAddr); // need to figure out how to get the dst_id, I removed currtick() for now
            valueMap[my_dst_id] = this_update;

        }

        if (pkt->needsResponse()) {
            pkt->makeResponse();
            corePorts.sendTimingResp(pkt);
        }



    }

    void
    MessageQueue::processNextReadEvent()
    {

        // if(corePorts.needSendRetryReq){
        //     corePorts.needSendRetryReq = false;
        //     corePorts.sendRetryReq();
        // }
        PacketPtr pkt = pkt_read_queue.front();
        pkt_read_queue.pop_front();

        if (queueLength() < 1){
            //DPRINTF(MessageQueue, "Tried reading empty queue\n");
             pkt->setLE(Update(0xFFFF, 0xFFFF));
            // How to send retry?
        }
        else{
            std::tuple<uint64_t, Tick> mapAddr = queue.front();
            queue.pop_front();
            pkt->setLE(valueMap[std::get<0>(mapAddr)]); // get<0> is temporary
            DPRINTF(MessageQueue, "%s: Read Value: %d, queueLength: %d\n", __func__, std::get<0>(mapAddr), queueLength());
              if (queueLength() == 0){
                DPRINTF(MessageQueue, "%s: queue is newly empty writing 1 to finished\n", __func__);
                PacketPtr pkt = createWritePacket(finished_addr, 1);
                fakePort.sendPacket(pkt);
              }
        }
        
        // DPRINTF(MessageQueue, "%s: Returned Value: %d\n", __func__, pkt->getLE<uint32_t>());
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
    MessageQueue::checkRetryReq()
    {
        //DPRINTF(MessageQueue, "%s: checking retry:\n", __func__);
        corePorts.checkRetryReq();
    }


    AddrRangeList
    MessageQueue::RespPort::getAddrRanges() const
    {
        return owner->getAddrRanges();
    }


    void
    MessageQueue::RespPort::checkRetryReq()
    {
        // DPRINTF(MessageQueue, "%s: checking retry: %d\n", __func__, needSendRetryReq);
        if (needSendRetryReq) {
            DPRINTF(MessageQueue, "%s: sending retry!\n", __func__);
            needSendRetryReq = false;
            sendRetryReq();
        }
    }

    bool
    MessageQueue::RespPort::recvTimingReq(PacketPtr pkt)
    {
       // DPRINTF(MessageQueue, "%s: Port Received Request\n", __func__);
        
        if (!owner->recvTimingReq(pkt)) {
            DPRINTF(MessageQueue, "%s: Port failed to Receive Request\n", __func__);
            needSendRetryReq = true;
            return false;
        }

        return true;
    }

    Tick
    MessageQueue::RespPort::recvAtomic(PacketPtr pkt)
    {
        panic("recvAtomic unimpl.");
    }

    void
    MessageQueue::RespPort::recvFunctional(PacketPtr pkt)
    {
        owner->recvFunctional(pkt);
    }

    void
    MessageQueue::RespPort::recvRespRetry()
    {
        panic("recvRespRetry from response port is called.");
    }

    Port&
    MessageQueue::getPort(const std::string& if_name, PortID idx)
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
    MessageQueue::ReqPort::sendPacket(PacketPtr pkt)
    {
        panic_if(blockedPacket != nullptr,
                "Should never try to send if blocked!");
        // If we can't send the packet across the port, store it for later.
        DPRINTF(MessageQueue, "%s: sending functional packet: %s, Addr = %d\n", __func__, pkt->print(), pkt->getAddr());
        sendFunctional(pkt);
        // if (!sendTimingReq(pkt))
        // {
        //     DPRINTF(MessageQueue, "%s: Packet is blocked.\n", __func__);
        //     blockedPacket = pkt;
        // }
    }

    bool
    MessageQueue::ReqPort::recvTimingResp(PacketPtr pkt)
    {
        panic("recvTimingResp called on the request port.");
    }

    void
    MessageQueue::ReqPort::recvReqRetry()
    {
        panic_if(blockedPacket == nullptr,
                "Received retry without a blockedPacket.");

        // DPRINTF(MessageQueue, "%s: rec.\n", __func__);
        PacketPtr pkt = blockedPacket;
        blockedPacket = nullptr;
        sendPacket(pkt);
        // if (blockedPacket == nullptr) {
        //     owner->recvReqRetry();
        // }
    }

//     MessageQueue::MessageQueueStats::MessageQueueStats(MessageQueue& _msgqueue):
//     statistics::Group(&_msgqueue), msgqueue(_msgqueue),
//     ADD_STAT(numMessagesReceived, statistics::units::Count::get(),
//               "Number of inocoming messages for this msgqueue."),
//     ADD_STAT(maxQueueLength, statistics::units::Count::get(),
//               "Largest value the queue reached during runtime")
// {
// }

// void
// MessageQueue::MessageQueueStats::regStats()
// {
//     using namespace statistics;



//     // vertexReadLatency.init(64);
//     // updateQueueLatency.init(64);

// }


}