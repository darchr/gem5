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
    std::vector<DataCell*> dataCells;
    std::unordered_map<uint64_t, DataCell*> dataCellMap;
    uint64_t dynamicRange;
    uint64_t radix;
    float timeSlot;
    int connectionWindow;
    uint64_t currentTimeSlotIndex;
    uint64_t maxPackets; // 0 means not provided.
    std::string schedulePath; // empty means not provided.
    std::queue<std::pair<uint64_t, uint64_t>> scheduleQueue;
    NetworkScheduler scheduler;

    // Initialization methods
    void initializeNetworkLayers(const std::vector<Layer*>& layers);
    void initializeDataCells(const std::vector<DataCell*>& cells);
    void assignPacketsFromSchedule();
    void computeNetworkParameters();
    void scheduleInitialEvent();

    // Processing packets
    std::unordered_map<uint64_t, uint64_t> buildStaticSchedule();
    bool processPackets(
        const std::unordered_map<uint64_t, uint64_t>& staticSchedule,
        uint64_t& packetsProcessedThisWindow
    );
    void deliverPacket(uint64_t srcAddr, uint64_t destAddr, uint64_t payload);

    struct SuperNetworkStats: public statistics::Group
    {
        // SRNoC statistics
        statistics::Formula activePower;
        statistics::Formula staticPower;
        statistics::Formula totalPower;
        statistics::Formula totalJJ;
        // Round-robin statistics
        statistics::Scalar totalPacketsProcessed;
        statistics::Scalar totalWindowsUsed;
        statistics::Formula pktsPerWindow;

        SuperNetworkStats(SuperNetwork* superNetwork);
        void regStats() override;
    };

    SuperNetworkStats stats;
    EventFunctionWrapper nextNetworkEvent;
    void processNextNetworkEvent();
    void scheduleNextNetworkEvent(Tick when);

public:
    SuperNetwork(const SuperNetworkParams& params);

    void addDataCell(DataCell* dataCell);
    DataCell* getDataCell(uint64_t addr);

    void assignRadix(uint64_t radix) { this->radix = radix; }
    uint64_t getRadix() const { return radix; }

    void assignTimeSlot();
    float getTimeSlot() const { return timeSlot; }

    void assignConnectionWindow(int window) { connectionWindow = window; }
    int getConnectionWindow() const { return connectionWindow; }

    void calculatePowerAndArea();
};

} // namespace gem5

#endif // __NETWORK_SUPERNETWORK_HH__
