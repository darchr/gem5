# Walkthrough - Uncacheable PMEM Range in Ruby Sequencer

This walkthrough details the changes made to implement an uncacheable PMEM address range in the Ruby Sequencer for gem5 v25. The goal is to route requests to a specific physical address range directly to the memory controller (via the IO/System bus), bypassing the Ruby cache coherence protocol.

## Changes Overview

### 1. Ruby Sequencer Modification
-   **Files**: `gem5/src/mem/ruby/system/Sequencer.{py,hh,cc}`
-   **Change**: Added a `pmem_address_range` parameter.
-   **Logic**:
    -   In `Sequencer::makeRequest`, checks if the request address falls within the configured PMEM range. If so, the request is scheduled directly on the `memRequestPort` (bypassing Ruby) and `RequestStatus_Issued` is returned.
    -   In `Sequencer::recvAtomic`, similar checks are performed for atomic requests to direct them to the memory port.
-   **Latency Tracking**: Implemented a histogram (`system.ruby.sequencer.pmem_latency`) to track reliability/performance key performance indicators (KPIs) of these bypassed requests.
-   **Debug Flag**: Added `RubyBypass` debug flag for tracing PMEM requests.

### 2. RubyPort Modifications
-   **Files**: `gem5/src/mem/ruby/system/RubyPort.{hh,cc}`
-   **Change**:
    -   Made `memRequestPort` accessible to `Sequencer`.
    -   Moved `recvAtomic` implementation to `RubyPort` to allow `Sequencer` to override it.
    -   Added `issueTime` to `SenderState` to track round-trip latency.

### 3. Cache Hierarchy Integration
-   **File**: `gem5/src/python/gem5/components/cachehierarchies/ruby/mesi_three_level_cache_hierarchy.py`
-   **Change**: Updated `__init__` to accept `pmem_address_range` and pass it to the `RubySequencer` constructor.

### 4. Simulation Script Update
-   **File**: `scripts/leo_script.py` / `remote_dir.py` / `host.py`
-   **Memory Split**:
    -   Reduced main memory (`DualChannelDDR4_2400`) from 8GiB to 7GiB.
    -   Created a `SimpleMemory` object for PMEM (1GiB) at the top of the physical address space (starting at 9GB, `0x240000000`).
    -   Attached PMEM directly to `board.iobus`.
-   **Configuration**:
    -   Enabled `MESIThreeLevelCacheHierarchy` (replacing Classic cache).
    -   Passed the PMEM range to the cache hierarchy.
    -   Updated Device Tree generation to correctly report the PMEM range.

## Verification

To verify the changes, you should compile gem5 and run the simulation script.

### Compilation
```bash
cd gem5
scons build/ARM/gem5.opt -j<NUM_CPUS>
```

### Running the Simulation
```bash
./gem5/build/ARM/gem5.opt --debug-flags=RubyBypass scripts/restore_ruby_bypass.py ...
```
(See `run_scripts/run_ruby_bypass_restore.sh` for full command)

### Expected Behavior
-   Accesses to the PMEM range (0x240000000 - 0x280000000) should bypass L1/L2/L3 caches and go purely through the `iobus` to the `SimpleMemory` device.
-   **Debug Output**: Enable `--debug-flags=RubyBypass` to see detailed logs:
    ```
    75000: system.ruby.sequencer: PMEM request ReadReq [340000:340040] bypassing Ruby
    ```
-   **Statistics**: Check `stats.txt` for `system.ruby.sequencer.pmem_latency` histogram.
