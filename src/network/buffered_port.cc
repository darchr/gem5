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

#include "network/buffered_port.hh"

#include "debug/BufferedPort.hh"
#include "sim/sim_exit.hh"
#include "sim/stats.hh"
#include "sim/system.hh"

namespace gem5
{

// Constructor for BufferedPort class
// Initializes the port with default values
BufferedPort::BufferedPort(const BufferedPortParams& params) :
    ClockedObject(params),
    addr(0)                 // Initialize address to 0
{
}

// Sets the address of the port
void
BufferedPort::setAddr(uint64_t addr)
{
    this->addr = addr;
}

// Retrieves the address of the port
uint64_t
BufferedPort::getAddr()
{
    return addr;
}

// Assigns a value to the port by pushing the dest address into queue
void
BufferedPort::assignValue(uint64_t dest_addr)
{
    valueQueue.emplace(ValueEntry{dest_addr, curTick()});
    DPRINTF(BufferedPort,
        "BufferedPort %d assigned value "
        "to destination %d\n",
        addr, dest_addr
    );
}

// Clear the queue of values
void
BufferedPort::clearQueue()
{
    while (!valueQueue.empty()) {
        valueQueue.pop();
    }
}

// Retrieves and removes the next value from the queue
// Returns 0 if no values are available
ValueEntry
BufferedPort::getNextValue()
{
    if (!valueQueue.empty()) {
        // Get the next value and remove it from the queue
        ValueEntry nextValue = valueQueue.front();
        valueQueue.pop();
        return nextValue;
    }
    return ValueEntry{0, 0}; // No value available
}

// Checks if there are any values in the queue
bool
BufferedPort::hasValues() const
{
    return !valueQueue.empty();
}

// Increments the missed value count
void
BufferedPort::incrementMissedValues()
{
    missedValues++;
    DPRINTF(BufferedPort, "BufferedPort %d missed values: %d\n",
        addr, missedValues
    );
}

// Handles receiving data by updating the last received values and stats
void
BufferedPort::receiveData(uint64_t received_data, uint64_t src_addr)
{
    DPRINTF(BufferedPort, "BufferedPort %d received data %d from source %d\n",
            addr, received_data, src_addr);

    lastReceivedData = received_data;  // Store the last received data
    lastReceivedFrom = src_addr;       // Store the source of the data
}

// Returns the next value without removing it from the queue
// Returns -1 if no values are available
uint64_t
BufferedPort::peekNextValue() const
{
    if (!valueQueue.empty()) {
         return valueQueue.front().dest;
    }
    return -1; // No value available
}

}  // namespace gem5
