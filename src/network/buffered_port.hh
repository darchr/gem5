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

#ifndef __NETWORK_BUFFEREDPORT_HH__
#define __NETWORK_BUFFEREDPORT_HH__

#include <queue>
#include <random>

#include "params/BufferedPort.hh"
#include "sim/clocked_object.hh"

namespace gem5
{
struct ValueEntry
{
    uint64_t dest;
    Tick     enqueueTick;
};
// The BufferedPort class represents a single node in the network
// responsible for storing, sending, and receiving values.
// It tracks its data, address, and maintains statistics for
// sent and received values.
class BufferedPort : public ClockedObject
{
    private:
        // The address identifier of the port
        uint64_t addr;

        // Queue to hold values (destination addresses) assigned to the port
        std::queue<ValueEntry> valueQueue;

        // Variables to store the most recent received data and source addr
        uint64_t lastReceivedData = 0;
        uint64_t lastReceivedFrom = 0;

        uint64_t missedValues = 0; // Count of missed values

    public:
        // Constructor: Initializes the BufferedPort with parameters
        BufferedPort(const BufferedPortParams& params);

        // Sets the address of the port
        void setAddr(uint64_t addr);

        // Retrieves the address of the port
        uint64_t getAddr();

        // Assigns a value to the queue with the given destination address
        // The value is stored with the current tick as the enqueue time
        void assignValue(uint64_t dest);

        // Clears the queue of values
        void clearQueue();

        // Receives data from another port
        // Stores the received value and source
        void receiveData(uint64_t received_data, uint64_t src_addr);

        // Retrieves and removes the next value from the queue
        ValueEntry getNextValue();

        // Checks if the port has any values in its queue
        bool hasValues() const;

        // Getters for the last received data and source information
        uint64_t getLastReceivedData() const { return lastReceivedData; }
        uint64_t getLastReceivedFrom() const { return lastReceivedFrom; }

        // Getter for the size of the value queue
        size_t queueSize() const { return valueQueue.size(); }

        uint64_t allToAllCursor = 0; // Cursor for all-to-all traffic mode

        void shuffleQueue()
        {
            if (valueQueue.size() < 2) return;

            std::vector<ValueEntry> tmp;
            while (!valueQueue.empty()) {
                tmp.push_back(valueQueue.front());
                valueQueue.pop();
            }
            std::random_device rd;
            std::mt19937 gen(rd());
            std::shuffle(tmp.begin(), tmp.end(), gen);
            for (auto &e : tmp) valueQueue.push(e);
        }


        // Getter for missed values count
        uint64_t getMissedValues() const { return missedValues; }
        // Increments the missed values count
        void incrementMissedValues();

        // Peeks at the next value without removing it from the queue
        uint64_t peekNextValue() const;
};

}  // namespace gem5

#endif  // __NETWORK_BUFFEREDPORT_HH__
