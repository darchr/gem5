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

#include "accl/graph/sega/temporal_adder.hh"

#include "base/trace.hh"
#include "debug/TemporalAdder.hh"

namespace gem5
{

TemporalAdder::TemporalAdder(const TemporalAdderParams& params)
    : ClockedObject(params),
      modValue(params.mod_value),
      latencyCycles(params.latency_cycles),
      numParallelUnits(params.num_parallel_units),
      operations(params.num_parallel_units),
      busyUnits(0),
      operandA(0),
      operandB(0),
      sum(0),
      carry(0),
      done(false),
      busy(false),
      completeEvent([this]{ completeOperation(); }, name() + ".completeEvent"),
      stats(this)
{
    fatal_if(modValue == 0,
             "%s: mod_value must be greater than 0 (got %u)",
             name(), modValue);
    fatal_if(latencyCycles == 0,
             "%s: latency_cycles must be greater than 0 (got %u)",
             name(), latencyCycles);
    fatal_if(numParallelUnits == 0,
             "%s: num_parallel_units must be greater than 0 (got %u)",
             name(), numParallelUnits);

    // Initialize completion events for each parallel unit
    for (unsigned i = 0; i < numParallelUnits; ++i) {
        operations[i].completeEvent = new EventFunctionWrapper(
            [this, i]{ completeOperation(i); },
            name() + csprintf(".completeEvent[%d]", i));
    }

    DPRINTF(TemporalAdder,
            "%s: Created with mod_value=%u, "
            " latency_cycles=%u, clock_period=%lu ticks, "
            "num_parallel_units=%u\n",
            name(), modValue, latencyCycles, clockPeriod(), numParallelUnits);
}

int
TemporalAdder::addEnable(uint32_t A, uint32_t B)
{
    // Find an available parallel unit
    int unitId = findAvailableUnit();
    if (unitId < 0) {
        DPRINTF(TemporalAdder,
                "%s: Operation rejected - all %u parallel units are busy\n",
                name(), numParallelUnits);
        return -1;  // Return -1 to indicate all units busy
    }

    Operation& op = operations[unitId];

    // Capture inputs
    op.operandA = A;
    op.operandB = B;
    op.done = false;
    busyUnits++;

    // Update global state for backward compatibility (use unit 0's state)
    if (unitId == 0) {
        operandA = A;
        operandB = B;
        done = false;
    }
    busy = (busyUnits > 0);

    DPRINTF(TemporalAdder,
            "%s: Add_en triggered on unit %d - "
            " A=%u, B=%u at tick %lu (busy=%u/%u)\n",
            name(), unitId, A, B, curTick(), busyUnits, numParallelUnits);

    // Compute result (will be available after latency)
    uint32_t temp = op.operandA + op.operandB;
    op.sum = temp % modValue;
    op.carry = temp / modValue;

    DPRINTF(TemporalAdder,
            "%s: Unit %d computed sum=%u, carry=%u (temp=%u)\n",
            name(), unitId, op.sum, op.carry, temp);

    // Schedule completion event after latency
    Tick latency = getLatency();
    schedule(*op.completeEvent, curTick() + latency);

    DPRINTF(TemporalAdder,
            "%s: Unit %d scheduled completion for tick %lu "
            "(latency=%lu ticks)\n",
            name(), unitId, curTick() + latency, latency);

    // Update statistics
    stats.totalOperations++;

    return unitId;  // Return the unit ID that accepted the operation
}

int
TemporalAdder::findAvailableUnit() const
{
    for (unsigned i = 0; i < numParallelUnits; ++i) {
        if (!operations[i].completeEvent->scheduled()) {
            return i;
        }
    }
    return -1;  // All units busy
}

unsigned
TemporalAdder::getAvailableUnits() const
{
    unsigned available = 0;
    for (unsigned i = 0; i < numParallelUnits; ++i) {
        if (!operations[i].completeEvent->scheduled()) {
            available++;
        }
    }
    return available;
}

uint32_t
TemporalAdder::getSumFromUnit(unsigned unitId) const
{
    assert(unitId < numParallelUnits);
    return operations[unitId].sum;
}

uint32_t
TemporalAdder::getCarryFromUnit(unsigned unitId) const
{
    assert(unitId < numParallelUnits);
    return operations[unitId].carry;
}

void
TemporalAdder::completeOperation(unsigned unitId)
{
    assert(unitId < numParallelUnits);
    Operation& op = operations[unitId];

    DPRINTF(TemporalAdder,
            "%s: Unit %d operation complete at tick %lu - sum=%u, carry=%u\n",
            name(), unitId, curTick(), op.sum, op.carry);

    // Set done flag
    op.done = true;
    busyUnits--;

    // Update global state for backward compatibility (mirror unit 0)
    if (unitId == 0) {
        sum = op.sum;
        carry = op.carry;
        done = true;
    }
    busy = (busyUnits > 0);

    // Update statistics
    stats.totalCycles += latencyCycles;
    stats.sumDistribution.sample(op.sum);
    stats.carryDistribution.sample(op.carry);
    stats.parallelUtilization.sample(busyUnits);
}

void
TemporalAdder::completeOperation()
{
    // Legacy function - calls completeOperation(0)
    completeOperation(0);
}

void
TemporalAdder::reset()
{
    DPRINTF(TemporalAdder,
    "%s: Reset called at tick %lu\n", name(),
    curTick()
    );

    done = false;
    // Note: We don't clear busy here - that's handled by completeOperation()
}

Tick
TemporalAdder::getLatency() const
{
    // Latency = latencyCycles * clockPeriod
    Tick latency = latencyCycles * clockPeriod();

    DPRINTF(TemporalAdder,
            "%s: getLatency() = %u cycles * %lu ticks/cycle = %lu ticks\n",
            name(), latencyCycles, clockPeriod(), latency);

    return latency;
}

// Statistics
TemporalAdder::TemporalAdderStats::TemporalAdderStats(TemporalAdder *adder)
    : statistics::Group(adder),
      ADD_STAT(totalOperations, statistics::units::Count::get(),
               "Total number of addition operations performed"),
      ADD_STAT(totalCycles, statistics::units::Cycle::get(),
               "Total cycles spent in operations"),
      ADD_STAT(avgCyclesPerOp, statistics::units::Rate<
                    statistics::units::Cycle, statistics::units::Count>::get(),
               "Average cycles per operation"),
      ADD_STAT(sumDistribution, statistics::units::Count::get(),
               "Distribution of sum values"),
      ADD_STAT(carryDistribution, statistics::units::Count::get(),
               "Distribution of carry values"),
      ADD_STAT(parallelUtilization, statistics::units::Count::get(),
               "Distribution of busy parallel units")
{
    // Formula for average cycles per operation
    avgCyclesPerOp = totalCycles / totalOperations;

    // Initialize histograms
    // Sum distribution: 0 to (mod_value - 1)
    sumDistribution.init(adder->modValue);

    // Carry distribution: reasonable range based on expected inputs
    // If we assume inputs are bounded, carry won't be huge
    // Let's use a reasonable upper bound
    carryDistribution.init(32);  // 0 to 31

    // Parallel utilization: 0 to numParallelUnits
    parallelUtilization.init(adder->numParallelUnits + 1);  // 0 to N units
}

} // namespace gem5
