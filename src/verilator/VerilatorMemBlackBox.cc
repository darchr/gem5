#include "VeriilatorMemBlackBox.hh"
#include "base/Logging.hh"

void VerilatorMemBlackBox::sendPacket(PacketPtr pkt){
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }

}

bool VerilatorMemBlackBox::recvTimingResp( PacketPtr pkt )
{
    DPRINTF(Verilator, "MEMORY RESPONSE RECIEVED\n");
    return owner->handleResponse(pkt);

}

void VerilatorMemBlackBox::recvReqRetry(){
    assert(blockedPacket != nullptr);

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    sendPacket(pkt);

}


//need to rewrite after changing memory.scala
VerilatorMemBlackBox::VerilatorMemBlackBox(
        VerilatorMemBlackBoxParams *params) :
    MemObject(params),
    instData(params -> name + ".instPort", this),
    dataPort(params -> name + ".dataPort", this)
{
}

VerilatorMemBlackBox::~VerilatorMemBlackBox()
{
}

void VerilatorMemBlackBox::doFetch()
{

     RequestPtr ifetch_req = std::make_shared<Request>(
        blkbox.Top__DOT__tile_,
        4,0x100,0);
     DPRINTF(Verilator, "Sending fetch for addr (pa: %#x)\n",
               req->getPaddr());
     PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
     DPRINTF(Verilator, " -- pkt addr: %#x\n", pkt->getAddr());
     pkt->allocate();
     instPort.sendPacket(pkt);

}

void VerilatorMemBlackBox::doMem(const RequestPtr &req, uint8_t *data,
        bool read )
{
    //need more stuff here
    PacketPtr pkt = read ? Packet::createRead(req) : Packet::createWrite(req);
    pkt->allocate();
    pkt->setData(data);
    delete[] data;

    dataPort.sendPacket(pkt);
}

bool VerilatorMemBlackBox::handleResponse( PacketPtr pkt )
{
 DPRINTF(Verilator, "Got response for addr %#x\n", pkt->getAddr());
    const uint32_t *t =  pkt->getConstPtr<uint32_t>();
    if (pkt->req->isInstFetch()) {

    } else  if (pkt->isRead()){
        //set packet data to sodor dmem data signal
        const uint32_t *tmp = pkt->getConstPtr<uint32_t>();
    }
    return true;

}

BaseMasterPort& VerilatorMemBlackBox::getMasterPort(
        const std::string& if_name, PortID idx )
{
 panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

    // This is the name from the Python SimObject declaration (SimpleMemobj.py)
    if (if_name == "instPort") {
        return instPort;
    } else if (if_name == "dataPort") {
        return dataPort;
    } else {
        // pass it along to our super class
        return MemObject::getMasterPort(if_name, idx);
    }

}
