#include "network/SuperNetwork.hh"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>

#include "debug/SuperNetwork.hh"
#include "network/NetworkScheduler.hh"
#include "sim/sim_exit.hh"
#include "sim/stats.hh"
#include "sim/system.hh"

namespace gem5 {

// Hardware Constants (from SRNoC paper)
namespace HardwareConstants {
    // Time in picoseconds
    constexpr double CROSSPOINT_DELAY = 4.1;
    constexpr double MERGER_DELAY = 8.84;
    constexpr double SPLITTER_DELAY = 2.06;
    constexpr double CIRCUIT_VARIABILITY = 1.2;
    constexpr double VARIABILITY_COUNTING_NETWORK = 4.38;
    constexpr double CROSSPOINT_SETUP_TIME = 8;

    // Static power in microwatts
    constexpr double SPLITTER_STATIC_POWER = 5.98;
    constexpr double MERGER_STATIC_POWER = 5;
    constexpr double CROSSPOINT_STATIC_POWER = 7.9;
    // 1x4 counting network
    constexpr double COUNTING_NETWORK_STATIC_POWER = 66.82;
    constexpr double TFF_STATIC_POWER = 10.8;

    // Active power in nanowatts
    constexpr double SPLITTER_ACTIVE_POWER = 83.2;
    constexpr double MERGER_ACTIVE_POWER = 69.6;
    constexpr double CROSSPOINT_ACTIVE_POWER = 60.7;
    // 1x4 counting network
    constexpr double COUNTING_NETWORK_ACTIVE_POWER = 163;
    constexpr double TFF_ACTIVE_POWER = 105.6;

    // Number of JJs
    constexpr int SPLITTER_JJ = 3;
    constexpr int MERGER_JJ = 5;
    constexpr int CROSSPOINT_JJ = 13;
    // 1x4 counting network
    constexpr int COUNTING_NETWORK_JJ = 60;
    constexpr int TFF_JJ = 10;
}

SuperNetwork::SuperNetwork(const SuperNetworkParams& params) :
    ClockedObject(params),
    maxPackets(params.max_packets),
    schedulePath(params.schedule_path),
    scheduler(params.max_packets, params.schedule_path, params.dataCells),
    currentTimeSlotIndex(0),
    nextNetworkEvent([this]{ processNextNetworkEvent(); },
        name() + ".nextNetworkEvent"),
    stats(this)
{
    initializeNetworkLayers(params.layers);
    initializeDataCells(params.dataCells);
    scheduler.initialize();
    assignPacketsFromSchedule();
    computeNetworkParameters();
    calculatePowerAndArea();
    scheduleInitialEvent();
}

void
SuperNetwork::initializeNetworkLayers(const std::vector<Layer*>& layers)
{
    if (layers.empty()) {
        fatal("At least one Layer must be provided to SuperNetwork.\n");
    }

    Layer* currentLayer = layers[0];
    dynamicRange = currentLayer->getRangeSize();
    DPRINTF(SuperNetwork, "Dynamic range: %d\n", dynamicRange);
}

void
SuperNetwork::initializeDataCells(const std::vector<DataCell*>& cells)
{
    if (cells.empty()) {
        return;
    }

    assignRadix(cells.size() * 2); // I/O ports are double the # of DataCells

    for (uint64_t i = 0; i < cells.size(); i++) {
        DataCell* cell = cells[i];
        cell->setAddr(i);

        uint64_t randomData = random() % dynamicRange;
        cell->setData(randomData);

        DPRINTF(SuperNetwork, "DataCell %d: addr=%d, data=%d\n",
                i, cell->getAddr(), cell->getData());

        addDataCell(cell);
    }
}

void
SuperNetwork::assignPacketsFromSchedule()
{
    uint64_t packetCount = 0;

    while (scheduler.hasPackets()) {
        auto [srcAddr, destAddr] = scheduler.getNextPacket();

        DataCell* cell = getDataCell(srcAddr);
        if (cell != nullptr) {
            DPRINTF(SuperNetwork,
                "Assigning packet: src=%d, dest=%d\n",
                srcAddr, destAddr
            );
            cell->assignPacket(destAddr);
            packetCount++;
        }
    }

    DPRINTF(SuperNetwork, "Total packets assigned: %d\n", packetCount);
}

void
SuperNetwork::computeNetworkParameters()
{
    // Compute time slot and print
    assignTimeSlot();
    DPRINTF(SuperNetwork, "Time slot: %.0f ps\n", getTimeSlot());

    // Assign connection window
    assignConnectionWindow(dynamicRange * getTimeSlot());
    DPRINTF(SuperNetwork, "Connection window: %d\n", getConnectionWindow());
}

void
SuperNetwork::scheduleInitialEvent()
{
    bool hasPackets = false;
    for (DataCell* cell : dataCells) {
        if (cell->hasPackets()) {
            hasPackets = true;
            break;
        }
    }

    if (!dataCells.empty() && hasPackets) {
        DPRINTF(SuperNetwork, "Scheduling first network event\n");
        scheduleNextNetworkEvent(curTick() + 1); // Start at the next tick
    } else {
        DPRINTF(SuperNetwork, "No packets to process\n");
    }
}

void
SuperNetwork::calculatePowerAndArea()
{
    using namespace HardwareConstants;

    // Scale up 1x4 counting network power to match the radix
    double countingNetworkRatio = (((getRadix()/2) + 1) / 4.0);
    double countingNetworkActivePower =
        COUNTING_NETWORK_ACTIVE_POWER * countingNetworkRatio;
    double countingNetworkStaticPower =
        COUNTING_NETWORK_STATIC_POWER * countingNetworkRatio;
    int countingNetworkJJ = COUNTING_NETWORK_JJ * countingNetworkRatio;

    // Calculate number of components based on network radix
    int r = radix/2;
    int numCountingNetworks = r;
    int numCrosspoints = r * r;
    int numSplitters = r * (r * (r - 1) + 1);
    int numMergers = r * (r * (r - 1) + 1);

    // Calculate active power (nanowatts)
    double activePower =
        numCountingNetworks * countingNetworkActivePower +
        numCrosspoints * CROSSPOINT_ACTIVE_POWER +
        numSplitters * SPLITTER_ACTIVE_POWER +
        numMergers * MERGER_ACTIVE_POWER;

    // Calculate static power (microwatts)
    double staticPower =
        numCountingNetworks * countingNetworkStaticPower +
        numCrosspoints * CROSSPOINT_STATIC_POWER +
        numSplitters * SPLITTER_STATIC_POWER +
        numMergers * MERGER_STATIC_POWER;

    // Convert units to watts
    activePower *= 1e-9;  // nanowatts to watts
    staticPower *= 1e-6;  // microwatts to watts
    double totalPower = activePower + staticPower;

    // Calculate total Josephson Junctions
    int totalJJ =
        numCountingNetworks * countingNetworkJJ +
        numCrosspoints * CROSSPOINT_JJ +
        numSplitters * SPLITTER_JJ +
        numMergers * MERGER_JJ;

    // Log and store results
    DPRINTF(SuperNetwork, "Active power: %.6f W\n", activePower);
    DPRINTF(SuperNetwork, "Static power: %.6f W\n", staticPower);
    DPRINTF(SuperNetwork, "Total power: %.6f W\n", totalPower);
    DPRINTF(SuperNetwork, "Total JJ: %d\n", totalJJ);

    stats.activePower = activePower;
    stats.staticPower = staticPower;
    stats.totalPower = totalPower;
    stats.totalJJ = totalJJ;
}

void
SuperNetwork::assignTimeSlot()
{
    using namespace HardwareConstants;

    double SE_adjusted = std::max(0.0,
        CROSSPOINT_SETUP_TIME - CIRCUIT_VARIABILITY
    );
    double calculatedTimeSlot = CIRCUIT_VARIABILITY * (
        CROSSPOINT_DELAY + SE_adjusted +
        SPLITTER_DELAY * (radix - 1) +
        MERGER_DELAY * (radix - 1) +
        VARIABILITY_COUNTING_NETWORK);

    this->timeSlot = std::ceil(calculatedTimeSlot);
}

void
SuperNetwork::addDataCell(DataCell* dataCell)
{
    dataCells.push_back(dataCell);
    uint64_t addr = dataCell->getAddr();
    dataCellMap[addr] = dataCell;
}

DataCell*
SuperNetwork::getDataCell(uint64_t addr)
{
    auto it = dataCellMap.find(addr);
    return (it != dataCellMap.end()) ? it->second : nullptr;
}

void
SuperNetwork::processNextNetworkEvent()
{
    bool packetsRemaining = false;
    uint64_t packetsProcessedThisWindow = 0;

    // Build static schedule for the current time slot
    std::unordered_map<uint64_t, uint64_t> staticSchedule =
        buildStaticSchedule();

    // Process packets according to the static schedule
    packetsRemaining = processPackets(
        staticSchedule,
        packetsProcessedThisWindow
    );

    // Update statistics
    stats.totalWindowsUsed++;

    // Advance the time slot for the next event
    currentTimeSlotIndex++;

    // Schedule next network event if needed
    if (packetsRemaining) {
        scheduleNextNetworkEvent(curTick() + connectionWindow);
    } else {
        DPRINTF(SuperNetwork, "All packets processed\n");
        exitSimLoop("All packets processed");
    }
}

std::unordered_map<uint64_t, uint64_t>
SuperNetwork::buildStaticSchedule()
{
    std::unordered_map<uint64_t, uint64_t> staticSchedule;

    for (DataCell* cell : dataCells) {
        uint64_t srcAddr = cell->getAddr();
        uint64_t allowedDest =
            (srcAddr + currentTimeSlotIndex) % dataCells.size();
        staticSchedule[srcAddr] = allowedDest;

        DPRINTF(SuperNetwork,
            "Window %lu: allowed transmission from DataCell %lu to %lu\n",
            currentTimeSlotIndex, srcAddr, allowedDest
        );
    }

    return staticSchedule;
}

bool
SuperNetwork::processPackets(
    const std::unordered_map<uint64_t, uint64_t>& staticSchedule,
    uint64_t& packetsProcessedThisWindow)
{
    bool packetsRemaining = false;

    for (DataCell* cell : dataCells) {
        if (!cell->hasPackets()) {
            continue;
        }

        uint64_t srcAddr = cell->getAddr();
        uint64_t allowedDest = staticSchedule.at(srcAddr);
        uint64_t packetDest = cell->peekNextPacket();

        if (packetDest == allowedDest) {
            // Allowed: remove the packet and process it
            cell->getNextPacket();
            uint64_t payload = cell->getData();

            DPRINTF(SuperNetwork,
                "Processing packet: src=%lu, dest=%lu, data=%lu\n",
                srcAddr, allowedDest, payload
            );

            // Deliver packet to destination
            deliverPacket(srcAddr, allowedDest, payload);
            packetsProcessedThisWindow++;
        } else {
            // Not allowed to send this packet in the current time slot
            DPRINTF(SuperNetwork,
                "DataCell %lu: packet for %lu not scheduled (allowed: %lu)\n",
                srcAddr, packetDest, allowedDest
            );
        }

        // Check if cell still has packets
        if (cell->hasPackets()) {
            packetsRemaining = true;
        }
    }

    return packetsRemaining;
}

void
SuperNetwork::deliverPacket(uint64_t srcAddr,
    uint64_t destAddr, uint64_t payload)
{
    DataCell* destCell = getDataCell(destAddr);
    if (destCell != nullptr) {
        destCell->receiveData(payload, srcAddr);
        DPRINTF(SuperNetwork,
            "Packet delivered: src=%lu, dest=%lu, data=%lu\n",
            srcAddr, destAddr, payload
        );
        stats.totalPacketsProcessed++;
    } else {
        DPRINTF(SuperNetwork,
            "Error: Destination cell %lu not found\n",
            destAddr
        );
    }
}

void
SuperNetwork::scheduleNextNetworkEvent(Tick when)
{
    if (!nextNetworkEvent.scheduled()) {
        schedule(nextNetworkEvent, when);
    }
}

SuperNetwork::SuperNetworkStats::SuperNetworkStats(
    SuperNetwork* superNetwork
    ) : statistics::Group(superNetwork),
    ADD_STAT(activePower, statistics::units::Watt::get(), "Active power"),
    ADD_STAT(staticPower, statistics::units::Watt::get(), "Static power"),
    ADD_STAT(totalPower, statistics::units::Watt::get(), "Total power"),
    ADD_STAT(totalJJ, statistics::units::Count::get(), "Total JJ"),
    ADD_STAT(totalPacketsProcessed, statistics::units::Count::get(),
        "Total packets processed"),
    ADD_STAT(totalWindowsUsed, statistics::units::Count::get(),
        "Number of connection windows used"),
    ADD_STAT(pktsPerWindow, statistics::units::Count::get(),
        "Average packets per window")
{
}

void
SuperNetwork::SuperNetworkStats::regStats()
{
    using namespace statistics;

    activePower.name("activePower")
         .desc("Active power")
         .precision(6);

    staticPower.name("staticPower")
            .desc("Static power")
            .precision(6);

    totalPower.name("totalPower")
           .desc("Total power")
           .precision(6);

    totalJJ.name("totalJJ")
          .desc("Total number of Josephson Junctions");

    totalPacketsProcessed.name("totalPacketsProcessed")
                   .desc("Total number of packets processed");

    totalWindowsUsed.name("totalWindowsUsed")
              .desc("Number of connection windows used");

    pktsPerWindow.name("pktsPerWindow")
                   .desc("Average packets processed per window")
                   .precision(2)
                   .flags(nozero)
                   = totalPacketsProcessed / totalWindowsUsed;
}

} // namespace gem5
