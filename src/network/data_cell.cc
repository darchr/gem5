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
// Initializes the cell with default values
DataCell::DataCell(const DataCellParams& params) :
    ClockedObject(params),
    addr(0)                 // Initialize address to 0
{
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
DataCell::assignPacket(uint64_t dest_addr)
{
    packetQueue.push(dest_addr);  // Queue the dest
    DPRINTF(DataCell, "DataCell %d assigned packet to destination %d\n",
            addr, dest_addr);
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

// Increments the missed packet count
void
DataCell::incrementMissedPackets()
{
    missedPackets++;
    DPRINTF(DataCell, "DataCell %d missed packets: %d\n",
        addr, missedPackets
    );
}

// Handles receiving data by updating the last received values and stats
void
DataCell::receiveData(uint64_t received_data, uint64_t src_addr)
{
    DPRINTF(DataCell, "DataCell %d received data %d from source %d\n",
            addr, received_data, src_addr);

    lastReceivedData = received_data;  // Store the last received data
    lastReceivedFrom = src_addr;       // Store the source of the data
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

}  // namespace gem5
