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

#include "network/layer.hh"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>

#include "debug/Layer.hh"
#include "network/network_scheduler.hh"
#include "network/super_network.hh"
#include "sim/eventq.hh"
#include "sim/sim_exit.hh"
#include "sim/stats.hh"
#include "sim/system.hh"

namespace gem5 {

// Constructor for the Layer class
// Initializes the network with given parameters, sets up data cells,
// and prepares for packet scheduling
Layer::Layer(const LayerParams& params) :
    ClockedObject(params),
    maxPackets(params.max_packets),
    schedulePath(params.schedule_path),
    scheduler(params.max_packets, params.schedule_path, params.data_cells),
    dynamicRange(params.dynamic_range),
    crosspointDelay(params.crosspoint_delay),
    mergerDelay(params.merger_delay),
    splitterDelay(params.splitter_delay),
    circuitVariability(params.circuit_variability),
    variabilityCountingNetwork(params.variability_counting_network),
    crosspointSetupTime(params.crosspoint_setup_time),
    holdTime(params.hold_time),
    packetsDelivered(0),
    currentTimeSlotIndex(0),
    isFinished(false),
    fileMode(false),
    size(params.data_cells.size()),
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

    // Initialize data cells with random data
    initializeDataCells(params.data_cells);

    // Initialize the network scheduler
    std::queue<std::pair<uint64_t, uint64_t>>
        schedule_queue = scheduler.initialize();

    if (!schedule_queue.empty()) {
        fileMode = true;
        maxPackets = schedule_queue.size();
        while (!schedule_queue.empty()) {
            const auto& entry = schedule_queue.front();

            uint64_t src_addr = entry.first;
            uint64_t dest_addr = entry.second;

            DataCell* cell = getDataCell(src_addr);
            if (cell != nullptr) {
                cell->assignPacket(dest_addr);
                DPRINTF(Layer,
                        "DataCell %d: addr=%d, packet assigned to %d\n",
                        src_addr, cell->getAddr(), dest_addr);
            }
            schedule_queue.pop();
        }
    }

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
    setRadix(cells.size() * 2);

    // Populate data cells with addresses and random data
    for (uint64_t i = 0; i < cells.size(); i++) {
        DataCell* cell = cells[i];
        cell->setAddr(i);

        // Generate random data within the dynamic range
        uint64_t random_data = random() % dynamicRange;
        cell->setData(random_data);

        DPRINTF(Layer, "DataCell %d: addr=%d, data=%d\n",
                i, cell->getAddr(), cell->getData());

        // Add the cell to the network
        addDataCell(cell);
    }
}

// Compute key network parameters like time slot and connection window
void
Layer::computeTimingParameters()
{
    // Calculate and assign the time slot
    assignTimeSlot();
    DPRINTF(Layer, "Time slot: %d Cycles\n", getTimeSlot());
    DPRINTF(Layer, "Time slot: %d ps\n",
        getTimeSlot() * clockPeriod()/714
    );

    // dynamic range increases due to circuit variability
    // and the number of data cells
    double expected_packets = std::round(
        (dynamicRange * std::exp(1.0)) / (std::exp(1.0) - 1.0)
    );

    // Calculate and assign the connection window
    setConnectionWindow(Cycles(expected_packets * getTimeSlot()));
    DPRINTF(Layer, "Connection window: %d Cycles\n",
        getConnectionWindow()
    );
    DPRINTF(Layer, "Connection window: %d ps\n",
        getConnectionWindow() * clockPeriod()/714
    );
}

// Calculate the time slot based on network component delays
void
Layer::assignTimeSlot()
{
    // Adjust setup time considering circuit variability
    double se_adjusted = std::max(0.0,
        crosspointSetupTime - circuitVariability
    );

    // Calculate time slot considering delays of various network components
    double calculated_time_slot = circuitVariability * (
        crosspointDelay + se_adjusted +
        splitterDelay * (radix - 1) +
        mergerDelay * (radix - 1) +
        variabilityCountingNetwork
    );

    // Round up the calculated time slot
    this->timeSlot = Cycles(std::ceil(calculated_time_slot));
}

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
    if (isFinished) {
        return;
    }
    uint64_t packets_processed_this_window = 0;

    // Build a static schedule for the current time slot
    std::unordered_map<uint64_t, uint64_t> static_schedule =
        buildStaticSchedule();

    // Process packets according to the static schedule
    // Add hold time before processing packets
    schedule(new EventFunctionWrapper(
        [this, static_schedule, packets_processed_this_window]() mutable {
            processPackets(static_schedule, packets_processed_this_window);
        },
        "processPacketsEvent"),
        curTick() + holdTime * clockPeriod() / 714);

    stats.totalWindowsUsed++;

    // Advance the time slot for the next event
    // Add scheduled setup time
    // currentTimeSlotIndex++;
    schedule(new EventFunctionWrapper(
        [this]() {
            currentTimeSlotIndex++;
        }, "advanceTimeSlotEvent"),
        curTick() + crosspointSetupTime * clockPeriod() / 714);

    // Schedule next network event.
    // In infinite mode (maxPackets == -1) we always schedule the next event.
    if (maxPackets == static_cast<uint64_t>(-1)
            || packetsDelivered < maxPackets) {
        scheduleNextNetworkEvent(curTick() +
            ((connectionWindow + crosspointSetupTime) * clockPeriod()/714));
    }
}

// Build a static schedule for packet transmission in the current time slot
std::unordered_map<uint64_t, uint64_t>
Layer::buildStaticSchedule()
{
    std::unordered_map<uint64_t, uint64_t> static_schedule;

    // Determine allowed destination for each data cell
    for (DataCell* cell : dataCells) {
        uint64_t src_addr = cell->getAddr();
        uint64_t allowed_dest =
            (src_addr + currentTimeSlotIndex) % dataCells.size();
        static_schedule[src_addr] = allowed_dest;

        DPRINTF(Layer,
            "Window %lu: allowed transmission from DataCell %lu to %lu\n",
            currentTimeSlotIndex, src_addr, allowed_dest
        );
    }

    return static_schedule;
}

// Process packets according to the static schedule
bool
Layer::processPackets(
    const std::unordered_map<uint64_t, uint64_t>& static_schedule,
    uint64_t& packets_processed_this_window)
{
    Tick payload_specific_delay = 0;
    // Determine if we are in infinite mode
    bool infinite_mode = (maxPackets == static_cast<uint64_t>(-1));

    // Iterate through all data cells
    for (DataCell* cell : dataCells) {
        // Check if we've already reached the maximum packets
        if (packetsDelivered >= maxPackets && !infinite_mode) {
            break;  // Exit the loop immediately if we've reached max packets
        }

        uint64_t src_addr = cell->getAddr();
        uint64_t allowed_dest = static_schedule.at(src_addr);

        uint64_t packet_dest = -1;
        // Check if there's already a packet in the buffer first
        if (cell->hasPackets()) {
            packet_dest = cell->peekNextPacket();
        } else {
            if (!fileMode) {
                if (trafficMode == TrafficMode::RANDOM) {
                    // Generate a random packet destination
                    packet_dest = scheduler.generateRandomPacket(src_addr);
                } else if (trafficMode == TrafficMode::HOTSPOT) {
                    // Use the static schedule for the current time slot
                    packet_dest = scheduler.generateHotspotPacket(
                        src_addr, hotspotAddr, hotspotFraction
                    );
                } else if (trafficMode == TrafficMode::ALL_TO_ALL) {
                    // Check if the cell already has queued destinations.
                    if (!cell->hasPackets()) {
                        // For an all-to-all mode, enqueue each destination.
                        for (uint64_t dest = 0; dest < size; dest++) {
                            cell->assignPacket(dest);
                            DPRINTF(Layer,
                                "DataCell %lu: enqueued "
                                "all-to-all packet for destination %lu\n",
                                src_addr, dest
                            );
                        }
                    }
                    // Peek the next destination from the cell’s queue.
                    packet_dest = cell->peekNextPacket();
                } else if (trafficMode == TrafficMode::TORNADO) {
                    // Generate a tornado packet
                    packet_dest = scheduler.generateTornadoPacket(src_addr);
                } else {
                    // Handle unknown traffic mode
                    fatal("Unknown traffic mode: %d\n", trafficMode);
                }
                DPRINTF(Layer,
                    "DataCell %lu: generated packet for %lu\n",
                    src_addr, packet_dest
                );
                // Only generate a new packet if there's nothing in the buffer
                assert(packet_dest != -1);
            } else {
                // In file mode, don't generate a new packet destination.
                // Optionally, log that no new packet was generated.
                DPRINTF(Layer,
                    "DataCell %lu: file mode active,"
                    "skipping packet generation\n",
                    src_addr
                );
            }
        }

        // Check if packet can be sent in the current time slot
        if (packet_dest == allowed_dest) {
            // Remove the packet if it was from the buffer
            if (cell->hasPackets()) {
                cell->getNextPacket();
            }
            uint64_t payload = cell->getData();

            // Calculate precise delivery time
            // within the connection window
            // Use the payload value -> RACE LOGIC
            payload_specific_delay =
                ((payload + 1) * getTimeSlot() * clockPeriod() / 714)
                + splitterDelay * (packet_dest + 1) * clockPeriod() / 714
                + mergerDelay * (size - src_addr - 1) * clockPeriod() / 714
                + crosspointDelay * clockPeriod() / 714;



            DPRINTF(Layer,
                "Processing packet: src=%lu, dest=%lu, \
                data=%lu, specific delay=%lu ps\n",
                src_addr, allowed_dest, payload, payload_specific_delay
            );
            stats.totalPacketsProcessed++;
            packetsDelivered++;
            packets_processed_this_window++;

            // Schedule packet delivery with payload-specific timing
            schedule(new EventFunctionWrapper([this,
                src_addr, allowed_dest, payload]() {
                deliverPacket(src_addr, allowed_dest, payload);
            }, "deliverPacketEvent"), curTick() + payload_specific_delay);

            // Check if we've reached max packets after processing this one
            // If not in infinite mode, check for termination condition.
            if (!infinite_mode && packetsDelivered >= maxPackets) {
                break;
            }
        } else if (!fileMode) {
            // Packet not allowed in the current time slot
            cell->assignPacket(packet_dest);
            DPRINTF(Layer,
                "DataCell %lu: packet for %lu not scheduled (allowed: %lu)\n",
                src_addr, packet_dest, allowed_dest
            );
            DPRINTF(Layer,
                "DataCell %lu: packet for %lu assigned to buffer\n",
                src_addr, packet_dest
            );
        }
    }

    stats.pktsPerWindow.sample(packets_processed_this_window);

    // Only exit simulation if not in infinite mode
    if (!infinite_mode && packetsDelivered >= maxPackets) {
        DPRINTF(Layer,
            "All packets processed in window %lu\n",
            currentTimeSlotIndex
        );
        DPRINTF(Layer,
            "Payload specific delay: %lu\n",
            payload_specific_delay
        );
        isFinished = true;
        if (superNetwork != nullptr) {
            // Schedule the notification after the delay
            schedule(new EventFunctionWrapper([this]() {
                superNetwork->notifyLayerFinished(this);
            }, "layerFinishedEvent"), curTick() + payload_specific_delay);
        }
    }

    return infinite_mode ? true : (packetsDelivered < maxPackets);
}

// Deliver a packet to its destination data cell
void
Layer::deliverPacket(uint64_t src_addr,
    uint64_t dest_addr, uint64_t payload)
{
    // Find the destination data cell
    DataCell* dest_cell = getDataCell(dest_addr);
    if (dest_cell != nullptr) {
        // Receive data at the destination cell
        dest_cell->receiveData(payload, src_addr);
        DPRINTF(Layer,
            "Packet delivered: src=%lu, dest=%lu, data=%lu\n",
            src_addr, dest_addr, payload
        );
    } else {
        DPRINTF(Layer,
            "Error: Destination cell %lu not found\n",
            dest_addr
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
    Layer* layer
    ) : statistics::Group(layer),
    parentLayer(layer),
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

    pktsPerWindow.init(parentLayer->size + 1);
}

} // namespace gem5
