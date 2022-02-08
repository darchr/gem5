#include "accl/util.hh"

PacketPtr
getReadPacket(Addr addr, unsigned int size, RequestorID requestorId)
{
    RequestPtr req = std::make_shared<Request>(addr, size, 0, requestorId);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr)requestorId) << 2);

    // Embed it in a packet
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    pkt->allocate();

    return pkt;
}
