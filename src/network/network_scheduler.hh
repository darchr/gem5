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

#ifndef __NETWORK_SCHEDULER_HH__
#define __NETWORK_SCHEDULER_HH__

#include <cstdint>
#include <fstream>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/logging.hh"
#include "network/buffered_port.hh"

namespace gem5 {

// NetworkScheduler class handles scheduling packets between BufferedPorts
class NetworkScheduler
{
public:
    // Constructor that initializes the scheduler
    NetworkScheduler(uint64_t max_packets,
        const std::string& schedule_path,
        const std::vector<BufferedPort*>& buffered_ports
    );

    // Initializes the scheduler
    std::queue<std::pair<uint64_t, uint64_t>> initialize();

    // Generates a random packet using the given src
    uint64_t generateRandomPacket(uint64_t src);

    // Generates a tornado packet
    // using the given src
    uint64_t generateTornadoPacket(uint64_t src);

    // Generates a hotspot packet
    // using the given src, hotspot_addr and hotspot_fraction
    uint64_t generateHotspotPacket(uint64_t src,
        uint64_t hotspot_addr,
        double hotspot_fraction
    );

    // Generates a random payload
    uint64_t generateRandomPayload(uint64_t dynamic_range);

    // Saves the current schedule to a specified file
    void saveSchedule();

    // Loads a schedule from a specified file
    void loadSchedule();

    // Loads schedule entries from a given list of source-destination pairs
    uint64_t loadScheduleEntries(const std::vector<std::pair<uint64_t,
        uint64_t>>& file_entries
    );

    // Checks if there are any packets left in the schedule
    bool hasPackets() const;

    // Clears the current schedule, effectively resetting the scheduler
    void clear();

private:
    // To check if we are in infinite mode
    bool infiniteMode;

    // Checks if a given file exists at the specified path
    bool fileExists(const std::string& path) const;

    // Maximum number of packets to be scheduled
    uint64_t maxPackets;
    // Path to the file where the schedule is saved/loaded
    std::string schedulePath;
    // List of BufferedPorts involved in the network
    const std::vector<BufferedPort*>& bufferedPorts;
    // Scheduled packets (source-destination pairs)
    std::queue<std::pair<uint64_t, uint64_t>> scheduleQueue;
    // Dynamic range of the layer
    uint64_t dynamicRange;
};

} // namespace gem5

#endif // __NETWORK_SCHEDULER_HH__
