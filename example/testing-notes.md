# Testing and Simulation Running notes for PB test board

## Th, 8-17-2023
### Seg faults
* Fetch1ToFetch2ForwardDelay causes a seg fault if not set to 1. Theoretically, it should be fine with anything >= 1. We were advised to try testing with microbenchmarks. However, testing with both riscv-ccm and riscv-dptd still resulted in seg faults. Will continue to test with more microbenchmarks. If seg faults continue, this parameter will be removed.

* Fetch1LineWidth causes a seg fault if it is not set to 0. We were advised by Kunal to just keep it at 0.

### Dependent Parameters
* executeMemoryIssueLimit is dependent on executeIssueLimit; has to be less than executeIssueLimit.

* fatal: board.processor.cores.core.execute: executeMemoryCommitLimit (3) must be <= executeCommitLimit (2)
