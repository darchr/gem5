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

#ifndef __NETWORK_SUPERNETWORK_HH__
#define __NETWORK_SUPERNETWORK_HH__

#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/statistics.hh"
#include "base/stats/group.hh"
#include "network/DataCell.hh"
#include "network/Layer.hh"
#include "network/NetworkScheduler.hh"
#include "params/SuperNetwork.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"

namespace gem5
{

class SuperNetwork : public ClockedObject
{
private:
    std::vector<DataCell*> dataCells;  // Stores all data cells in the network
    std::unordered_map<uint64_t, DataCell*> dataCellMap;

    // Network configuration parameters
    uint64_t dynamicRange;  // The dynamic range of the network
    uint64_t radix;  // Radix for the network, used in the topology
    float timeSlot;  // Time slot for scheduling packets
    int connectionWindow;  // Time window for establishing network connections
    uint64_t currentTimeSlotIndex;  // Current index for the time slot
    uint64_t maxPackets;  // Maximum number of packets, 0 means no limit
    std::string schedulePath;  // Path to the schedule file
    std::queue<std::pair<uint64_t, uint64_t>> scheduleQueue;
    NetworkScheduler scheduler;  // Scheduler for the network

    // Initialization methods to set up network layers and data cells
    void initializeNetworkLayers(const std::vector<Layer*>& layers);
    void initializeDataCells(const std::vector<DataCell*>& cells);
    void assignPacketsFromSchedule();
    void computeNetworkParameters();
    void scheduleInitialEvent();

    // Methods for processing packets
    // Builds a static schedule for the current time slot
    std::unordered_map<uint64_t, uint64_t> buildStaticSchedule();
    bool processPackets(
        const std::unordered_map<uint64_t, uint64_t>& staticSchedule,
        uint64_t& packetsProcessedThisWindow
    );
    // Method for delivering a packet to its destination
    void deliverPacket(uint64_t srcAddr, uint64_t destAddr, uint64_t payload);

    // Struct to hold statistics related to the SuperNetwork
    struct SuperNetworkStats: public statistics::Group
    {
        // Statistics for SRNoC (Source-Routed NoC)

        // Active power consumption of the network
        statistics::Formula activePower;
        // Static power consumption of the network
        statistics::Formula staticPower;
        // Total power consumption of the network
        statistics::Formula totalPower;
        // Total Josephson Junctions used in the network
        statistics::Formula totalJJ;

        // Statistics for round-robin scheduling

        // Total number of packets processed
        statistics::Scalar totalPacketsProcessed;
        // Number of scheduling windows used
        statistics::Scalar totalWindowsUsed;
        // Number of packets processed per time window
        statistics::Formula pktsPerWindow;

        // Constructor that links stats to the SuperNetwork instance
        SuperNetworkStats(SuperNetwork* superNetwork);

        // Registers the statistics with the simulator
        void regStats() override;
    };

    SuperNetworkStats stats;
    EventFunctionWrapper nextNetworkEvent;
    void processNextNetworkEvent();  // Processes the next network event
    void scheduleNextNetworkEvent(Tick when);  // Schedules the next event

public:
    // Constructor for initializing a SuperNetwork with parameters
    SuperNetwork(const SuperNetworkParams& params);

    // Methods for adding and retrieving data cells in the network
    void addDataCell(DataCell* dataCell);
    DataCell* getDataCell(uint64_t addr);

    // Methods for assigning and retrieving radix,
    // time slot, and connection window values
    void assignRadix(uint64_t radix) { this->radix = radix; }
    uint64_t getRadix() const { return radix; }

    void assignTimeSlot();
    float getTimeSlot() const { return timeSlot; }

    void assignConnectionWindow(int window) { connectionWindow = window; }
    int getConnectionWindow() const { return connectionWindow; }

    // Method for calculating the power and area used by the network
    void calculatePowerAndArea();
};

} // namespace gem5

#endif // __NETWORK_SUPERNETWORK_HH__
