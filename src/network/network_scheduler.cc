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

#include "network/network_scheduler.hh"

#include <random>  // For random number generation

#include "debug/NetworkScheduler.hh"

namespace gem5 {

// Constructor for NetworkScheduler
// Initializes the scheduler with maximum packets,
// schedule path, and list of BufferedPorts
NetworkScheduler::NetworkScheduler(
    uint64_t max_packets,
    const std::string& schedule_path,
    const std::vector<BufferedPort*>& buffered_ports
)
: maxPackets(max_packets),
  schedulePath(schedule_path),
  bufferedPorts(buffered_ports),
  infiniteMode(max_packets == static_cast<uint64_t>(-1))
{}

// Initializes the scheduler by either generating or loading a schedule
std::queue<std::pair<uint64_t, uint64_t>>
NetworkScheduler::initialize()
{
    if (!maxPackets) {
        if (schedulePath.empty()) {
            fatal("Either max_packets or schedule_path must be provided.\n");
        } else {
            loadSchedule();
        }
    } else if (!schedulePath.empty()) {
        fatal("Both max_packets and schedule_path are specified.\n");
    }
    return scheduleQueue;
}

uint64_t
NetworkScheduler::generateRandomPayload(uint64_t dynamic_range) {
    // Create a random number generator.
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dist(0, dynamic_range - 1);
    return dist(gen);
}


// Generates a random packet using the given src.
// It picks a random destination from BufferedPorts.
uint64_t
NetworkScheduler::generateRandomPacket(uint64_t src)
{
    if (bufferedPorts.empty()) {
        warn("No BufferedPorts available for scheduling.\n");
        return -1;
    }

    // Random number generator setup
    std::random_device rd;
    std::mt19937 gen(rd());

    uint64_t dest_addr = src;
    if (bufferedPorts.size() > 1) {
        int dest_index = gen() % bufferedPorts.size();
        dest_addr = bufferedPorts[dest_index]->getAddr();
    }

    return dest_addr;
}

// Generates a tornado packet using the given src.
// It generates a packet targeting the port
// roughly halfway around the network.
// stresses bisection bandwidth.
uint64_t
NetworkScheduler::generateTornadoPacket(uint64_t src)
{
    if (bufferedPorts.empty()) {
        warn("No BufferedPorts available for scheduling.\n");
        return -1;
    }

    // Check if the source address is valid
    if (src >= bufferedPorts.size()) {
        fatal("Invalid source address %lu.\n", src);
        return -1;
    }

    // Generate a packet targeting the port
    // roughly halfway around the network.
    uint64_t dest_addr = (src + bufferedPorts.size() / 2)
        % bufferedPorts.size();

    return dest_addr;
}

// Generates a hotspot packet using the
// given src, hotspot_addr and hotspot_fraction.
// It generates a packet targeting the hotspot with
// probability hotspot_fraction.
uint64_t
NetworkScheduler::generateHotspotPacket(
    uint64_t src,
    uint64_t hotspot_addr,
    double hotspot_fraction
)
{
    if (bufferedPorts.empty()) {
        warn("No BufferedPorts available for scheduling.\n");
        return -1;
    }

    // Check if the hotspot address is valid
    if (hotspot_addr >= bufferedPorts.size()) {
        fatal("Invalid hotspot address %lu.\n", hotspot_addr);
        return -1;
    }

    // Random number generator setup
    std::random_device rd;
    std::mt19937 gen(rd());

    // Generate a packet targeting the hotspot
    // with probability hotspot_fraction.
    if (gen() % 100 < hotspot_fraction * 100) {
        return hotspot_addr;
    }

    // Generate a random packet targeting a random destination.
    uint64_t destAddr = src;
    if (bufferedPorts.size() > 1) {
        int destIndex = gen() % bufferedPorts.size();
        destAddr = bufferedPorts[destIndex]->getAddr();
    }

    return destAddr;
}


// Loads a schedule from a file
void
NetworkScheduler::loadSchedule()
{
    std::ifstream ifs(schedulePath);
    if (!ifs.is_open()) {
        // Fatal error if the file cannot be opened for reading
        fatal("Failed to open schedule file %s for reading.\n", schedulePath);
    }

    // Clear any existing schedule
    clear();

    std::vector<std::pair<uint64_t, uint64_t>> file_entries;
    uint64_t src, dest;

    // Read src-dest pairs from the file
    while (ifs >> src >> dest) {
        file_entries.emplace_back(src, dest);
    }

    // Load the read entries into the schedule queue
    uint64_t packet_count = loadScheduleEntries(file_entries);
    DPRINTF(NetworkScheduler,
        "Schedule loaded from %s with %d entries.\n",
        schedulePath, packet_count);
}

// Adds schedule entries from the file into the queue
uint64_t
NetworkScheduler::loadScheduleEntries(const std::vector<std::pair<uint64_t,
    uint64_t>>& file_entries)
{
    uint64_t packet_count = 0;

    if (maxPackets > 0) {
        // If maxPackets is specified, only load up to that limit
        uint64_t remaining = maxPackets;

        // Add entries in rounds until the maxPackets limit is reached
        while (remaining > 0) {
            uint64_t entriesThisRound = std::min(remaining,
                (uint64_t)file_entries.size());

            for (uint64_t i = 0; i < entriesThisRound; i++) {
                scheduleQueue.push(file_entries[i]);
                packet_count++;
            }

            remaining -= entriesThisRound;
        }
    } else {
        // If no maxPackets limit, load all entries
        for (const auto& entry : file_entries) {
            scheduleQueue.push(entry);
            packet_count++;
        }
    }

    return packet_count;
}

// Checks if the schedule file exists
bool
NetworkScheduler::fileExists(const std::string& path) const
{
    std::ifstream ifs(path);
    return ifs.good();
}

// Checks if there are packets remaining in the schedule
bool
NetworkScheduler::hasPackets() const
{
    return infiniteMode ? true : (!scheduleQueue.empty());
}

// Clears the current schedule by swapping with an empty queue
void
NetworkScheduler::clear()
{
    std::queue<std::pair<uint64_t, uint64_t>> empty;
    std::swap(scheduleQueue, empty);
}

} // namespace gem5
