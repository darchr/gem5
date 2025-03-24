#ifndef __NETWORK_DATACELL_HH__
#define __NETWORK_DATACELL_HH__

#include <queue>

#include "params/DataCell.hh"
#include "sim/clocked_object.hh"

namespace gem5
{

class DataCell : public ClockedObject
{
    private:
        uint64_t data;
        uint64_t addr;
        std::queue<uint64_t> packetQueue;

        // For tracking packets
        uint64_t sentPackets = 0;
        uint64_t receivedPackets = 0;

        // Store last received data for debugging/verification
        uint64_t lastReceivedData = 0;
        uint64_t lastReceivedFrom = 0;

        struct DataCellStats : public statistics::Group
        {
            statistics::Scalar sentPackets;
            statistics::Scalar receivedPackets;

            DataCellStats(DataCell* dataCell);
            void regStats() override;
        };

        DataCellStats stats;

    public:
        DataCell(const DataCellParams& params);
        void setData(uint64_t data);
        uint64_t getData();
        void setAddr(uint64_t addr);
        uint64_t getAddr();
        void assignPacket(uint64_t destAddr);
        void receiveData(uint64_t receivedData, uint64_t srcAddr);
        uint64_t getNextPacket();
        bool hasPackets() const;
        uint64_t getSentPackets() const { return sentPackets; }
        uint64_t getReceivedPackets() const { return receivedPackets; }
        uint64_t getLastReceivedData() const { return lastReceivedData; }
        uint64_t getLastReceivedFrom() const { return lastReceivedFrom; }
        uint64_t peekNextPacket() const;
};
}

#endif
