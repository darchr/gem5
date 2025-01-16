
#include "mem/cxl/cxl_controller.hh"

CXLController::CXLController(const Params &p)
    : SimObject(p),
      cxlOutputPort(p.cxl_output_port),
      hostInputPort(new HostSidePort(params.name + ".cpu_side", this)),
      memRanges(p.mem_ranges)
{
    cxlOutputPort->setInvalidationCallback(
        [this](Addr addr) { handleInvalidation(addr); });
}

Port &
CXLController::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "host_side") {
        return *hostInputPort;
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

void
CXLController::HostSidePort::sendPacket(PacketPtr pkt)
{
    DPRINTF(CXLController, "Sending response packet to host %s",
            pkt);
    if (!sendTimingResp(pkt)) {
        panic("Should never try to send if blocked!");
    }
}

AddrRangeList
CXLController::HostSidePort::getAddrRanges() const
{
    return owner->memRanges;
}

Tick
CXLController::HostSidePort::recvAtomic(PacketPtr pkt)
{
    panic("CXLController doesn't expect atomic packets");
}

void
CXLController::HostSidePort::recvFunctional(PacketPtr pkt)
{
    panic("CXLController doesn't expect functional packets");
}

bool
CXLController::HostSidePort::recvTimingReq(PacketPtr pkt)
{
    DPRINTF(CXLController, "Got request %s\n", pkt);

    // Just forward to the memory side
    return owner->cxlOutputPort->initiateMemoryRequest(pkt);
}

void
CXLController::HostSidePort::recvRespRetry()
{
    panic("CXLController doesn't expect response retries");
}

void
CXLController::handleInvalidation(Addr addr)
{
    panic("CXLController doesn't expect invalidation");
}
