#include "accl/graph/sega/router.hh"

#include "accl/graph/sega/mpu.hh"
#include "debug/Router.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"

namespace gem5
{

Router::Router(const RouterParams &params) :
    ClockedObject(params)
{
    DPRINTF(Router, "Router created\n");

    // Create and store in ports.
    for (int i = 0; i < params.port_in_ports_connection_count; ++i) {
        inPorts.emplace_back(this,
            name() + ".in_ports" + std::to_string(i), i);
        MPU* mpu = (i < params.mpu_vector.size() ?
            params.mpu_vector[i] : nullptr);
        inPortToMPU.push_back(mpu);
    }

    // Create and store out ports.
    for (int i = 0; i < params.port_out_ports_connection_count; ++i) {
        outPorts.emplace_back(this,
            name() + ".out_ports" + std::to_string(i), i);
        MPU* mpu = (i < params.mpu_vector.size() ?
            params.mpu_vector[i] : nullptr);
        outPortToMPU.push_back(mpu);
    }

    DPRINTF(Router, "Ports created\n");

    // Copy the MPUs and register the router with them.
    for (auto mpu : params.mpu_vector) {
        mpuVector.push_back(mpu);
        mpu->registerRouter(this);
    }

    // --- Assign input ports to MPUs in contiguous groups ---
    size_t numMPUs = mpuVector.size();
    if (numMPUs > 0) {
        // Calculate how many ports per MPU,
        // distributing any extra ports among the first MPUs.
        size_t totalInPorts = inPorts.size();
        size_t portsPerMPU = totalInPorts / numMPUs;
        size_t extra = totalInPorts % numMPUs;
        size_t portIndex = 0;

        for (size_t mpuIndex = 0; mpuIndex < numMPUs; ++mpuIndex) {
            size_t numPortsForThisMPU =
                portsPerMPU + (mpuIndex < extra ? 1 : 0);
            for (size_t j = 0; j < numPortsForThisMPU; ++j) {
                assignInPortToMPU(portIndex, mpuVector[mpuIndex]);
                DPRINTF(Router, "Startup: Assigned in port %lu to MPU %s\n",
                    static_cast<unsigned long>(portIndex),
                    mpuVector[mpuIndex]->name());
                portIndex++;
            }
        }
        // Safety: warn if any ports remain unassigned (should not happen)
        while (portIndex < totalInPorts) {
            DPRINTF(Router,
                "Startup: Warning - No MPU available for in port %lu\n",
                static_cast<unsigned long>(portIndex));
            portIndex++;
        }
    } else {
        for (size_t i = 0; i < inPorts.size(); ++i) {
            DPRINTF(Router,
                "Startup: Warning - No MPU available for in port %lu\n",
                static_cast<unsigned long>(i));
        }
    }

    // --- Assign output ports to MPUs in contiguous groups ---
    if (numMPUs > 0) {
        size_t totalOutPorts = outPorts.size();
        size_t portsPerMPU = totalOutPorts / numMPUs;
        size_t extra = totalOutPorts % numMPUs;
        size_t portIndex = 0;

        for (size_t mpuIndex = 0; mpuIndex < numMPUs; ++mpuIndex) {
            size_t numPortsForThisMPU =
                portsPerMPU + (mpuIndex < extra ? 1 : 0);
            for (size_t j = 0; j < numPortsForThisMPU; ++j) {
                assignOutPortToMPU(portIndex, mpuVector[mpuIndex]);
                DPRINTF(Router,
                    "Startup: Assigned out port %lu to MPU %s\n",
                    static_cast<unsigned long>(portIndex),
                    mpuVector[mpuIndex]->name());
                portIndex++;
            }
        }
        while (portIndex < totalOutPorts) {
            DPRINTF(Router,
                "Startup: Warning - No MPU available for out port %lu\n",
                static_cast<unsigned long>(portIndex));
            portIndex++;
        }
    } else {
        for (size_t i = 0; i < outPorts.size(); ++i) {
            DPRINTF(Router,
                "Startup: Warning - No MPU available for out port %lu\n",
                static_cast<unsigned long>(i));
        }
    }
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
Router::startup()
{
    for (auto mpu: mpuVector) {
        AddrRangeList localAddrRange = mpu->getAddrRanges();
        for (int i = 0; i < outPorts.size(); i++){
            AddrRangeList range_list = outPorts[i].getAddrRanges();
            assert(range_list.size() == 1);
            AddrRange range = outPorts[i].getAddrRanges().front();
            mpuAddrMap.insert(range, mpu);
        }
    }

    for (int i = 0; i < inPorts.size(); i++){
        inPorts[i].sendRangeChange();
    }

    // print mpuAddrMap
    for (auto iter = mpuAddrMap.begin(); iter != mpuAddrMap.end(); ++iter) {
        DPRINTF(Router, "Address range: %#x - %#x, MPU: %s\n",
                iter->first.start(), iter->first.end(), iter->second->name());
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
    AddrRangeList ranges;
    if (owner) {
        // Get the MPU assigned to this port
        MPU* mpu = owner->getMPUForInPort(this->id());
        if (mpu) {
            // Return the address ranges for this MPU
            ranges = mpu->getAddrRanges();
        }
    }
    return ranges;
}

bool
Router::RouterResponsePort::recvTimingReq(PacketPtr pkt)
{
    if (!owner) {
        DPRINTF(Router, "Error: owner not set\n");
        return false;
    }
    DPRINTF(Router, "Received request at cycle %llu\n", curTick());

    MPU* mpu = owner->getMPUForInPort(this->id());
    if (!mpu) {
        DPRINTF(Router, "Error: No MPU assigned for in port %d\n",
            this->id());
        return false;
    }
    int outPortIndex = owner->getOutPortIndexForMPU(mpu);
    if (outPortIndex < 0) {
        DPRINTF(Router, "Error: No out port available for MPU %s\n",
            mpu->name());
        return false;
    }

    if (owner->mode == RouterMode::STATIC_DELAY) {
        // Send the packet after a fixed delay
        Tick delay = 10 * owner->clockPeriod();
        owner->schedule(
            new EventFunctionWrapper(
                [this, pkt, outPortIndex]() {
                    owner->outPorts[outPortIndex].sendPacket(pkt);
                },
                owner->name() + ".forward", true),
            curTick() + delay
        );
        return true;
    }
    // Tick delay = 10 * owner->clockPeriod();
    // owner->schedule(
    //     new EventFunctionWrapper(
    //         [this, pkt, outPortIndex]() {
    //             owner->outPorts[outPortIndex].sendPacket(pkt);
    //         },
    //         owner->name() + ".forward", true),
    //     curTick() + delay
    // );

    // return true;
}

Tick
Router::RouterResponsePort::recvAtomic(PacketPtr pkt)
{
    DPRINTF(Router, "Received atomic request\n");

    MPU* mpu = owner->getMPUForInPort(this->id());
    if (!mpu) {
        DPRINTF(Router, "Error: No MPU for in port %d\n", this->id());
        return 0;
    }
    int outPortIndex = owner->getOutPortIndexForMPU(mpu);
    if (outPortIndex < 0) {
        DPRINTF(Router, "Error: No out port for MPU %s\n", mpu->name());
        return 0;
    }

    Tick delay = 0;

    if (owner->mode == RouterMode::STATIC_DELAY) {
        delay = 10 * owner->clockPeriod();
    }
    owner->outPorts[outPortIndex].sendAtomic(pkt);
    return delay;
}

void
Router::RouterResponsePort::recvFunctional(PacketPtr pkt)
{
    DPRINTF(Router, "Received functional request\n");
    MPU* mpu = owner->getMPUForInPort(this->id());
    if (!mpu) {
        DPRINTF(Router, "Error: No MPU for in port %d\n", this->id());
        return;
    }
    int outPortIndex = owner->getOutPortIndexForMPU(mpu);
    if (outPortIndex < 0) {
        DPRINTF(Router, "Error: No out port for MPU %s\n", mpu->name());
        return;
    }
    owner->outPorts[outPortIndex].sendFunctional(pkt);
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

// Assign an input port to an MPU
void
Router::assignInPortToMPU(PortID portId, MPU* mpu) {
    if (portId < inPortToMPU.size()) {
        inPortToMPU[portId] = mpu;
        DPRINTF(Router, "Assigned input port %d to MPU %s\n",
                portId, mpu->name());
    } else {
        DPRINTF(Router, "Error: Input port ID %d out of range\n", portId);
    }
}

// Assign an output port to an MPU
void
Router::assignOutPortToMPU(PortID portId, MPU* mpu) {
    if (portId < outPortToMPU.size()) {
        outPortToMPU[portId] = mpu;
        DPRINTF(Router, "Assigned output port %d to MPU %s\n",
                portId, mpu->name());
    } else {
        DPRINTF(Router, "Error: Output port ID %d out of range\n", portId);
    }
}

// Get the MPU associated with an input port
MPU*
Router::getMPUForInPort(PortID portId) const {
    if (portId < inPortToMPU.size()) {
        return inPortToMPU[portId];
    }
    return nullptr;
}

// Get the MPU associated with an output port
MPU*
Router::getMPUForOutPort(PortID portId) const {
    if (portId < outPortToMPU.size()) {
        return outPortToMPU[portId];
    }
    return nullptr;
}

int
Router::getOutPortIndexForMPU(MPU* mpu) const {
    for (size_t i = 0; i < outPortToMPU.size(); ++i) {
        if (outPortToMPU[i] == mpu)
            return i;
    }
    return -1;
}

void
Router::init()
{

}

} // namespace gem5
