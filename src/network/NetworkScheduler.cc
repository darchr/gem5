#include "network/NetworkScheduler.hh"

#include <random>

#include "debug/NetworkScheduler.hh"

namespace gem5 {

NetworkScheduler::NetworkScheduler(
uint64_t maxPackets,
const std::string& schedulePath,
const std::vector<DataCell*>& cells
)
: maxPackets(maxPackets),
  schedulePath(schedulePath),
  dataCells(cells)
{}

void
NetworkScheduler::initialize()
{
    if (maxPackets == 0 && schedulePath.empty()) {
        fatal("Either max_packets or schedule_path must be provided.\n");
    } else if (maxPackets != 0 && schedulePath.empty()) {
        generateRandomSchedule();
    } else if (maxPackets != 0 && !schedulePath.empty()) {
        if (fileExists(schedulePath)) {
            warn("schedule_path %s exists; it will be overwritten.\n",
                schedulePath);
        }
        generateRandomSchedule();
        saveSchedule();
    } else if (maxPackets == 0 && !schedulePath.empty()) {
        loadSchedule();
    }
}

void
NetworkScheduler::generateRandomSchedule()
{
    // Clear any existing schedule
    clear();

    if (dataCells.empty()) {
        warn("No DataCells available for scheduling.\n");
        return;
    }

    // Generate schedule with src and dest pairs
    std::random_device rd;
    std::mt19937 gen(rd());

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

void
NetworkScheduler::saveSchedule()
{
    std::ofstream ofs(schedulePath);
    if (!ofs.is_open()) {
        fatal("Failed to open schedule file %s for writing.\n", schedulePath);
    }

    std::queue<std::pair<uint64_t, uint64_t>> tempQueue = scheduleQueue;
    while (!tempQueue.empty()) {
        auto entry = tempQueue.front();
        ofs << entry.first << " " << entry.second << "\n";
        tempQueue.pop();
    }

    ofs.close();
    DPRINTF(NetworkScheduler, "Schedule saved to %s.\n", schedulePath);
}

void
NetworkScheduler::loadSchedule()
{
    std::ifstream ifs(schedulePath);
    if (!ifs.is_open()) {
        fatal("Failed to open schedule file %s for reading.\n", schedulePath);
    }

    clear();

    std::vector<std::pair<uint64_t, uint64_t>> fileEntries;
    uint64_t src, dest;
    while (ifs >> src >> dest) {
        fileEntries.emplace_back(src, dest);
    }

    uint64_t packetCount = loadScheduleEntries(fileEntries);
    DPRINTF(NetworkScheduler,
        "Schedule loaded from %s with %d entries.\n",
        schedulePath, packetCount);
}

uint64_t
NetworkScheduler::loadScheduleEntries(const std::vector<std::pair<uint64_t,
    uint64_t>>& fileEntries)
{
    uint64_t packetCount = 0;

    if (maxPackets > 0) {
        uint64_t remaining = maxPackets;
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
        for (const auto& entry : fileEntries) {
            scheduleQueue.push(entry);
            packetCount++;
        }
    }

    return packetCount;
}

bool
NetworkScheduler::fileExists(const std::string& path) const
{
    std::ifstream ifs(path);
    return ifs.good();
}

bool
NetworkScheduler::hasPackets() const
{
    return !scheduleQueue.empty();
}

std::pair<uint64_t, uint64_t>
NetworkScheduler::getNextPacket()
{
    if (scheduleQueue.empty()) {
        return {0, 0};
    }

    auto packet = scheduleQueue.front();
    scheduleQueue.pop();
    return packet;
}

void
NetworkScheduler::clear()
{
    std::queue<std::pair<uint64_t, uint64_t>> empty;
    std::swap(scheduleQueue, empty);
}

} // namespace gem5
