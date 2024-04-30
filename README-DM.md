# Composable Memory Simulation Platform

This documents how to use the composable memory simulation platform in a gem5,
SST and gem5 + SST setup.
The setup can be used in gem5 to fast-forward full-system simulation and then
used in SST to simulate a multi-node system.

The code is mainly confined in the `disaggregated_memory` directory.
The directory is divided into four subdirectories, similar to the structure of
the gem5's standard library:

- `boards`: The disaggregated memory boards are inherited from the stdlib's
  boards. Users can pass two memory ranges. The first one is to model the local
  memory and the second one is to model a remote memory. The remote memory may
  or may not be in gem5, as these boards can be used directly with SST. These
  ranges are exposed as NUMA and zNUMA nodes to the operating system.
  Currently the following boards are supported:
  - `ArmComposableMemoryBoard` implemented in `arm_main_board.py`
  - `RiscvComposableMemoryBoard` implemented in `riscv_main_board.py`
- `memories`: This directory contains `ExternalRemoteMemory` inherited from
  ExternalMemory. Users can use both gem5 and SST to model this remote memory.
- `cachehierarchies`: gem5's stdlib cachehierarchies were modified to handle
  more than one outgoing connection from the LLC. Currently the following
  cachehierarchies are supported:
  - `ClassicPrivateL1PrivateL2DMCache`: A 2-level private classic cache
    hierarchy
  - `ClassicPrivateL1PrivateL2SharedL3DMCache`: A 3-level classic cache
    hierarchy that has a shared LLC.
  - *Note* ruby caches only work with the RiscvComposableMemoryBoard.
- `configs`: Top-level gem5 scripts that can be used to take checkpoints or run
  SST simulations.

Instructions on how to use this platform can be found in the following
sections.

## Workflow

In short, we use this setup to fast-forward simulations using gem5 to reach the
ROI and take a checkpoint. We then end the simulation and start is again in SST
while loading the checkpoint.

SST does not allow untimed memory accesses at runtime as different gem5 nodes
might be reciding on different processes. Therefore, we split this simulation
into two phases. The following diagram shows the workflow of the platform.

```
G t0 : starting simulation in gem5 (atomic/kvm)
E |
M |     t1 : simulation reached the start of ROI
5 |_____|____________________________________________________________ time ->
         |                                                  |
S        t2 : we start the simulation in SST (timing)       |
S                                                           |
T                                       end of simulation : t3
```
The first phase is entirely in gem5. This is represented by time t0 and t1. The
objective here is to reach the ROI asap take a checkpoint.

The second phase starts by loading the checkpoint back into the system but
using an SST-side script. The system remains identical except for the External
Memory, which now sends requests and receives responses to and from SST's
memory.

This can be scaled into N differnt gem5 nodes. Checkpoints need to be taken for
each of these nodes in their respective first phases.

See the paper link here for a better visualization.

## Taking Checkpoints

The following is an example of the first phase. We start the simulation
entirely in gem5. Assume that this is our first gem5 system (instance-id is 0).
This system has 2 GiB of local memory. Another block of 32 GiB memory is mapped
to this system as remote memory.

```sh
build/ARM/gem5.opt disaggregated_memory/configs/arm-main.py \
    --cpu-clock-rate 3GHz \         # set the clock rate
    --instance 0 \                  # set the instance id. This is appended with ckpt-file.
    --cpu-type kvm \                # using a KVM CPU to skip OS boot. The host needs to support kvm
    --local-memory-size 2GiB \      # The local memory should be small to moderate                      
    --remote-memory-addr-range 38654705664,73014444032 \  # Range 38 GiB to 70 GiB is mapped to a shared memory pool 
    --remote-memory-latency 0 \     # Remote memory latency should be added on the SST-side script
    --take-ckpt=True \              # This instance should take a checkpoint 
    --ckpt-file="test-ckpt" \       # The checkpoint is saved in m5out/test-ckpt0 directory
    --is-composable=False           # We are using only gem5 to take the checkpoint
```

If we are modelling multiple systems, all sharing the same memory resource in
SST, we need to repeat this step for the next system. This can be done by:

```sh
build/ARM/gem5.opt disaggregated_memory/configs/arm-main.py \
    --cpu-clock-rate 3GHz \         # set the clock rate
    --instance 1 \                  # set the instance id. This is appended with ckpt-file.
    --cpu-type kvm \                # using a KVM CPU to skip OS boot. The host needs to support kvm
    --local-memory-size 2GiB \      # The local memory should be small to moderate                      
    --remote-memory-addr-range 38654705664,73014444032 \  # Range 38 GiB to 70 GiB is mapped to a shared memory pool 
    --remote-memory-latency 0 \     # Remote memory latency should be added on the SST-side script
    --take-ckpt=True \              # This instance should take a checkpoint 
    --ckpt-file="test-ckpt" \       # The checkpoint is saved in m5out/test-ckpt1 directory
    --is-composable=False           # We are using only gem5 to take the checkpoint
```

Note that the stats.txt will be reset in the m5out directory. However, we are
not concerned about stats at this point as we are not using a timing CPU and
also we haven't reached the ROI.

This marks the end of phase 1.

## Restoring Checkpoints

The restoring of checkpoints marks the beginning of phase 2. The simulation now
needs to be initiated in SST. The SST-side script can be found in
`ext/sst/sst/arm_composable_memory.py`. Most of the required parameters need to
be set in the script directly.

```python
...
# XXX marks parameters that needs/can be changed.
disaggregated_memory_latency = "xxns"       # add latency to memory requests going to SST.
...
stat_output_directory = "m5out_"            # set the main output directory
...
is_composable = True                        # since this is now being simulated in SST
...
cpu_type = ["o3", "timing"]                 # Only timing CPUs work in this case.
...
gem5_run_script = "../../disaggregated_memory/configs/arm-main.py"
...
system_nodes = N                            # Number is gem5 systems to simulate

# node_memory_slice and remote_memory_slice needs to be consistent with the
# numbers used in phase 1.
...
# make sure that the --ckpt-file is correctly set in the cmd list.
```

All the outputs will be stored in `m5out_0`, `m5out_1` .. up to N directories.
If you are simulating just one node, then you can start the simulation without
mpi. This can be done by:
```sh
bin/sst --add-lib-path=./ sst/arm_composable_memory.py
```
If there are more than one gem5 system to simulate, then use this command;
```sh
mpirun -np 3 -- bin/sst --add-lib-path=./ sst/arm_composable_memory.py
```
*Note* Make sure that the checkpoint paths are correctly set when restoring
multiple systems. The instance id is appended at the end of the --ckpt-file
name.

Also, for SST-side statistics, set the following path correctly;
```py
sst.setStatisticOutput("sst.statOutputTXT",
        {"filepath" : f"arm-main-board.txt"})
```

