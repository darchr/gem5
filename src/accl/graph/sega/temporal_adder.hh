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

#ifndef __ACCL_GRAPH_SEGA_TEMPORAL_ADDER_HH__
#define __ACCL_GRAPH_SEGA_TEMPORAL_ADDER_HH__

#include <vector>

#include "base/statistics.hh"
#include "base/types.hh"
#include "params/TemporalAdder.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"

namespace gem5
{

// Temporal Adder Unit - Performs modular addition with fixed latency
// This unit performs (A + B) mod N with configurable latency
// Each epoch is typically 1600 ps. The unit models a simple functional unit
// that can be integrated into a PE datapath
class TemporalAdder : public ClockedObject
{
  public:
    PARAMS(TemporalAdder);
    TemporalAdder(const Params& params);

    // Trigger an addition operation
    int addEnable(uint32_t A, uint32_t B);

    // Check if operation is complete
    bool isDone() const { return done; }

    // Check if unit is currently busy
    bool isBusy() const { return busy; }

    // Get the computed sum
    uint32_t getSum() const { return sum; }

    // Get the computed carry
    uint32_t getCarry() const { return carry; }

    // Get the computed sum from a specific unit
    uint32_t getSumFromUnit(unsigned unitId) const;

    // Get the computed carry from a specific unit
    uint32_t getCarryFromUnit(unsigned unitId) const;

    // Get the modulus value used by this adder
    uint32_t getModValue() const { return modValue; }

    // Reset the done flag (prepare for next operation)
    void reset();

    // Get total latency in ticks
    Tick getLatency() const;

    // Get number of available (free) parallel units
    unsigned getAvailableUnits() const;

  private:
    // Structure to track individual operations
    struct Operation
    {
        uint32_t operandA;
        uint32_t operandB;
        uint32_t sum;
        uint32_t carry;
        bool done;
        EventFunctionWrapper* completeEvent;

        Operation() : operandA(0), operandB(0), sum(0), carry(0),
                      done(false), completeEvent(nullptr) {}
    };

    // Parameters
    // Modulus for addition (default: 8)
    const unsigned modValue;
    // Number of clock cycles for latency (default: 3)
    const unsigned latencyCycles;
    // Number of parallel units (default: 1)
    const unsigned numParallelUnits;

    // Internal state - multiple parallel operations
    std::vector<Operation> operations; // One per parallel unit
    unsigned busyUnits; // Count of currently busy units

    uint32_t operandA; // Captured operand A (unit 0)
    uint32_t operandB; // Captured operand B (unit 0)
    uint32_t sum; // Result: (A + B) mod modValue (unit 0)
    uint32_t carry; // Result: (A + B) / modValue (unit 0)
    bool done; // Done signal (unit 0)
    bool busy; // Unit is processing (any unit busy)

    // Event for operation completion (legacy, for unit 0)
    EventFunctionWrapper completeEvent;

    // Find first available (non-busy) unit
    int findAvailableUnit() const;

    // Process the addition operation (called after latency)
    void completeOperation();

    // Complete a specific parallel unit's operation
    void completeOperation(unsigned unitId);

  protected:
    struct TemporalAdderStats : public statistics::Group
    {
        TemporalAdderStats(TemporalAdder *adder);

        // Statistics
        statistics::Scalar totalOperations;
        statistics::Scalar totalCycles;
        statistics::Formula avgCyclesPerOp;
        statistics::Histogram sumDistribution;
        statistics::Histogram carryDistribution;
        // Distribution of busy units over time
        statistics::Histogram parallelUtilization;
    } stats;
};

} // namespace gem5

#endif // __ACCL_GRAPH_SEGA_TEMPORAL_ADDER_HH__
