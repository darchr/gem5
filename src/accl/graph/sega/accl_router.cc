#include "accl/graph/sega/accl_router.hh"

#include "accl/graph/sega/mpu.hh"
#include "debug/AcclRouter.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"

namespace gem5
{

AcclRouter::AcclRouter(const AcclRouterParams &params) :
    ClockedObject(params),
    crosspointDelay(params.crosspoint_delay),
    mergerDelay(params.merger_delay),
    splitterDelay(params.splitter_delay),
    circuitVariability(params.circuit_variability),
    crosspointSetupTime(params.crosspoint_setup_time),
    variabilityCountingNetwork(params.variability_counting_network),
    rotateActiveOutPortEvent(
        [this]() { this->rotateActiveOutPort(); },
        name() + ".rotateActiveOutPort", true
    ),
    packetsProcessed(this,
        "packetsProcessed",
        "Total number of packets processed by the AcclRouter"),
    valueLatency(this,
        "valueLatency",
        "Distribution of value latency (ps)")
    //stats(*this)
{
    valueLatency.init(64);
    // stats.regStats();
    DPRINTF(AcclRouter, "AcclRouter created\n");

    // assert that none of the delays are negative
    assert(crosspointDelay >= 0);
    assert(mergerDelay >= 0);
    assert(splitterDelay >= 0);
    assert(circuitVariability >= 0);
    assert(crosspointSetupTime >= 0);
    assert(variabilityCountingNetwork >= 0);

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

    DPRINTF(AcclRouter, "Ports created\n");

    // Set radix based on the number of in and out ports
    radix = params.port_in_ports_connection_count +
            params.port_out_ports_connection_count;
    DPRINTF(AcclRouter, "AcclRouter radix: %lu\n", radix);

    // Copy the MPUs and register the AcclRouter with them.
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
                DPRINTF(AcclRouter,
                    "Startup: Assigned in port %lu to MPU %s\n",
                    static_cast<unsigned long>(portIndex),
                    mpuVector[mpuIndex]->name()
                );
                portIndex++;
            }
        }
        // Safety: warn if any ports remain unassigned (should not happen)
        while (portIndex < totalInPorts) {
            DPRINTF(AcclRouter,
                "Startup: Warning - No MPU available for in port %lu\n",
                static_cast<unsigned long>(portIndex));
            portIndex++;
        }
    } else {
        for (size_t i = 0; i < inPorts.size(); ++i) {
            DPRINTF(AcclRouter,
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
                DPRINTF(AcclRouter,
                    "Startup: Assigned out port %lu to MPU %s\n",
                    static_cast<unsigned long>(portIndex),
                    mpuVector[mpuIndex]->name());
                portIndex++;
            }
        }
        while (portIndex < totalOutPorts) {
            DPRINTF(AcclRouter,
                "Startup: Warning - No MPU available for out port %lu\n",
                static_cast<unsigned long>(portIndex));
            portIndex++;
        }
    } else {
        for (size_t i = 0; i < outPorts.size(); ++i) {
            DPRINTF(AcclRouter,
                "Startup: Warning - No MPU available for out port %lu\n",
                static_cast<unsigned long>(i));
        }
    }

    // Compute timing parameters based on the network delays
    computeTimingParameters();

}


AcclRouter::~AcclRouter()
{
}

Port&
AcclRouter::getPort(const std::string& if_name, PortID idx)
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
AcclRouter::startup()
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
        DPRINTF(AcclRouter, "Address range: %#x - %#x, MPU: %s\n",
                iter->first.start(), iter->first.end(), iter->second->name());
    }

    // setCurTick(curTick()+1);
    // Initialize active out port rotation
    schedule(
        rotateActiveOutPortEvent,
        curTick() + connectionWindow
    );
}

void
AcclRouter::AcclRouterResponsePort::sendPacket(PacketPtr pkt)
{
    if (blocked()) {
        DPRINTF(AcclRouter, "Send blocked - pkt queued\n");
        assert(blockedPacket == nullptr);
        blockedPacket = pkt;
        return;
    }

    if (!sendTimingResp(pkt)) {
        DPRINTF(AcclRouter, "Send failed - pkt queued\n");
        blockedPacket = pkt;
    }
}

AddrRangeList
AcclRouter::AcclRouterResponsePort::getAddrRanges() const
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

// Calculate the time slot based on network component delays
void
AcclRouter::assignTimeSlot()
{
    // Adjust setup time considering circuit variability
    double se_adjusted = std::max(0.0,
        crosspointSetupTime - circuitVariability
    );

    // Calculate time slot considering delays of various network components
    // NOTE: gem5's Tick is based on picoseconds, so this calculation
    // assumes the delay parameters are also in picoseconds.
    double calculated_time_slot = std::ceil(circuitVariability * (
        crosspointDelay + se_adjusted +
        splitterDelay * (radix - 1) +
        mergerDelay * (radix - 1) +
        variabilityCountingNetwork
    ) + crosspointSetupTime);

    // Round up the calculated time slot and assign it
    this->timeSlot = calculated_time_slot;
}

// Compute key network parameters like time slot and connection window
void
AcclRouter::computeTimingParameters()
{
    // Calculate and assign the time slot
    assignTimeSlot();
    DPRINTF(AcclRouter, "Calculated time slot: %d ps\n", timeSlot);
    DPRINTF(AcclRouter, "System clock period: %lu ps\n", clockPeriod());

    if (timeSlot <= 0) {
        fatal("AcclRouter %s: Time slot (%lu) must be greater than 0!\n",
            name(), timeSlot);
    }
    if (timeSlot > clockPeriod()) {
        fatal("AcclRouter %s: Time slot (%lu) is greater than "
            "clock period (%lu)!\n",
            name(), timeSlot, clockPeriod());
    }

    // Calculate the number of time slots possible
    // given the clock period and time slot duration
    // Round up to the nearest whole number
    rlTimeSlots = std::ceil(
        static_cast<double>(clockPeriod()) / timeSlot
    );
    DPRINTF(AcclRouter,
        "Possible time slots per clock cycle: %lu\n",
        rlTimeSlots
    );

    // Calculate and assign the connection window
    connectionWindow = rlTimeSlots * timeSlot;
    DPRINTF(AcclRouter,
        "Calculated connection window: %d ps\n",
        connectionWindow
    );

    // Print initial active out port index
    DPRINTF(AcclRouter,
        "Initial active out port: %u\n",
        static_cast<unsigned>(activeIndex)
    );
}

PortID
AcclRouter::getMappedOutPort(PortID in_id) const
{
    // simple one-to-one rotation: out = (in + activeIndex) % N
    return (in_id + activeIndex) % outPorts.size();
}

Tick
AcclRouter::ticksUntilPortActive(PortID in_id, PortID out_id) const
{
    if (outPorts.empty())
        return 0;                              // degenerate case

    const unsigned N = outPorts.size();
    const unsigned desiredRelIdx = (out_id + N - in_id) % N;
    const unsigned currentRelIdx = activeIndex % N;

    const unsigned windowsAway =
        (desiredRelIdx + N - currentRelIdx) % N;  // 0 ⇒ already active

    return static_cast<Tick>(windowsAway) * connectionWindow;
}

bool
AcclRouter::checkPortMapping(PortID in_id, PortID out_id) const
{
    return getMappedOutPort(in_id) == out_id;
}

void
AcclRouter::printPortMappings() const
{
    for (PortID in_id = 0; in_id < inPorts.size(); ++in_id) {
        PortID out_id = getMappedOutPort(in_id);
        DPRINTF(AcclRouter,
                "  In port %u -> Out port %u  [%s]",
                in_id, out_id,
                checkPortMapping(in_id, out_id) ? "OK" : "ERR");
    }
}

void
AcclRouter::notifyMPUDone(MPU*)
{
    /* have all MPUs reached the done state? */
    for (auto m : mpuVector)
        if (!m->done())
            return;                 // at least one still working

    /* first time we realise everything is finished */
    if (rotateActiveOutPortEvent.scheduled())
        deschedule(rotateActiveOutPortEvent);

    DPRINTF(AcclRouter, "All MPUs done - rotation halted.\n");
}

// Rotate the active out port index each connection window and print it
void AcclRouter::rotateActiveOutPort()
{
    // 1) advance
    if (!outPorts.empty()) {
        activeIndex = (activeIndex + 1) % outPorts.size();
        DPRINTF(AcclRouter,
            "Rotated active out port to: %u\n",
            static_cast<unsigned>(activeIndex)
        );
    }
    printPortMappings();

    // 2) check for finish
    bool allDone = true;
    for (auto m : mpuVector)
        if (!m->done()) { allDone = false; break; }
    if (allDone) {
        DPRINTF(AcclRouter, "All MPUs are done, stopping rotation.\n");
        return;
    }
    if (!rotateActiveOutPortEvent.scheduled()){
        schedule(rotateActiveOutPortEvent, curTick() + connectionWindow);
    }
}
bool
AcclRouter::AcclRouterResponsePort::recvTimingReq(PacketPtr pkt)
{
    if (!owner) {
        DPRINTF(AcclRouter, "Error: owner not set\n");
        return false;
    }
    DPRINTF(AcclRouter, "Received request at cycle %llu\n", curTick());
    owner->packetsProcessed++;

    MPU* mpu = owner->getMPUForInPort(this->id());
    if (!mpu) {
        DPRINTF(AcclRouter, "Error: No MPU assigned for in port %d\n",
            this->id());
        return false;
    }
    int outPortIndex = owner->getOutPortIndexForMPU(mpu);
    if (outPortIndex < 0) {
        DPRINTF(AcclRouter, "Error: No out port available for MPU %s\n",
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
        owner->valueLatency.sample(
            delay
        );
        return true;
    } else if (owner->mode == RouterMode::SRNOC) {
        // Send packet after 20 cycles for SRNoC mode
        int value = 0;
        if (pkt->getSize() >= sizeof(int) &&
            pkt->getConstPtr<int>() != nullptr) {
            value = *pkt->getConstPtr<int>();
            DPRINTF(AcclRouter, "Packet int payload: %d\n", value);
        }
        // if value is more than number of time slots, throw an error
        if (value < 0 || value >= owner->rlTimeSlots) {
            DPRINTF(AcclRouter,
                "Error: Value %d out of range for time slots %lu\n",
                value, owner->rlTimeSlots);
            fatal("Value %d out of range for time slots %lu\n",
                value, owner->rlTimeSlots);
            return false;
        }
        Tick value_delay = value * owner->timeSlot;
        Tick port_delay = owner->ticksUntilPortActive(this->id(),
                        outPortIndex);
        owner->schedule(
            new EventFunctionWrapper(
                [this, pkt, outPortIndex]() {
                    owner->outPorts[outPortIndex].sendPacket(pkt);
                },
                owner->name() + ".forward", true),
            curTick() + value_delay + port_delay
        );
        owner->valueLatency.sample(
            value_delay + port_delay
        );
        DPRINTF(AcclRouter, "Calculated delay: %llu\n",
            value_delay + port_delay);
        // Return true to indicate the packet was sent successfully
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
AcclRouter::AcclRouterResponsePort::recvAtomic(PacketPtr pkt)
{
    DPRINTF(AcclRouter, "Received atomic request\n");

    MPU* mpu = owner->getMPUForInPort(this->id());
    if (!mpu) {
        DPRINTF(AcclRouter, "Error: No MPU for in port %d\n", this->id());
        return 0;
    }
    int outPortIndex = owner->getOutPortIndexForMPU(mpu);
    if (outPortIndex < 0) {
        DPRINTF(AcclRouter, "Error: No out port for MPU %s\n", mpu->name());
        return 0;
    }

    Tick delay = 0;

    if (owner->mode == RouterMode::STATIC_DELAY) {
        delay = 10 * owner->clockPeriod();
    } else if (owner->mode == RouterMode::SRNOC) {
        // For SRNoC mode, we use a longer delay
        int value = 0;
        if (pkt->getSize() >= sizeof(int) &&
            pkt->getConstPtr<int>() != nullptr) {
            value = *pkt->getConstPtr<int>();
            DPRINTF(AcclRouter, "Packet int payload: %d\n", value);
        }
        // if value is more than number of time slots, throw an error
        if (value < 0 || value >= owner->rlTimeSlots) {
            DPRINTF(AcclRouter,
                "Error: Value %d out of range for time slots %lu\n",
                value, owner->rlTimeSlots);
            return false;
        }
        // delay is value times rlTimeSlots
        delay = value * owner->timeSlot +
                owner->ticksUntilPortActive(this->id(), outPortIndex);
        DPRINTF(AcclRouter, "Calculated delay: %llu\n", delay);
    }
    owner->outPorts[outPortIndex].sendAtomic(pkt);
    owner->valueLatency.sample(delay);
    return delay;
}

void
AcclRouter::AcclRouterResponsePort::recvFunctional(PacketPtr pkt)
{
    DPRINTF(AcclRouter, "Received functional request\n");
    MPU* mpu = owner->getMPUForInPort(this->id());
    if (!mpu) {
        DPRINTF(AcclRouter, "Error: No MPU for in port %d\n", this->id());
        return;
    }
    int outPortIndex = owner->getOutPortIndexForMPU(mpu);
    if (outPortIndex < 0) {
        DPRINTF(AcclRouter, "Error: No out port for MPU %s\n", mpu->name());
        return;
    }
    owner->outPorts[outPortIndex].sendFunctional(pkt);
}

void
AcclRouter::AcclRouterResponsePort::recvRespRetry()
{
    DPRINTF(AcclRouter, "Received response retry\n");
    if (blocked()) {
        PacketPtr pkt = blockedPacket;
        blockedPacket = nullptr;
        sendPacket(pkt);
    }
}

void
AcclRouter::AcclRouterResponsePort::sendRetryReq()
{
    DPRINTF(AcclRouter, "Sending retry request\n");
    if (needRetry) {
        needRetry = false;
        sendRetryReq();
    }
}

// AcclRouterRequestPort Implementation
void
AcclRouter::AcclRouterRequestPort::sendPacket(PacketPtr pkt)
{
    if (blocked()) {
        DPRINTF(AcclRouter, "Send blocked - pkg queued\n");
        assert(blockedPacket == nullptr);
        blockedPacket = pkt;
        return;
    }

    if (!sendTimingReq(pkt)) {
        DPRINTF(AcclRouter, "Send failed - pkg queued\n");
        blockedPacket = pkt;
    }
}

bool
AcclRouter::AcclRouterRequestPort::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(AcclRouter, "Received timing response\n");
    return true;
}

void
AcclRouter::AcclRouterRequestPort::recvReqRetry()
{
    DPRINTF(AcclRouter, "Received request retry\n");
    if (blocked()) {
        PacketPtr pkt = blockedPacket;
        blockedPacket = nullptr;
        sendPacket(pkt);
    }
}

void
AcclRouter::AcclRouterRequestPort::sendRetryResp()
{
    DPRINTF(AcclRouter, "Sending retry response\n");
    if (needRetry) {
        needRetry = false;
        sendRetryResp();
    }
}

// Assign an input port to an MPU
void
AcclRouter::assignInPortToMPU(PortID portId, MPU* mpu) {
    if (portId < inPortToMPU.size()) {
        inPortToMPU[portId] = mpu;
        DPRINTF(AcclRouter, "Assigned input port %d to MPU %s\n",
                portId, mpu->name());
    } else {
        DPRINTF(AcclRouter, "Error: Input port ID %d out of range\n", portId);
    }
}

// Assign an output port to an MPU
void
AcclRouter::assignOutPortToMPU(PortID portId, MPU* mpu) {
    if (portId < outPortToMPU.size()) {
        outPortToMPU[portId] = mpu;
        DPRINTF(AcclRouter, "Assigned output port %d to MPU %s\n",
                portId, mpu->name());
    } else {
        DPRINTF(AcclRouter, "Error: Output port ID %d out of range\n", portId);
    }
}

// Get the MPU associated with an input port
MPU*
AcclRouter::getMPUForInPort(PortID portId) const {
    if (portId < inPortToMPU.size()) {
        return inPortToMPU[portId];
    }
    return nullptr;
}

// Get the MPU associated with an output port
MPU*
AcclRouter::getMPUForOutPort(PortID portId) const {
    if (portId < outPortToMPU.size()) {
        return outPortToMPU[portId];
    }
    return nullptr;
}

int
AcclRouter::getOutPortIndexForMPU(MPU* mpu) const {
    for (size_t i = 0; i < outPortToMPU.size(); ++i) {
        if (outPortToMPU[i] == mpu)
            return i;
    }
    return -1;
}

void
AcclRouter::init()
{
}

// AcclRouter::AcclRouterStats::AcclRouterStats(AcclRouter& acclrouter)
//     : statistics::Group(&acclrouter), router(acclrouter),
//     // ADD_STAT(valueLatency, statistics::units::Count::get(),
//     //     "Distribution of value latency (ps)"),
//     ADD_STAT(packetsProcessed, statistics::units::Count::get(),
//         "Total number of packets processed by the AcclRouter")
// {
// }

// void
// AcclRouter::AcclRouterStats::regStats()
// {
//     using namespace statistics;

//     packetsProcessed.name("packetsProcessed")
//         .desc("Total number of packets processed by the AcclRouter");

//     // valueLatency.init(64);
//     // Group::regStats();
// }

// AcclRouter::AcclRouterStats::~AcclRouterStats() = default;

} // namespace gem5
