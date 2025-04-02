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

#include "network/Layer.hh"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>

#include "debug/Layer.hh"
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

// Constructor for the Layer class
// Initializes the network with given parameters, sets up data cells,
// and prepares for packet scheduling
Layer::Layer(const LayerParams& params) :
    ClockedObject(params),
    maxPackets(params.max_packets),
    schedulePath(params.schedule_path),
    scheduler(params.max_packets, params.schedule_path, params.data_cells),
    dynamicRange(params.dynamic_range),
    packetsDelivered(0),
    currentTimeSlotIndex(0),
    // Event for processing the next network event
    nextNetworkEvent([this]{ processNextNetworkEvent(); },
        name() + ".nextNetworkEvent"),
    stats(this)
{

    // Initialize data cells with random data
    initializeDataCells(params.data_cells);

    // Initialize the network scheduler
    scheduler.initialize();

    // Assign packets from the predefined schedule
    // assignPacketsFromSchedule();

    // Compute network parameters like time slot and connection window
    // computeNetworkParameters();

    // // Schedule the initial network event
    // scheduleNextNetworkEvent(curTick());
}

// Initialize data cells with random data and unique addresses
void
Layer::initializeDataCells(const std::vector<DataCell*>& cells)
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

        DPRINTF(Layer, "DataCell %d: addr=%d, data=%d\n",
                i, cell->getAddr(), cell->getData());

        // Add the cell to the network
        addDataCell(cell);
    }
}

// Assign packets to data cells based on the predefined schedule
void
Layer::assignPacketsFromSchedule()
{
    uint64_t packetCount = 0;

    // Process all packets in the scheduler
    while (scheduler.hasPackets()) {
        // Get the next packet's source and destination addresses
        auto [srcAddr, destAddr] = scheduler.getNextPacket();

        // Find the source data cell
        DataCell* cell = getDataCell(srcAddr);
        if (cell != nullptr) {
            DPRINTF(Layer,
                "Assigning packet: src=%d, dest=%d\n",
                srcAddr, destAddr
            );
            // Assign the packet to the source cell
            cell->assignPacket(destAddr);
            packetCount++;
        }
    }

    DPRINTF(Layer, "Total packets assigned: %d\n", packetCount);
}

// Compute key network parameters like time slot and connection window
// void
// Layer::computeNetworkParameters()
// {
//     // Calculate and assign the time slot
//     assignTimeSlot();
//     DPRINTF(Layer, "Time slot: %d Cycles\n", getTimeSlot());
//     DPRINTF(Layer, "Time slot: %d ps\n",
//         getTimeSlot() * clockPeriod()/714
//     );

//     // Calculate and assign the connection window
//     assignConnectionWindow(Cycles(dynamicRange * getTimeSlot()));
//     DPRINTF(Layer, "Connection window: %d Cycles\n",
//         getConnectionWindow()
//     );
//     DPRINTF(Layer, "Connection window: %d ps\n",
//         getConnectionWindow() * clockPeriod()/714
//     );
// }

// Schedule the initial network event if there are packets to process
// void
// Layer::scheduleInitialEvent()
// {
//     bool hasPackets = false;
//     // Check if any data cell has packets
//     for (DataCell* cell : dataCells) {
//         if (cell->hasPackets()) {
//             hasPackets = true;
//             break;
//         }
//     }

//     // Schedule the first network event if packets exist
//     if (!dataCells.empty() && hasPackets) {
//         DPRINTF(Layer, "Scheduling first network event\n");
//         scheduleNextNetworkEvent(curTick()); // Start at this tick
//     } else {
//         DPRINTF(Layer, "No packets to process\n");
//     }
// }

// Calculate the time slot based on network component delays
// void
// Layer::assignTimeSlot()
// {
//     // Adjust setup time considering circuit variability
//     double SE_adjusted = std::max(0.0,
//         crosspointSetupTime - circuitVariability
//     );

//     // Calculate time slot considering delays of various network components
//     double calculatedTimeSlot = circuitVariability * (
//         crosspointDelay + SE_adjusted +
//         splitterDelay * (radix - 1) +
//         mergerDelay * (radix - 1) +
//         variabilityCountingNetwork
//     );

//     // Round up the calculated time slot
//     this->timeSlot = Cycles(std::ceil(calculatedTimeSlot));
// }

// Add a data cell to the network's data cell collection
void
Layer::addDataCell(DataCell* dataCell)
{
    dataCells.push_back(dataCell);
    uint64_t addr = dataCell->getAddr();
    dataCellMap[addr] = dataCell;
}

// Retrieve a data cell by its address
DataCell*
Layer::getDataCell(uint64_t addr)
{
    auto it = dataCellMap.find(addr);
    return (it != dataCellMap.end()) ? it->second : nullptr;
}

// Process the next network event in the simulation
void
Layer::processNextNetworkEvent()
{
    uint64_t packetsProcessedThisWindow = 0;

    // Build a static schedule for the current time slot
    std::unordered_map<uint64_t, uint64_t> staticSchedule =
        buildStaticSchedule();

    // Process packets according to the static schedule
    bool packetsRemaining = processPackets(
        staticSchedule,
        packetsProcessedThisWindow
    );

    stats.totalWindowsUsed++;

    // Advance the time slot for the next event
    currentTimeSlotIndex++;

    // Schedule next network event
    if (packetsDelivered < maxPackets) {
        scheduleNextNetworkEvent(curTick() +
            connectionWindow * clockPeriod()/714);
    }
}

// Build a static schedule for packet transmission in the current time slot
std::unordered_map<uint64_t, uint64_t>
Layer::buildStaticSchedule()
{
    std::unordered_map<uint64_t, uint64_t> staticSchedule;

    // Determine allowed destination for each data cell
    for (DataCell* cell : dataCells) {
        uint64_t srcAddr = cell->getAddr();
        uint64_t allowedDest =
            (srcAddr + currentTimeSlotIndex) % dataCells.size();
        staticSchedule[srcAddr] = allowedDest;

        DPRINTF(Layer,
            "Window %lu: allowed transmission from DataCell %lu to %lu\n",
            currentTimeSlotIndex, srcAddr, allowedDest
        );
    }

    return staticSchedule;
}

// Process packets according to the static schedule
// Process packets according to the static schedule
bool
Layer::processPackets(
    const std::unordered_map<uint64_t, uint64_t>& staticSchedule,
    uint64_t& packetsProcessedThisWindow)
{
    Tick payloadSpecificDelay = 0;

    // Iterate through all data cells
    for (DataCell* cell : dataCells) {
        // Check if we've already reached the maximum packets
        if (packetsDelivered >= maxPackets) {
            break;  // Exit the loop immediately if we've reached max packets
        }

        uint64_t srcAddr = cell->getAddr();
        uint64_t allowedDest = staticSchedule.at(srcAddr);

        uint64_t packetDest;
        // Check if there's already a packet in the buffer first
        if (cell->hasPackets()) {
            packetDest = cell->peekNextPacket();
        } else {
            // Only generate a new packet if there's nothing in the buffer
            packetDest = scheduler.generateRandomPacket(srcAddr);
            assert(packetDest != -1);
        }

        // Check if packet can be sent in the current time slot
        if (packetDest == allowedDest) {
            // Remove the packet if it was from the buffer
            if (cell->hasPackets()) {
                cell->getNextPacket();
            }
            uint64_t payload = cell->getData();

            // Calculate precise delivery time
            // within the connection window
            // Use the payload value -> RACE LOGIC
            payloadSpecificDelay = ((payload + 1) % dynamicRange) *
                (getTimeSlot()) * clockPeriod()/714;

            DPRINTF(Layer,
                "Processing packet: src=%lu, dest=%lu, \
                data=%lu, specific delay=%lu ps\n",
                srcAddr, allowedDest, payload, payloadSpecificDelay
            );
            stats.totalPacketsProcessed++;
            packetsDelivered++;
            packetsProcessedThisWindow++;

            // Schedule packet delivery with payload-specific timing
            schedule(new EventFunctionWrapper([this,
                srcAddr, allowedDest, payload]() {
                deliverPacket(srcAddr, allowedDest, payload);
            }, "deliverPacketEvent"), curTick() + payloadSpecificDelay);

            // Check if we've reached max packets after processing this one
            if (packetsDelivered >= maxPackets) {
                break;
            }
        // } else if (cell->hasPackets() &&
        // cell->peekNextPacket() == allowedDest) {
        //     cell->getNextPacket();
        //     Above call should remove the packet from the queue
        //     uint64_t payload = cell->getData();

        //     // Calculate precise delivery time (as before)
        //     payloadSpecificDelay = ((payload + 1) % dynamicRange) *
        //         (getTimeSlot()) * clockPeriod() / 714;

        //     DPRINTF(Layer,
        //         "Processing packet: src=%lu, dest=%lu, data=%lu,
        //         specific delay=%lu ps\n",
        //         srcAddr, allowedDest, payload, payloadSpecificDelay
        //     );

        //     packetsDelivered++;
        //     packetsProcessedThisWindow++;

        //     // Schedule packet delivery with payload-specific timing
        //     schedule(new EventFunctionWrapper([this,
        //         srcAddr, allowedDest, payload]() {
        //         deliverPacket(srcAddr, allowedDest, payload);
        //     }, "deliverPacketEvent"), curTick() + payloadSpecificDelay);

        //     // Check if we've reached max packets after processing this one
        //     if (packetsDelivered >= maxPackets) {
        //         break;
        //     }
        } else {
            // Packet not allowed in the current time slot
            cell->assignPacket(packetDest);
            DPRINTF(Layer,
                "DataCell %lu: packet for %lu not scheduled (allowed: %lu)\n",
                srcAddr, packetDest, allowedDest
            );
            DPRINTF(Layer,
                "DataCell %lu: packet for %lu assigned to buffer\n",
                srcAddr, packetDest
            );
        }
    }

    stats.pktsPerWindow.sample(packetsProcessedThisWindow);

    if (packetsDelivered >= maxPackets) {
        DPRINTF(Layer, "All packets processed in window %lu\n",
            currentTimeSlotIndex);
        DPRINTF(Layer, "Payload specific delay: %lu\n",
            payloadSpecificDelay
        );
        // schedule exit
        exitSimLoop("All packets processed", 0,
            curTick() + payloadSpecificDelay, 0,
            false
        );
    }

    return (packetsDelivered < maxPackets);
}

// Deliver a packet to its destination data cell
void
Layer::deliverPacket(uint64_t srcAddr,
    uint64_t destAddr, uint64_t payload)
{
    // Find the destination data cell
    DataCell* destCell = getDataCell(destAddr);
    if (destCell != nullptr) {
        // Receive data at the destination cell
        destCell->receiveData(payload, srcAddr);
        DPRINTF(Layer,
            "Packet delivered: src=%lu, dest=%lu, data=%lu\n",
            srcAddr, destAddr, payload
        );
    } else {
        DPRINTF(Layer,
            "Error: Destination cell %lu not found\n",
            destAddr
        );
    }
}

// Schedule the next network event
void
Layer::scheduleNextNetworkEvent(Tick when)
{
    // Schedule only if not already scheduled
    if (!nextNetworkEvent.scheduled()) {
        schedule(nextNetworkEvent, when);
    }
}

// Constructor for Layer statistics
Layer::LayerStats::LayerStats(
    Layer* Layer
    ) : statistics::Group(Layer),
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
Layer::LayerStats::regStats()
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
