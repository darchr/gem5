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

#include "network/NetworkScheduler.hh"

#include <random>  // For random number generation

#include "debug/NetworkScheduler.hh"

namespace gem5 {

// Constructor for NetworkScheduler
// Initializes the scheduler with maximum packets,
// schedule path, and list of DataCells
NetworkScheduler::NetworkScheduler(
    uint64_t maxPackets,
    const std::string& schedulePath,
    const std::vector<DataCell*>& cells
)
: maxPackets(maxPackets),
  schedulePath(schedulePath),
  dataCells(cells),
  infiniteMode(maxPackets == static_cast<uint64_t>(-1))
{}

// Initializes the scheduler by either generating or loading a schedule
void
NetworkScheduler::initialize()
{
    if (maxPackets == 0 && schedulePath.empty()) {
        // Error: No packets and no schedule path specified
        fatal("Either max_packets or schedule_path must be provided.\n");
    } else if (maxPackets != 0 && schedulePath.empty()) {
        // Generate a random schedule if no path is specified
        generateRandomSchedule();
    } else if (maxPackets != 0 && !schedulePath.empty()) {
        // If both max packets and schedule path are provided
        if (fileExists(schedulePath)) {
            // Warn if the schedule file already exists
            warn("schedule_path %s exists; it will be overwritten.\n",
                schedulePath);
        }
        generateRandomSchedule();
        saveSchedule();
    } else if (maxPackets == 0 && !schedulePath.empty()) {
        // Load an existing schedule from the specified path
        loadSchedule();
    }
}

void
NetworkScheduler::generateAllToAllSchedule()
{
    clear();
    if (dataCells.empty()) {
        warn("No DataCells available for scheduling.\n");
        return;
    }

    // For each cell as a source,
    // schedule packets to all other cells as destinations.
    for (size_t i = 0; i < dataCells.size(); i++) {
        for (size_t j = 0; j < dataCells.size(); j++) {
            if (i != j) {
                uint64_t srcAddr = dataCells[i]->getAddr();
                uint64_t destAddr = dataCells[j]->getAddr();
                scheduleQueue.push(std::make_pair(srcAddr, destAddr));
            }
        }
    }
    DPRINTF(NetworkScheduler,
        "All-to-All schedule generated with %d packets.\n",
        scheduleQueue.size()
    );
}

void
NetworkScheduler::generateHotspotSchedule(
    uint64_t hotspotAddr,
    double hotspotFraction
)
{
    clear();
    if (dataCells.empty()) {
        warn("No DataCells available for scheduling.\n");
        return;
    }

    // Calculate the number of hotspot packets based on hotspotFraction.
    uint64_t hotspotPackets = maxPackets * hotspotFraction;
    uint64_t otherPackets = maxPackets - hotspotPackets;
    std::random_device rd;
    std::mt19937 gen(rd());

    // Generate packets targeting the hotspot.
    for (uint64_t i = 0; i < hotspotPackets; i++) {
        int srcIndex = gen() % dataCells.size();
        uint64_t srcAddr = dataCells[srcIndex]->getAddr();
        scheduleQueue.push(std::make_pair(srcAddr, hotspotAddr));
    }

    // Generate remaining random packets.
    for (uint64_t i = 0; i < otherPackets; i++) {
        int srcIndex = gen() % dataCells.size();
        int destIndex = gen() % dataCells.size();
        uint64_t srcAddr = dataCells[srcIndex]->getAddr();
        uint64_t destAddr = dataCells[destIndex]->getAddr();
        scheduleQueue.push(std::make_pair(srcAddr, destAddr));
    }

    DPRINTF(NetworkScheduler,
        "Hotspot schedule generated: %d packets targeting hotspot %d.\n",
        hotspotPackets, hotspotAddr
    );
}


// Generates a random schedule of packets
void
NetworkScheduler::generateRandomSchedule()
{
    // Clear any existing schedule before generating a new one
    clear();

    if (dataCells.empty()) {
        // Warn if there are no DataCells available
        warn("No DataCells available for scheduling.\n");
        return;
    }

     // In infinite mode we do not pre-generate the schedule.
    if (infiniteMode) {
        DPRINTF(NetworkScheduler,
            "Infinite mode active: schedule will \
            be generated on demand.\n"
        );
        return;
    }

    // Random number generator setup
    std::random_device rd;              // Random seed
    std::mt19937 gen(rd());             // Mersenne Twister random generator

    // Generate random src-dest packet pairs
    for (uint64_t i = 0; i < maxPackets; i++) {
        int srcIndex = gen() % dataCells.size();
        int destIndex = gen() % dataCells.size();
        uint64_t srcAddr = dataCells[srcIndex]->getAddr();
        uint64_t destAddr = dataCells[destIndex]->getAddr();
        scheduleQueue.push(std::make_pair(srcAddr, destAddr));
    }

    DPRINTF(NetworkScheduler,
        "Random schedule generated with %d packets.\n",
        scheduleQueue.size());
}

// Saves the current schedule to a file
void
NetworkScheduler::saveSchedule()
{
    std::ofstream ofs(schedulePath);
    if (!ofs.is_open()) {
        // Fatal error if the file cannot be opened for writing
        fatal("Failed to open schedule file %s for writing.\n", schedulePath);
    }

    // Write each src-dest packet pair to the file
    std::queue<std::pair<uint64_t, uint64_t>> tempQueue = scheduleQueue;
    while (!tempQueue.empty()) {
        auto entry = tempQueue.front();
        ofs << entry.first << " " << entry.second << "\n";
        tempQueue.pop();
    }

    ofs.close();
    DPRINTF(NetworkScheduler, "Schedule saved to %s.\n", schedulePath);
}

// Generates a random packet using the given src.
// It picks a random destination from dataCells.
uint64_t
NetworkScheduler::generateRandomPacket(uint64_t src)
{
    if (dataCells.empty()) {
        warn("No DataCells available for scheduling.\n");
        return -1;
    }

    // Random number generator setup
    std::random_device rd;
    std::mt19937 gen(rd());

    uint64_t destAddr = src;
    if (dataCells.size() > 1) {
        int destIndex = gen() % dataCells.size();
        destAddr = dataCells[destIndex]->getAddr();
    }

    return destAddr;
}

// Generates a hotspot packet using the
// given src, hotspotAddr and hotspotFraction.
// It generates a packet targeting the hotspot with
// probability hotspotFraction.
uint64_t
NetworkScheduler::generateHotspotPacket(
    uint64_t src,
    uint64_t hotspotAddr,
    double hotspotFraction
)
{
    if (dataCells.empty()) {
        warn("No DataCells available for scheduling.\n");
        return -1;
    }

    // Check if the hotspot address is valid
    if (hotspotAddr >= dataCells.size()) {
        fatal("Invalid hotspot address %lu.\n", hotspotAddr);
        return -1;
    }

    // Random number generator setup
    std::random_device rd;
    std::mt19937 gen(rd());

    // Generate a packet targeting the hotspot
    // with probability hotspotFraction.
    if (gen() % 100 < hotspotFraction * 100) {
        return hotspotAddr;
    }

    // Generate a random packet targeting a random destination.
    uint64_t destAddr = src;
    if (dataCells.size() > 1) {
        int destIndex = gen() % dataCells.size();
        destAddr = dataCells[destIndex]->getAddr();
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

    std::vector<std::pair<uint64_t, uint64_t>> fileEntries;
    uint64_t src, dest;

    // Read src-dest pairs from the file
    while (ifs >> src >> dest) {
        fileEntries.emplace_back(src, dest);
    }

    // Load the read entries into the schedule queue
    uint64_t packetCount = loadScheduleEntries(fileEntries);
    DPRINTF(NetworkScheduler,
        "Schedule loaded from %s with %d entries.\n",
        schedulePath, packetCount);
}

// Adds schedule entries from the file into the queue
uint64_t
NetworkScheduler::loadScheduleEntries(const std::vector<std::pair<uint64_t,
    uint64_t>>& fileEntries)
{
    uint64_t packetCount = 0;

    if (maxPackets > 0) {
        // If maxPackets is specified, only load up to that limit
        uint64_t remaining = maxPackets;

        // Add entries in rounds until the maxPackets limit is reached
        while (remaining > 0) {
            uint64_t entriesThisRound = std::min(remaining,
                (uint64_t)fileEntries.size());

            for (uint64_t i = 0; i < entriesThisRound; i++) {
                scheduleQueue.push(fileEntries[i]);
                packetCount++;
            }

            remaining -= entriesThisRound;
        }
    } else {
        // If no maxPackets limit, load all entries
        for (const auto& entry : fileEntries) {
            scheduleQueue.push(entry);
            packetCount++;
        }
    }

    return packetCount;
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

// Retrieves the next packet in the schedule
std::pair<uint64_t, uint64_t>
NetworkScheduler::getNextPacket()
{
    if (scheduleQueue.empty()) {
        // Return a default packet with {-1, -1}
        // if the queue is empty
        return {-1, -1};
    }

    // Retrieve and remove the next packet from the queue
    auto packet = scheduleQueue.front();
    scheduleQueue.pop();
    return packet;
}

// Clears the current schedule by swapping with an empty queue
void
NetworkScheduler::clear()
{
    std::queue<std::pair<uint64_t, uint64_t>> empty;
    std::swap(scheduleQueue, empty);
}

} // namespace gem5
