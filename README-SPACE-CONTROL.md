# Space-Control

This document lists down the steps to reproduce each Space-Control experiment
from the paper.

## Creating the Disk Image

## Generating Baseline
This simulates 8 systems running GAPBS and sharing a graph.
6 of these systems are running a GAPBS kernel (bfs, bc, cc, cc\_sv, pr, tc).
```sh
python3 disaggregated_memory/unified_run_space_control.py --count=8 --exp-name=no-permissions-8sys-1s --joblist=disaggregated_memory/joblist/space-control/base.json
```

Results of each of the system will be stored in `no-permissions-8sys-1s`
directory.
