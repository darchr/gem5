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

#ifndef __NETWORK_DATACELL_HH__
#define __NETWORK_DATACELL_HH__

#include <queue>

#include "params/DataCell.hh"
#include "sim/clocked_object.hh"

namespace gem5
{

// The DataCell class represents a single node in the network responsible for
// storing, sending, and receiving packets. It tracks its data, address,
// and maintains statistics for sent and received packets.
class DataCell : public ClockedObject
{
    private:
        // The data value stored in the cell
        uint64_t data;

        // The address identifier of the cell
        uint64_t addr;

        // Queue to hold packets (destination addresses) assigned to the cell
        std::queue<uint64_t> packetQueue;

        // Counters for the # of packets sent and received by the cell
        uint64_t sentPackets = 0;
        uint64_t receivedPackets = 0;

        // Variables to store the most recent received data and source addr
        uint64_t lastReceivedData = 0;
        uint64_t lastReceivedFrom = 0;

        // Statistics struct to track sent and received packets
        struct DataCellStats : public statistics::Group
        {
            statistics::Scalar sentPackets;      // # of packets sent
            statistics::Scalar receivedPackets;  // # of packets received

            // Constructor for the statistics group
            DataCellStats(DataCell* dataCell);

            // Registers the statistics with gem5's statistics system
            void regStats() override;
        };

        // Statistics instance associated with the current DataCell
        DataCellStats stats;

    public:
        // Constructor: Initializes the DataCell with parameters
        DataCell(const DataCellParams& params);

        // Sets the data value in the cell
        void setData(uint64_t data);

        // Retrieves the current data value
        uint64_t getData();

        // Sets the address of the cell
        void setAddr(uint64_t addr);

        // Retrieves the address of the cell
        uint64_t getAddr();

        // Assigns a packet to the queue with the given destination address
        void assignPacket(uint64_t destAddr);

        // Receives data from another cell
        // Stores the received value and source
        void receiveData(uint64_t receivedData, uint64_t srcAddr);

        // Retrieves and removes the next packet from the queue
        uint64_t getNextPacket();

        // Checks if the cell has any packets in its queue
        bool hasPackets() const;

        // Getters for tracking statistics
        uint64_t getSentPackets() const { return sentPackets; }
        uint64_t getReceivedPackets() const { return receivedPackets; }

        // Getters for the last received data and source information
        uint64_t getLastReceivedData() const { return lastReceivedData; }
        uint64_t getLastReceivedFrom() const { return lastReceivedFrom; }

        // Peeks at the next packet without removing it from the queue
        uint64_t peekNextPacket() const;
};

}  // namespace gem5

#endif  // __NETWORK_DATACELL_HH__
