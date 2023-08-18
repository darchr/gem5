# Testing and Simulation Running notes for PB test board

## Th, 8-17-2023
### Seg faults
* Fetch1ToFetch2ForwardDelay causes a seg fault if not set to 1. Theoretically, it should be fine with anything >= 1. We were advised to try testing with microbenchmarks. However, testing with both riscv-ccm and riscv-dptd still resulted in seg faults. Will continue to test with more microbenchmarks. If seg faults continue, this parameter will be removed.

* Fetch1LineWidth causes a seg fault if it is not set to 0. We were advised by Kunal to just keep it at 0.

### Dependent Parameters
* executeMemoryIssueLimit is dependent on executeIssueLimit; has to be less than executeIssueLimit.

* fatal: board.processor.cores.core.execute: executeMemoryCommitLimit (3) must be <= executeCommitLimit (2)


### Benchmark selection

* Choose more "balanced" benchmarks, instead of ones for testing.
* Potential options:
    - riscv-print-this - io operation
    - riscv-bubblesort - computation
    - riscv-floatmm - computation
    - riscv-hello  - io operation


### Runs
* For riscv-bubblesort, riscv-print-this, and riscv-floatmm 0_0_16, 5 runs were done for each combination of benchmark, foldover, and row. After that, only 3 runs were done for the remaining riscv-floatmm runs and riscv-hello. This was because I noticed that when comparing the five stats files for different runs in one row/foldover/benchmark, the seconds simulated stat + the ticks simulated stat were exactly the same. This could be because we are using SE mode, or because of how the MinorCPU (the underlying implementation of the RISCV Matched Board) works.
