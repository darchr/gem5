#include "network/DataCell.hh"

#include "debug/DataCell.hh"
#include "sim/sim_exit.hh"
#include "sim/stats.hh"
#include "sim/system.hh"

namespace gem5
{

DataCell::DataCell(const DataCellParams& params) :
    ClockedObject(params),
    data(0),
    addr(0),
    stats(this)
{
}

void
DataCell::setData(uint64_t data)
{
    this->data = data;
}

uint64_t
DataCell::getData()
{
    return data;
}

void
DataCell::setAddr(uint64_t addr)
{
    this->addr = addr;
}

uint64_t
DataCell::getAddr()
{
    return addr;
}

void
DataCell::assignPacket(uint64_t destAddr)
{
    packetQueue.push(destAddr);
    DPRINTF(DataCell, "DataCell %d assigned packet to destination %d\n",
            addr, destAddr);
}

uint64_t
DataCell::getNextPacket()
{
    if (!packetQueue.empty()) {
        uint64_t nextPacket = packetQueue.front();
        packetQueue.pop();
        stats.sentPackets++;
        return nextPacket;
    }
    return 0; // Indicate no packet available
}

bool
DataCell::hasPackets() const
{
    return !packetQueue.empty();
}

void
DataCell::receiveData(uint64_t receivedData, uint64_t srcAddr)
{
    DPRINTF(DataCell, "DataCell %d received data %d from source %d\n",
            addr, receivedData, srcAddr);


    lastReceivedData = receivedData;
    lastReceivedFrom = srcAddr;

    receivedPackets++;
    stats.receivedPackets++;
}

uint64_t
DataCell::peekNextPacket() const
{
    if (!packetQueue.empty()) {
         return packetQueue.front();
    }
    return -1;
}


DataCell::DataCellStats::DataCellStats(DataCell* dataCell) :
    statistics::Group(dataCell),
    ADD_STAT(sentPackets, statistics::units::Count::get(),
        "Packets sent from this cell"),
    ADD_STAT(receivedPackets, statistics::units::Count::get(),
        "Packets received by this cell")
{
}

void
DataCell::DataCellStats::regStats()
{
    using namespace statistics;

    sentPackets.name("sentPackets")
              .desc("Number of packets sent from this cell");

    receivedPackets.name("receivedPackets")
                  .desc("Number of packets received by this cell");
}

}
