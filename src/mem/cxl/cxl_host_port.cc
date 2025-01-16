#include "mem/cxl/cxl_host_port.hh"

#include "mem/ruby/protocol/CXL/CXLH2DRequestType.hh"

namespace gem5
{

CXLHostPort::CXLHostPort(const Params &p)
    : SimObject(p),
        rubyController(p.controller)
{
}

void
CXLHostPort::init()
{
    assert(rubyController != nullptr);
    mandatoryQueue = rubyController->getMandatoryQueue();
}

bool
CXLHostPort::initiateMemoryRequest(PacketPtr pkt)
{
    // Check to make sure we have room in our outstanding request buffer

    // Get the device that we should set as the destination

    // Create a CXL request packet
    CXLH2DRequestType cxl_pkt;
    cxl_pkt.m_addr = pkt->getPhysAddr();

    if (pkt->isRead()) {
        cxl_pkt.m_Type = CXLH2DRequestType_RdShared;
    }

    mandatoryQueue->enqueue(cxl_pkt, clockEdge(), 0, false, false);
}

void
CXLHostPort::setInvalidationCallback(std::function<void(Addr)> callback)
{
    invalidationCallback = callback;
}

bool
CXLHostPort::initiateInvalidationAck(Addr addr)
{
    panic("Not implemented yet");
}

}
