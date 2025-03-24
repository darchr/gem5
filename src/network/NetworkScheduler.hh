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
#include "network/DataCell.hh"

namespace gem5 {

class NetworkScheduler
{
public:
    NetworkScheduler(uint64_t maxPackets,
        const std::string& schedulePath,
        const std::vector<DataCell*>& cells
    );

    void initialize();
    void generateRandomSchedule();
    void saveSchedule();
    void loadSchedule();
    uint64_t loadScheduleEntries(const std::vector<std::pair<uint64_t,
        uint64_t>>& fileEntries
    );

    bool hasPackets() const;
    std::pair<uint64_t, uint64_t> getNextPacket();
    void clear();

private:
    bool fileExists(const std::string& path) const;

    uint64_t maxPackets;
    std::string schedulePath;
    const std::vector<DataCell*>& dataCells;
    std::queue<std::pair<uint64_t, uint64_t>> scheduleQueue;
};

} // namespace gem5

#endif // __NETWORK_SCHEDULER_HH__
