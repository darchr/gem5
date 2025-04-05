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

#include "network/data_cell.hh"

#include "debug/DataCell.hh"
#include "sim/sim_exit.hh"
#include "sim/stats.hh"
#include "sim/system.hh"

namespace gem5
{

// Constructor for DataCell class
// Initializes the cell with default values and links to the statistics group
DataCell::DataCell(const DataCellParams& params) :
    ClockedObject(params),
    data(0),                 // Initialize data to 0
    addr(0),                 // Initialize address to 0
    stats(this)              // Link the stats object to this cell
{
}

// Sets the data value in the cell
void
DataCell::setData(uint64_t data)
{
    this->data = data;
}

// Retrieves the current data value in the cell
uint64_t
DataCell::getData()
{
    return data;
}

// Sets the address of the cell
void
DataCell::setAddr(uint64_t addr)
{
    this->addr = addr;
}

// Retrieves the address of the cell
uint64_t
DataCell::getAddr()
{
    return addr;
}

// Assigns a packet to the cell by pushing the dest address into queue
void
DataCell::assignPacket(uint64_t destAddr)
{
    packetQueue.push(destAddr);  // Queue the dest
    DPRINTF(DataCell, "DataCell %d assigned packet to destination %d\n",
            addr, destAddr);
}

// Retrieves and removes the next packet from the queue
// Returns 0 if no packets are available
uint64_t
DataCell::getNextPacket()
{
    if (!packetQueue.empty()) {
        // Get the next packet and remove it from the queue
        uint64_t nextPacket = packetQueue.front();
        packetQueue.pop();
        stats.sentPackets++;
        return nextPacket;
    }
    return 0; // No packet available
}

// Checks if there are any packets in the queue
bool
DataCell::hasPackets() const
{
    return !packetQueue.empty();
}

// Handles receiving data by updating the last received values and stats
void
DataCell::receiveData(uint64_t receivedData, uint64_t srcAddr)
{
    DPRINTF(DataCell, "DataCell %d received data %d from source %d\n",
            addr, receivedData, srcAddr);

    lastReceivedData = receivedData;  // Store the last received data
    lastReceivedFrom = srcAddr;       // Store the source of the data

    receivedPackets++;                // Increment the local received counter
    stats.receivedPackets++;          // Increment the statistics counter
}

// Returns the next packet without removing it from the queue
// Returns -1 if no packets are available
uint64_t
DataCell::peekNextPacket() const
{
    if (!packetQueue.empty()) {
         return packetQueue.front();
    }
    return -1;  // No packet available
}

// Constructor for the statistics group associated with the DataCell
DataCell::DataCellStats::DataCellStats(DataCell* dataCell) :
    statistics::Group(dataCell),
    ADD_STAT(sentPackets, statistics::units::Count::get(),
        "Packets sent from this cell"),
    ADD_STAT(receivedPackets, statistics::units::Count::get(),
        "Packets received by this cell")
{
}

// Registers statistics for the DataCell
void
DataCell::DataCellStats::regStats()
{
    using namespace statistics;

    sentPackets.name("sentPackets")
              .desc("Number of packets sent from this cell");

    receivedPackets.name("receivedPackets")
                  .desc("Number of packets received by this cell");
}

}  // namespace gem5
