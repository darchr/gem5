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

#ifndef __NETWORK_Layer_HH__
#define __NETWORK_Layer_HH__

#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/statistics.hh"
#include "base/stats/group.hh"
#include "network/buffered_port.hh"
#include "network/enums.hh"
#include "network/layer.hh"
#include "network/network_scheduler.hh"
#include "params/Layer.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"

namespace gem5
{

// Forward declaration of the SuperNetwork class
class SuperNetwork;

class Layer : public ClockedObject
{
private:
    std::vector<BufferedPort*> bufferedPorts;  // all ports in the network
    std::unordered_map<uint64_t, BufferedPort*> bufferedPortsMap;

    // network delay parameters
    double crosspointDelay;
    double mergerDelay;
    double splitterDelay;
    double circuitVariability;
    double variabilityCountingNetwork;
    double crosspointSetupTime;
    double holdTime;

    // Network configuration parameters
    uint64_t maxValuesPerWindow;  // Maximum values per window
    uint64_t radix;  // Radix for the network, used in the topology
    uint64_t rlTimeSlots;  // Number of time slots per connection win.
    uint64_t timeSlot;  // Time slot for scheduling values
    uint64_t connectionWindow;  // Connection window
    uint64_t currentTimeSlotIndex;  // Current index for the time slot
    int maxValues;  // Maximum number of values, -1 means no limit
    int valuesDelivered; // Number of value deliveries
    uint32_t valuesPerPortPerWindow; // Values per port per window
    int size; // Size of the network
    bool isFinished;  // Flag to indicate if the layer has finished
    bool fileMode;  // Flag to indicate if a file is used for scheduling
    bool shuffleEnabled;  // Flag to indicate if shuffling is enabled
    bool noBufferMode;  // Flag to indicate if no buffering is used
    std::string schedulePath;  // Path to the schedule file
    NetworkScheduler scheduler;  // Scheduler for the network
    SuperNetwork* superNetwork;  // Pointer to the super network
    TrafficMode trafficMode;  // Traffic mode for the network

    // Hotspot parameters
    uint64_t hotspotAddr; // Address of the hotspot
    double hotspotFraction; // Fraction of values targeting the hotspot

    // Initialization methods to set up buffered ports
    void initializeBufferedPorts(
        const std::vector<BufferedPort*>& buffered_ports
    );

    // Method to calculate the time slot based on network parameters
    void assignTimeSlot();

    // Methods for processing values
    // Builds a static schedule for the current time slot
    std::unordered_map<uint64_t, uint64_t> buildStaticSchedule();
    bool processValues(
        const std::unordered_map<uint64_t, uint64_t>& static_schedule,
        uint64_t& values_processed_this_window
    );
    // Method for delivering a value to its destination
    void deliverValue(uint64_t src_addr,
        uint64_t dest_addr, uint64_t payload,
        Tick enqueue_tick
    );

    // Struct to hold statistics related to the Layer
    struct LayerStats: public statistics::Group
    {
        // pointer to the Layer instance
        Layer *parentLayer;

        // Statistics for round-robin scheduling

        // Total number of values processed
        statistics::Scalar totalValuesProcessed;
        // Number of scheduling windows used
        statistics::Scalar totalWindowsUsed;
        // Number of values attempted to be sent
        statistics::Scalar totalValuesAttempted;
        // Distribution of values processed per time window
        statistics::Histogram pktsPerWindow;
        // Distribution of missed values per BufferedPort
        statistics::Histogram missedValuesPerBufferedPort;
        // Value latency distribution
        statistics::Histogram valueLatency;

        // Constructor that links stats to the Layer instance
        LayerStats(Layer* layer);

        // Registers the statistics with the simulator
        void regStats() override;
    };

    LayerStats stats;
    EventFunctionWrapper nextNetworkEvent;
    void processNextNetworkEvent();  // Processes the next network event

public:
    // Constructor for initializing a Layer with parameters
    Layer(const LayerParams& params);

    // Methods for computing layer timing parameters
    void computeTimingParameters();

    // Methods for adding and retrieving ports in the network
    void addBufferedPort(BufferedPort* buffered_port);
    BufferedPort* getBufferedPort(uint64_t addr);

    // Methods for getting certain network parameters
    uint64_t getMaximumValuesPerWindow() const
    {
        return maxValuesPerWindow;
    }

    uint64_t getRLTimeSlots() const
    {
        return rlTimeSlots;
    }

    // Methods for assigning and retrieving radix,
    // time slot, and connection window values
    void setRadix(uint64_t radix) { this->radix = radix; }
    uint64_t getRadix() const { return radix; }

    uint64_t getTimeSlot() const { return timeSlot; }
    void setTimeSlot(uint64_t timeSlot) { this->timeSlot = timeSlot; }

    void setConnectionWindow(uint64_t window) { connectionWindow = window; }
    int getConnectionWindow() const { return connectionWindow; }

    void scheduleNextNetworkEvent(Tick when);  // Schedules the next event

    void registerSuperNetwork(SuperNetwork* superNetwork)
    {
        this->superNetwork = superNetwork;
    }
    SuperNetwork* getSuperNetwork() const { return superNetwork; }

    bool hasFinished() const { return isFinished; }

    // setters for TrafficMode
    void setRandomTrafficMode()
    {
        trafficMode = TrafficMode::RANDOM;
    }

    void setAllToAllTrafficMode()
    {
        trafficMode = TrafficMode::ALL_TO_ALL;
    }

    void setTornadoTrafficMode()
    {
        trafficMode = TrafficMode::TORNADO;
    }

    void setBitComplementTrafficMode()
    {
        trafficMode = TrafficMode::BIT_COMPLEMENT;
    }

    void setNearestNeighborTrafficMode()
    {
        trafficMode = TrafficMode::NEAREST_NEIGHBOR;
    }

    void setHotspotTrafficMode(uint64_t hotspotAddr,
        double hotspotFraction)
    {
        this->hotspotAddr = hotspotAddr;
        this->hotspotFraction = hotspotFraction;
        trafficMode = TrafficMode::HOTSPOT;
    }

    void setShuffle() { shuffleEnabled = true; }

    void setNoBufferMode()
    {
        noBufferMode = true;
    }
};

} // namespace gem5

#endif // __NETWORK_Layer_HH__
