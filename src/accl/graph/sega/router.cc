#include "accl/graph/sega/router.hh"

#include "accl/graph/sega/mpu.hh"
#include "debug/Router.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"

namespace gem5
{

Router::Router(const RouterParams &params) :
    ClockedObject(params),
    owner(nullptr)
{
    DPRINTF(Router, "Router created\n");
    for (int i = 0;
        i < params.port_in_ports_connection_count; ++i) {
        inPorts.emplace_back(this,
            name() + ".in_ports" + std::to_string(i), i);
    }

    for (int i = 0;
        i < params.port_out_ports_connection_count; ++i) {
        outPorts.emplace_back(this,
            name() + ".out_ports" + std::to_string(i), i);
    }

    DPRINTF(Router, "Ports created\n");
}


Router::~Router()
{
}

Port&
Router::getPort(const std::string& if_name, PortID idx)
{
    if (if_name == "in_ports") {
        return inPorts[idx];
    } else if (if_name == "out_ports") {
        return outPorts[idx];
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

void
Router::init()
{
    for (int i = 0; i < inPorts.size(); i++){
        inPorts[i].sendRangeChange();
    }
}

void
Router::RouterResponsePort::sendPacket(PacketPtr pkt)
{
    if (blocked()) {
        DPRINTF(Router, "Send blocked - pkt queued\n");
        assert(blockedPacket == nullptr);
        blockedPacket = pkt;
        return;
    }

    if (!sendTimingResp(pkt)) {
        DPRINTF(Router, "Send failed - pkt queued\n");
        blockedPacket = pkt;
    }
}

AddrRangeList
Router::RouterResponsePort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

bool
Router::RouterResponsePort::recvTimingReq(PacketPtr pkt)
{
    if (!owner) {
        DPRINTF(Router, "Error: owner not set\n");
        return false;
    }
    DPRINTF(Router, "Received request at cycle %llu\n", curTick());

    // Schedule sending to output ports after 10 cycles
    Tick delay = 10 * owner->clockPeriod();


    owner->schedule(
        new EventFunctionWrapper(
            [this, pkt]() {
                owner->outPorts[this->id()].sendPacket(pkt);
            },
            owner->name() + ".forward", true),
        curTick() + delay
    );

    return true;
}

Tick
Router::RouterResponsePort::recvAtomic(PacketPtr pkt)
{
    DPRINTF(Router, "Received atomic request\n");

    // Add 10 cycles of delay
    Tick delay = 10 * owner->clockPeriod();

    owner->outPorts[this->id()].sendAtomic(pkt);

    return delay;
}

void
Router::RouterResponsePort::recvFunctional(PacketPtr pkt)
{
    DPRINTF(Router, "Received functional request\n");
    owner->outPorts[this->id()].sendFunctional(pkt);
}

void
Router::RouterResponsePort::recvRespRetry()
{
    DPRINTF(Router, "Received response retry\n");
    if (blocked()) {
        PacketPtr pkt = blockedPacket;
        blockedPacket = nullptr;
        sendPacket(pkt);
    }
}

void
Router::RouterResponsePort::sendRetryReq()
{
    DPRINTF(Router, "Sending retry request\n");
    if (needRetry) {
        needRetry = false;
        sendRetryReq();
    }
}

// RouterRequestPort Implementation
void
Router::RouterRequestPort::sendPacket(PacketPtr pkt)
{
    if (blocked()) {
        DPRINTF(Router, "Send blocked - pkg queued\n");
        assert(blockedPacket == nullptr);
        blockedPacket = pkt;
        return;
    }

    if (!sendTimingReq(pkt)) {
        DPRINTF(Router, "Send failed - pkg queued\n");
        blockedPacket = pkt;
    }
}

bool
Router::RouterRequestPort::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(Router, "Received timing response\n");
    return true;
}

void
Router::RouterRequestPort::recvReqRetry()
{
    DPRINTF(Router, "Received request retry\n");
    if (blocked()) {
        PacketPtr pkt = blockedPacket;
        blockedPacket = nullptr;
        sendPacket(pkt);
    }
}

void
Router::RouterRequestPort::sendRetryResp()
{
    DPRINTF(Router, "Sending retry response\n");
    if (needRetry) {
        needRetry = false;
        sendRetryResp();
    }
}

void
Router::registerMPU(MPU* mpu)
{
    owner = mpu;
}

AddrRangeList
Router::getAddrRanges()
{
    return owner->getAddrRanges();
}

} // namespace gem5
