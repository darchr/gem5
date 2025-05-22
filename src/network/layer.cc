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
#include <random>

#include "debug/Layer.hh"
#include "network/network_scheduler.hh"
#include "network/super_network.hh"
#include "sim/eventq.hh"
#include "sim/sim_exit.hh"
#include "sim/stats.hh"
#include "sim/system.hh"

namespace gem5 {

// Constructor for the Layer class
// Initializes the network with given parameters, sets up ports,
// and prepares for value scheduling
Layer::Layer(const LayerParams& params) :
    ClockedObject(params),
    maxValues(params.max_values),
    schedulePath(params.schedule_path),
    scheduler(params.max_values,
        params.schedule_path,
        params.buffered_ports
    ),
    crosspointDelay(params.crosspoint_delay),
    mergerDelay(params.merger_delay),
    splitterDelay(params.splitter_delay),
    circuitVariability(params.circuit_variability),
    variabilityCountingNetwork(params.variability_counting_network),
    crosspointSetupTime(params.crosspoint_setup_time),
    holdTime(params.hold_time),
    valuesDelivered(0),
    currentTimeSlotIndex(0),
    valuesPerPortPerWindow(params.values_per_port_per_window),
    isFinished(false),
    fileMode(false),
    shuffleEnabled(false),
    bufferDepth(params.buffer_depth),
    size(params.buffered_ports.size()),
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

    // Initialize ports
    initializeBufferedPorts(params.buffered_ports);

    // Initialize the network scheduler
    std::queue<std::pair<uint64_t, uint64_t>>
        schedule_queue = scheduler.initialize();

    if (!schedule_queue.empty()) {
        fileMode = true;
        maxValues = schedule_queue.size();
        while (!schedule_queue.empty()) {
            const auto& entry = schedule_queue.front();

            uint64_t src_addr = entry.first;
            uint64_t dest_addr = entry.second;

            BufferedPort* port = getBufferedPort(src_addr);
            if (port != nullptr) {
                port->assignValue(dest_addr);
                DPRINTF(Layer,
                        "BufferedPort %d: addr=%d, value assigned to %d\n",
                        src_addr, port->getAddr(), dest_addr);
            }
            schedule_queue.pop();
        }
    }
}

// Initialize ports with unique addresses
void
Layer::initializeBufferedPorts(const std::vector<BufferedPort*>& ports)
{
    // Skip if no ports are provided
    if (ports.empty()) {
        return;
    }

    // Set network radix
    setRadix(ports.size() * 2);

    // Populate ports with addresses
    for (uint64_t i = 0; i < ports.size(); i++) {
        BufferedPort* port = ports[i];
        port->setAddr(i);

        DPRINTF(Layer, "BufferedPort %d: addr=%d\n",
                i, port->getAddr());

        // Add the port to the network
        addBufferedPort(port);
    }
}

// Compute key network parameters like time slot and connection window
void
Layer::computeTimingParameters()
{
    // Calculate and assign the time slot
    assignTimeSlot();
    DPRINTF(Layer, "Time slot: %d ps\n",
        getTimeSlot()
    );

    DPRINTF(Layer,
        "Clock period: %lu\n",
        clockPeriod()
    );

    if (getTimeSlot() <= 0) {
        fatal("Layer %s: Time slot (%lu) must be greater than 0!\n",
            name(), getTimeSlot());
    }
    if (getTimeSlot() > clockPeriod()) {
        fatal("Layer %s: Time slot (%lu) is greater than ",
            "clock period (%lu)!\n",
            name(), getTimeSlot(), clockPeriod());
    }

    // Calculate the number of time slots possible
    // given the clock period and time slot duration
    // Round up to the nearest whole number
    rlTimeSlots = std::ceil(
        static_cast<double>(clockPeriod()) / getTimeSlot()
    );

    // We can only use a fraction of the time slots
    maxValuesPerWindow = std::floor(rlTimeSlots -
        (rlTimeSlots / std::exp(1.0)));

    DPRINTF(Layer,
        "RL Time slots: %lu\n",
        rlTimeSlots
    );

    DPRINTF(Layer,
        "Max values per window: %lu\n",
        maxValuesPerWindow * size
    );

    // Calculate and assign the connection window
    setConnectionWindow(rlTimeSlots * getTimeSlot());
    DPRINTF(Layer, "Connection window: %d ps\n",
        getConnectionWindow()
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
    double calculated_time_slot = std::ceil(circuitVariability * (
        crosspointDelay + se_adjusted +
        splitterDelay * (radix - 1) +
        mergerDelay * (radix - 1) +
        variabilityCountingNetwork
    ) + crosspointSetupTime);

    // Round up the calculated time slot
    this->timeSlot = calculated_time_slot;
}

// Add a port to the network's port collection
void
Layer::addBufferedPort(BufferedPort* port)
{
    bufferedPorts.push_back(port);
    uint64_t addr = port->getAddr();
    bufferedPortsMap[addr] = port;
}

// Retrieve a port by its address
BufferedPort*
Layer::getBufferedPort(uint64_t addr)
{
    auto it = bufferedPortsMap.find(addr);
    return (it != bufferedPortsMap.end()) ? it->second : nullptr;
}

// Process the next network event in the simulation
void
Layer::processNextNetworkEvent()
{
    if (isFinished) {
        return;
    }
    uint64_t values_processed_this_window = 0;

    // Build a static schedule for the current time slot
    std::unordered_map<uint64_t, uint64_t> static_schedule =
        buildStaticSchedule();

    // Process values according to the static schedule
    // Add hold time before processing values
    schedule(new EventFunctionWrapper(
        [this, static_schedule, values_processed_this_window]() mutable {
            processValues(static_schedule, values_processed_this_window);
        },
        "processValuesEvent"),
        curTick() + holdTime);

    stats.totalWindowsUsed++;

    // Advance the time slot for the next event
    currentTimeSlotIndex++;

    // Schedule next network event.
    // In infinite mode (maxValues == -1) we always schedule the next event.
    if (maxValues == static_cast<uint64_t>(-1)
            || valuesDelivered < maxValues) {
        scheduleNextNetworkEvent(curTick() +
            ((connectionWindow)));
    }
}

uint64_t
Layer::fillQueue(BufferedPort* port, TrafficMode mode)
{
    int count = 0;
    while (isBuffered() ? port->queueSize() < bufferDepth : count < 1) {
        count++;
        uint64_t src  = port->getAddr();
        uint64_t dest = 0;

        switch (mode) {
        case TrafficMode::RANDOM:
            dest = scheduler.generateRandomValue(src);
            DPRINTF(Layer,
                "BufferedPort %lu: generated random value for %lu\n",
                src, dest
            );
            break;

        case TrafficMode::HOTSPOT:
            dest = scheduler.generateHotspotValue(src, hotspotAddr,
                                                  hotspotFraction);
            DPRINTF(Layer,
                "BufferedPort %lu: generated hotspot value for %lu\n",
                src, dest
            );
            break;

        case TrafficMode::BIT_COMPLEMENT:
            dest = scheduler.generateBitComplementValue(src);
            DPRINTF(Layer,
                "BufferedPort %lu: generated bit complement value \
                for %lu\n",
                src, dest
            );
            break;

        case TrafficMode::TORNADO:
            dest = scheduler.generateTornadoValue(src);
            DPRINTF(Layer,
                "BufferedPort %lu: generated tornado value for %lu\n",
                src, dest
            );
            break;

        case TrafficMode::NEAREST_NEIGHBOR: {
            /* pick left or right at random so we don’t exceed
               injectionsPerPortPerWindow == 1 unintentionally */
            uint64_t neighbor =
                (random() & 1) ? (src + 1) % size
                               : (src + size - 1) % size;
            dest = neighbor;
            DPRINTF(Layer,
                "BufferedPort %lu: generated nearest-neighbor value \
                for %lu\n",
                src, dest
            );
            break;
        }

        case TrafficMode::ALL_TO_ALL: {
            /* cycle through the permutation instead of enqueuing them all */
            uint64_t next =
                (port->allToAllCursor + 1) % size;      // store cursor in port
            port->allToAllCursor = next;
            dest = next;
            DPRINTF(Layer,
                "BufferedPort %lu: generated all-to-all value for %lu\n",
                src, dest
            );
            break;
        }

        default:
            fatal("unknown traffic mode");
        }
        port->assignValue(dest);
    }

    // if shuffleEnabled is true, shuffle the queue
    if (shuffleEnabled) {
        port->shuffleQueue();
        DPRINTF(Layer,
            "BufferedPort %lu: shuffled queue\n",
            port->getAddr()
        );
    }

    return port->peekNextValue();
}


// Build a static schedule for value transmission in the current time slot
std::unordered_map<uint64_t, uint64_t>
Layer::buildStaticSchedule()
{
    std::unordered_map<uint64_t, uint64_t> static_schedule;

    // Determine allowed destination for each port
    for (BufferedPort* port : bufferedPorts) {
        uint64_t src_addr = port->getAddr();
        uint64_t allowed_dest =
            (src_addr + currentTimeSlotIndex) % bufferedPorts.size();
        static_schedule[src_addr] = allowed_dest;

        DPRINTF(Layer,
            "Window %lu: allowed transmission from BufferedPort %lu to %lu\n",
            currentTimeSlotIndex, src_addr, allowed_dest
        );
    }

    return static_schedule;
}

// Process values according to the static schedule
bool
Layer::processValues(
    const std::unordered_map<uint64_t, uint64_t>& static_schedule,
    uint64_t& values_processed_this_window)
{
    Tick payload_specific_delay = 0;
    // Determine if we are in infinite mode
    bool infinite_mode = (maxValues == static_cast<uint64_t>(-1));

    // Iterate through all ports
    for (BufferedPort* port : bufferedPorts) {
        std::vector<uint64_t> used_payloads;
        if (values_processed_this_window >= (maxValuesPerWindow * size)) {
            DPRINTF(Layer,
                "Window %lu: reached max values (%lu), stopping.\n",
                currentTimeSlotIndex, values_processed_this_window
            );
            break;
        }
        // Check if we've already reached the maximum values
        if (valuesDelivered >= maxValues && !infinite_mode) {
            break;  // Exit the loop immediately if we've reached max values
        }

        uint64_t src_addr = port->getAddr();
        uint64_t allowed_dest = static_schedule.at(src_addr);

        uint64_t value_dest = -1;
        // Check if there's already a value in the buffer first
        if (port->hasValues() && isBuffered()) {
            // Peek the next value from the port's queue
            value_dest = port->peekNextValue();
        } else {
            if (!fileMode) {
                value_dest = fillQueue(port, trafficMode);
            }
            //     if (trafficMode == TrafficMode::RANDOM) {
            //         // Generate a random value destination
            //         value_dest = scheduler.generateRandomValue(src_addr);
            //     } else if (trafficMode == TrafficMode::HOTSPOT) {
            //         // Use the static schedule for the current time slot
            //         value_dest = scheduler.generateHotspotValue(
            //             src_addr, hotspotAddr, hotspotFraction
            //         );
            //     } else if (trafficMode == TrafficMode::BIT_COMPLEMENT) {
            //         // Generate a bit complement value
            //         value_dest = scheduler.generateBitComplementValue(
            //             src_addr
            //         );
            //     } else if (trafficMode == TrafficMode::NEAREST_NEIGHBOR) {
            //         // only generate once per port
            //         if (!port->hasValues()) {
            //             // compute wrap‑around neighbors
            //             uint64_t left  = (src_addr + size - 1) % size;
            //             uint64_t right = (src_addr + 1)        % size;

            //             // pack them into a small vector
            //             std::vector<uint64_t> neighbors = { left, right };

            //             // optionally randomize order
            //             if (shuffleEnabled) {
            //                 std::random_device rd;
            //                 std::mt19937       g(rd());
            //                 std::shuffle(neighbors.begin(),
            //                     neighbors.end(), g
            //                 );
            //             }

            //             // enqueue neighbor values
            //             for (auto dest : neighbors) {
            //                 port->assignValue(dest);
            //                 DPRINTF(Layer,
            //                 "BufferedPort %lu: enqueued nearest-neighbor "
            //                     "value for destination %lu\n",
            //                     src_addr, dest
            //                 );
            //             }
            //         }
            //         value_dest = port->peekNextValue();
            //         DPRINTF(Layer,
            //         "BufferedPort %lu: nearest-neighbor value for %lu\n",
            //         src_addr, value_dest
            //         );
            //     } else if (trafficMode == TrafficMode::ALL_TO_ALL) {
            //         // Check if the port already has queued destinations.
            //         if (!port->hasValues()) {
            //             // Create a vector to hold all destination indices.
            //             std::vector<uint64_t> destinations;
            //             destinations.reserve(size);
            //             for (uint64_t dest = 0; dest < size; dest++) {
            //                 destinations.push_back(dest);
            //             }

            //             // Conditionally shuffle the vector
            //             if (shuffleEnabled) {
            //                 // Create a random number generator.
            //                 std::random_device rd;
            //                 std::mt19937 g(rd());
            //                 // Shuffle the destinations.
            //                 std::shuffle(destinations.begin(),
            //                     destinations.end(), g
            //                 );
            //             }

            //             // Enqueue each destination from the vector.
            //             // vector can be shuffled or not
            //             for (auto dest : destinations) {
            //                 port->assignValue(dest);
            //                 DPRINTF(Layer,
            //                     "BufferedPort %lu: enqueued all-to-all "
            //                     "value for destination %lu\n",
            //                     src_addr, dest
            //                 );
            //             }
            //         }
            //         // Peek the next destination from the port's queue.
            //         value_dest = port->peekNextValue();
            //         DPRINTF(Layer,
            //             "BufferedPort %lu: all-to-all value for %lu\n",
            //             src_addr, value_dest
            //         );
            //     } else if (trafficMode == TrafficMode::TORNADO) {
            //         // Generate a tornado value
            //         value_dest = scheduler.generateTornadoValue(src_addr);
            //     } else {
            //         // Handle unknown traffic mode
            //         fatal("Unknown traffic mode: %d\n", trafficMode);
            //     }
            //     DPRINTF(Layer,
            //         "BufferedPort %lu: generated value for %lu\n",
            //         src_addr, value_dest
            //     );
            //
            //     assert(value_dest != -1);
            // } else {
            //     // In file mode, don't generate a new value destination.
            //     // Optionally, log that no new value was generated.
            //     DPRINTF(Layer,
            //         "BufferedPort %lu: file mode active,"
            //         "skipping value generation\n",
            //         src_addr
            //     );
            // }
        }
        stats.totalValuesAttempted++;
        // Check if value can be sent in the current time slot
        if (value_dest == allowed_dest) {
            // Remove the value if it was from the buffer
            Tick enqueue_tick = 0;
            if (port->hasValues()) {
                auto entry = port->getNextValue();
                value_dest   = entry.dest;
                enqueue_tick  = entry.enqueueTick;
            } else {
                enqueue_tick = curTick();
            }

            // We can clear out the buffer
            if (isBuffered()) {
                port->clearQueue();
            }

            if (valuesPerPortPerWindow > rlTimeSlots) {
                // If the number of values per port per window
                // is greater than the number of time slots,
                // we need to generate unique payloads
                warn("Layer %s: valuesPerPortPerWindow (%lu) is greater than "
                    "rlTimeSlots (%lu), using rlTimeSlots instead\n",
                    name(), valuesPerPortPerWindow, rlTimeSlots);
                valuesPerPortPerWindow = rlTimeSlots;
            }

            uint64_t payload;
            used_payloads.reserve(valuesPerPortPerWindow);
            while (used_payloads.size() < valuesPerPortPerWindow) {
                uint64_t p = scheduler.generateRandomPayload(rlTimeSlots);
                if (std::find(used_payloads.begin(),
                            used_payloads.end(),
                            p)
                    == used_payloads.end()) {
                    used_payloads.push_back(p);
                }
            }

            for (auto p : used_payloads) {
                // Calculate precise delivery time
                // within the connection window
                // Use the payload value -> RACE LOGIC
                // - Payload: (payload + 1) time slots
                // - Splitter delay:
                //     * 1 stage if dest == 0
                //     * (dest + 1) stages if dest < size - 1
                //     * dest stages if dest == size - 1
                // - Merger delay:
                //     * (size - 1) stages unless src == size - 1,
                //         in which case it's 1 stage
                // - Crosspoint delay is constant
                payload_specific_delay =
                    ((p + 1) * getTimeSlot()) +
                    splitterDelay * (
                        (value_dest == 0) ? 1 :
                        (value_dest < size - 1) ? (value_dest + 1) :
                        value_dest
                    ) +
                    mergerDelay * (
                        (src_addr == size - 1) ? 1 :
                        (size - 1)
                    ) +
                    crosspointDelay +
                    variabilityCountingNetwork;

                DPRINTF(Layer,
                    "Processing value: src=%lu, dest=%lu, \
                    data=%lu, specific delay=%lu ps\n",
                    src_addr, allowed_dest, p, payload_specific_delay
                );

                stats.totalValuesProcessed++;
                valuesDelivered++;
                values_processed_this_window++;


                // Schedule value delivery with payload-specific timing
                schedule(new EventFunctionWrapper([this,
                    src_addr, allowed_dest, p, enqueue_tick]() {
                    deliverValue(src_addr, allowed_dest, p, enqueue_tick);
                }, "deliverValueEvent"), curTick() + payload_specific_delay);

                // Check if we've reached max values after processing this one
                // If not in infinite mode, check for termination condition.
                if (!infinite_mode && valuesDelivered >= maxValues) {
                    break;
                }
            }
        } else if (!fileMode) {
            // Value not allowed in the current time slot
            if (isBuffered()) {
                // If in buffered mode
                // Assign the value to the buffer
                port->assignValue(value_dest);
            }
            // Increment missed values for the BufferedPort
            port->incrementMissedValues();
            stats.missedValuesPerBufferedPort.sample(
                port->getMissedValues()
            );
            DPRINTF(Layer,
                "BufferedPort %lu: value for %lu not "
                "scheduled (allowed: %lu)\n",
                src_addr, value_dest, allowed_dest
            );
            DPRINTF(Layer,
                "BufferedPort %lu: value for %lu assigned to buffer\n",
                src_addr, value_dest
            );
        }
    }

    stats.pktsPerWindow.sample(values_processed_this_window);

    // Only exit simulation if not in infinite mode
    if (!infinite_mode && valuesDelivered >= maxValues) {
        DPRINTF(Layer,
            "All values processed in window %lu\n",
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

    return infinite_mode ? true : (valuesDelivered < maxValues);
}

// Deliver a value to its destination port
void
Layer::deliverValue(uint64_t src_addr,
    uint64_t dest_addr, uint64_t payload,
    Tick enqueue_tick)
{
    Tick total_latency = curTick() - enqueue_tick;
    DPRINTF(Layer,
        "Value delivery: src=%lu, dest=%lu, data=%lu, latency=%lu\n",
        src_addr, dest_addr, payload, total_latency
    );
    // Find the destination port
    BufferedPort* dest_port = getBufferedPort(dest_addr);
    if (dest_port != nullptr) {
        // Receive data at the destination port
        dest_port->receiveData(payload, src_addr);
        stats.valueLatency.sample(total_latency);
        DPRINTF(Layer,
            "Value delivered: src=%lu, dest=%lu, data=%lu\n",
            src_addr, dest_addr, payload
        );
    } else {
        DPRINTF(Layer,
            "Error: Destination port %lu not found\n",
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
    ADD_STAT(totalValuesProcessed, statistics::units::Count::get(),
        "Total values processed"),
    ADD_STAT(totalWindowsUsed, statistics::units::Count::get(),
        "Number of connection windows used"),
    ADD_STAT(pktsPerWindow, statistics::units::Count::get(),
        "Distribution of values per window"),
    ADD_STAT(missedValuesPerBufferedPort, statistics::units::Count::get(),
        "Distribution of missed values per BufferedPort"),
    ADD_STAT(valueLatency, statistics::units::Count::get(),
        "Distribution of value latency (ps)"),
    ADD_STAT(totalValuesAttempted, statistics::units::Count::get(),
        "Total values attempted to be sent")
{
}

// Register statistics for detailed tracking and reporting
void
Layer::LayerStats::regStats()
{
    using namespace statistics;

    // Configure statistics with names, descriptions, and formatting

    totalValuesProcessed.name("totalValuesProcessed")
                   .desc("Total number of values processed");

    totalWindowsUsed.name("totalWindowsUsed")
              .desc("Number of connection windows used");

    totalValuesAttempted.name("totalValuesAttempted")
                   .desc("Total values attempted to be sent");

    pktsPerWindow.init(parentLayer->size + 1);

    missedValuesPerBufferedPort.init(64);

    valueLatency.init(64)
        .name("valueLatency")
        .desc("End-to-end latency (ps) = buffer delay + network");
}

} // namespace gem5
