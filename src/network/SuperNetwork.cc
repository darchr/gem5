/*
 * Copyright (c) 2025 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "network/SuperNetwork.hh"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>

#include "debug/SuperNetwork.hh"
#include "network/NetworkScheduler.hh"
#include "sim/eventq.hh"
#include "sim/sim_exit.hh"
#include "sim/stats.hh"
#include "sim/system.hh"

namespace gem5 {

// // Hardware Constants namespace containing predefined values for various
// // network components based on the SRNoC paper
// namespace HardwareConstants {
//     // Timing delays in picoseconds for different network components
//     constexpr double CROSSPOINT_DELAY = 4.1;
//     constexpr double MERGER_DELAY = 8.84;
//     constexpr double SPLITTER_DELAY = 2.06;
//     constexpr double CIRCUIT_VARIABILITY = 1.2;
//     constexpr double VARIABILITY_COUNTING_NETWORK = 4.38;
//     constexpr double CROSSPOINT_SETUP_TIME = 8;

//     // Static power consumption in microwatts for various components
//     constexpr double SPLITTER_STATIC_POWER = 5.98;
//     constexpr double MERGER_STATIC_POWER = 5;
//     constexpr double CROSSPOINT_STATIC_POWER = 7.9;
//     constexpr double COUNTING_NETWORK_STATIC_POWER = 66.82;
//     constexpr double TFF_STATIC_POWER = 10.8;

//     // Active power consumption in nanowatts for various components
//     constexpr double SPLITTER_ACTIVE_POWER = 83.2;
//     constexpr double MERGER_ACTIVE_POWER = 69.6;
//     constexpr double CROSSPOINT_ACTIVE_POWER = 60.7;
//     constexpr double COUNTING_NETWORK_ACTIVE_POWER = 163;
//     constexpr double TFF_ACTIVE_POWER = 105.6;

//     // Number of Josephson Junctions (JJs) for each component type
//     constexpr int SPLITTER_JJ = 3;
//     constexpr int MERGER_JJ = 5;
//     constexpr int CROSSPOINT_JJ = 13;
//     constexpr int COUNTING_NETWORK_JJ = 60;
//     constexpr int TFF_JJ = 10;
// }

// Constructor for the SuperNetwork class
// Initializes the network with given parameters, sets up data cells,
// and prepares for packet scheduling
SuperNetwork::SuperNetwork(const SuperNetworkParams& params) :
    ClockedObject(params),
    maxPackets(params.max_packets),
    schedulePath(params.schedule_path),
    scheduler(params.max_packets, params.schedule_path, params.dataCells),
    crosspointDelay(params.crosspoint_delay),
    mergerDelay(params.merger_delay),
    splitterDelay(params.splitter_delay),
    circuitVariability(params.circuit_variability),
    variabilityCountingNetwork(params.variability_counting_network),
    crosspointSetupTime(params.crosspoint_setup_time),
    currentTimeSlotIndex(0),
    // Event for processing the next network event
    nextNetworkEvent([this]{ processNextNetworkEvent(); },
        name() + ".nextNetworkEvent"),
    stats(this)
{
    assert(params.crosspoint_delay >= 0);
    assert(params.merger_delay >= 0);
    assert(params.splitter_delay >= 0);
    assert(params.circuit_variability >= 0);
    assert(params.variability_counting_network >= 0);
    assert(params.crosspoint_setup_time >= 0);

    // Initialize network layers
    initializeNetworkLayers(params.layers);

    // Initialize data cells with random data
    initializeDataCells(params.dataCells);

    // Initialize the network scheduler
    scheduler.initialize();

    // Assign packets from the predefined schedule
    assignPacketsFromSchedule();

    // Compute network parameters like time slot and connection window
    computeNetworkParameters();

    // Calculate power consumption and area requirements

    // Schedule the initial network event
    scheduleInitialEvent();
}

// Initialize network layers, setting the dynamic range
void
SuperNetwork::initializeNetworkLayers(const std::vector<Layer*>& layers)
{
    // Ensure at least one layer is provided
    if (layers.empty()) {
        fatal("At least one Layer must be provided to SuperNetwork.\n");
    }

    // Set dynamic range from the first layer
    Layer* currentLayer = layers[0];
    dynamicRange = currentLayer->getRangeSize();

    // we need an encoding for 0, so increase the dynamic range by 1
    dynamicRange++;

    DPRINTF(SuperNetwork, "Dynamic range: %d\n", dynamicRange);
}

// Initialize data cells with random data and unique addresses
void
SuperNetwork::initializeDataCells(const std::vector<DataCell*>& cells)
{
    // Skip if no data cells are provided
    if (cells.empty()) {
        return;
    }

    // Set network radix
    assignRadix(cells.size() * 2);

    // Populate data cells with addresses and random data
    for (uint64_t i = 0; i < cells.size(); i++) {
        DataCell* cell = cells[i];
        cell->setAddr(i);

        // Generate random data within the dynamic range
        uint64_t randomData = random() % dynamicRange;
        cell->setData(randomData);

        DPRINTF(SuperNetwork, "DataCell %d: addr=%d, data=%d\n",
                i, cell->getAddr(), cell->getData());

        // Add the cell to the network
        addDataCell(cell);
    }
}

// Assign packets to data cells based on the predefined schedule
void
SuperNetwork::assignPacketsFromSchedule()
{
    uint64_t packetCount = 0;

    // Process all packets in the scheduler
    while (scheduler.hasPackets()) {
        // Get the next packet's source and destination addresses
        auto [srcAddr, destAddr] = scheduler.getNextPacket();

        // Find the source data cell
        DataCell* cell = getDataCell(srcAddr);
        if (cell != nullptr) {
            DPRINTF(SuperNetwork,
                "Assigning packet: src=%d, dest=%d\n",
                srcAddr, destAddr
            );
            // Assign the packet to the source cell
            cell->assignPacket(destAddr);
            packetCount++;
        }
    }

    DPRINTF(SuperNetwork, "Total packets assigned: %d\n", packetCount);
}

// Compute key network parameters like time slot and connection window
void
SuperNetwork::computeNetworkParameters()
{
    // Calculate and assign the time slot
    assignTimeSlot();
    DPRINTF(SuperNetwork, "Time slot: %.0f ps\n", getTimeSlot());

    // Calculate and assign the connection window
    assignConnectionWindow(dynamicRange * getTimeSlot());
    DPRINTF(SuperNetwork, "Connection window: %d\n", getConnectionWindow());
}

// Schedule the initial network event if there are packets to process
void
SuperNetwork::scheduleInitialEvent()
{
    bool hasPackets = false;
    // Check if any data cell has packets
    for (DataCell* cell : dataCells) {
        if (cell->hasPackets()) {
            hasPackets = true;
            break;
        }
    }

    // Schedule the first network event if packets exist
    if (!dataCells.empty() && hasPackets) {
        DPRINTF(SuperNetwork, "Scheduling first network event\n");
        scheduleNextNetworkEvent(curTick()); // Start at this tick
    } else {
        DPRINTF(SuperNetwork, "No packets to process\n");
    }
}

// Calculate the time slot based on network component delays
void
SuperNetwork::assignTimeSlot()
{
    // Adjust setup time considering circuit variability
    double SE_adjusted = std::max(0.0,
        crosspointSetupTime - circuitVariability
    );

    // Calculate time slot considering delays of various network components
    double calculatedTimeSlot = circuitVariability * (
        crosspointDelay + SE_adjusted +
        splitterDelay * (radix - 1) +
        mergerDelay * (radix - 1) +
        variabilityCountingNetwork
    );

    // Round up the calculated time slot
    this->timeSlot = std::ceil(calculatedTimeSlot);
}

// Add a data cell to the network's data cell collection
void
SuperNetwork::addDataCell(DataCell* dataCell)
{
    dataCells.push_back(dataCell);
    uint64_t addr = dataCell->getAddr();
    dataCellMap[addr] = dataCell;
}

// Retrieve a data cell by its address
DataCell*
SuperNetwork::getDataCell(uint64_t addr)
{
    auto it = dataCellMap.find(addr);
    return (it != dataCellMap.end()) ? it->second : nullptr;
}

// Process the next network event in the simulation
void
SuperNetwork::processNextNetworkEvent()
{
    bool packetsRemaining = false;
    uint64_t packetsProcessedThisWindow = 0;

    // Build a static schedule for the current time slot
    std::unordered_map<uint64_t, uint64_t> staticSchedule =
        buildStaticSchedule();

    // Process packets according to the static schedule
    packetsRemaining = processPackets(
        staticSchedule,
        packetsProcessedThisWindow
    );

    // Update statistics
    stats.totalWindowsUsed++;

    // Update packets per window distribution
    stats.pktsPerWindow.sample(packetsProcessedThisWindow);

    // Advance the time slot for the next event
    currentTimeSlotIndex++;

    // Schedule next network event or exit simulation
    if (packetsRemaining) {
        scheduleNextNetworkEvent(curTick() + connectionWindow);
    }
}

// Build a static schedule for packet transmission in the current time slot
std::unordered_map<uint64_t, uint64_t>
SuperNetwork::buildStaticSchedule()
{
    std::unordered_map<uint64_t, uint64_t> staticSchedule;

    // Determine allowed destination for each data cell
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

// Process packets according to the static schedule
bool
SuperNetwork::processPackets(
    const std::unordered_map<uint64_t, uint64_t>& staticSchedule,
    uint64_t& packetsProcessedThisWindow)
{
    bool packetsRemaining = false;
    Tick payloadSpecificDelay;

    // Iterate through all data cells
    for (DataCell* cell : dataCells) {
        // Skip cells without packets
        if (!cell->hasPackets()) {
            continue;
        }

        uint64_t srcAddr = cell->getAddr();
        uint64_t allowedDest = staticSchedule.at(srcAddr);
        uint64_t packetDest = cell->peekNextPacket();

        // Check if packet can be sent in the current time slot
        if (packetDest == allowedDest) {
            // Remove and process the packet
            cell->getNextPacket();
            uint64_t payload = cell->getData();

            // Calculate precise delivery time
            // within the connection window
            // Use the payload value -> RACE LOGIC
            payloadSpecificDelay =
                ((payload + 1) % dynamicRange) * (getTimeSlot());


            DPRINTF(SuperNetwork,
                "Processing packet: src=%lu, dest=%lu, \
                data=%lu, specific delay=%lu ps\n",
                srcAddr, allowedDest, payload, payloadSpecificDelay
            );

            // Schedule packet delivery with payload-specific timing
            schedule(new EventFunctionWrapper([this,
                srcAddr, allowedDest, payload]() {
                deliverPacket(srcAddr, allowedDest, payload);
            }, "deliverPacketEvent"), curTick() + payloadSpecificDelay);

            // DPRINTF(SuperNetwork,
            //     "Processing packet: src=%lu, dest=%lu, data=%lu\n",
            //     srcAddr, allowedDest, payload
            // );

            // // Deliver packet to destination
            // deliverPacket(srcAddr, allowedDest, payload);
            packetsProcessedThisWindow++;
        } else {
            // Packet not allowed in the current time slot
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

    if (!packetsRemaining) {
        DPRINTF(SuperNetwork, "All packets processed in window %lu\n",
            currentTimeSlotIndex);
        DPRINTF(SuperNetwork, "Payload specific delay: %lu\n",
            payloadSpecificDelay
        );
        // schedule exit
        exitSimLoop("All packets processed", 0,
            curTick() + payloadSpecificDelay, 0,
            false
        );
    }

    return packetsRemaining;
}

// Deliver a packet to its destination data cell
void
SuperNetwork::deliverPacket(uint64_t srcAddr,
    uint64_t destAddr, uint64_t payload)
{
    // Find the destination data cell
    DataCell* destCell = getDataCell(destAddr);
    if (destCell != nullptr) {
        // Receive data at the destination cell
        destCell->receiveData(payload, srcAddr);
        DPRINTF(SuperNetwork,
            "Packet delivered: src=%lu, dest=%lu, data=%lu\n",
            srcAddr, destAddr, payload
        );
        // Update total packets processed
        stats.totalPacketsProcessed++;
    } else {
        DPRINTF(SuperNetwork,
            "Error: Destination cell %lu not found\n",
            destAddr
        );
    }
}

// Schedule the next network event
void
SuperNetwork::scheduleNextNetworkEvent(Tick when)
{
    // Schedule only if not already scheduled
    if (!nextNetworkEvent.scheduled()) {
        schedule(nextNetworkEvent, when);
    }
}

// Constructor for SuperNetwork statistics
SuperNetwork::SuperNetworkStats::SuperNetworkStats(
    SuperNetwork* superNetwork
    ) : statistics::Group(superNetwork),
    ADD_STAT(totalPacketsProcessed, statistics::units::Count::get(),
        "Total packets processed"),
    ADD_STAT(totalWindowsUsed, statistics::units::Count::get(),
        "Number of connection windows used"),
    ADD_STAT(pktsPerWindow, statistics::units::Count::get(),
        "Distribution of packets per window")
{
}

// Register statistics for detailed tracking and reporting
void
SuperNetwork::SuperNetworkStats::regStats()
{
    using namespace statistics;

    // Configure statistics with names, descriptions, and formatting

    totalPacketsProcessed.name("totalPacketsProcessed")
                   .desc("Total number of packets processed");

    totalWindowsUsed.name("totalWindowsUsed")
              .desc("Number of connection windows used");

    // Calculate average packets per window
    // pktsPerWindow.name("pktsPerWindow")
    //                .desc("Average packets processed per window")
    //                .precision(2)
    //                .flags(nozero)
    //                = totalPacketsProcessed / totalWindowsUsed;

    pktsPerWindow.init(64);
}

} // namespace gem5
