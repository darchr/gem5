# Composable Memory Simulation Platform

This documents how to use the composable memory simulation platform in a gem5,
SST and gem5 + SST setup.
The setup can be used in gem5 to fast-forward the inti phase of running an application in full-system simulation using KVM in gem5 and then
be used in SST to simulate a multi-host system.

The code is mainly confined in the `disaggregated_memory` directory.
The directory is divided into four subdirectories, similar to the structure of
the gem5's standard library:

- `boards`: The disaggregated memory boards are inherited from the stdlib's
  boards. Users can pass two memory ranges. The first one is to model the local
  memory and the second one is to model a remote memory. The remote memory may
  or may not be in gem5, as these boards can be used directly with SST. These
  ranges are exposed as NUMA and zNUMA nodes to the operating system.
  Currently the following boards are supported:
  - `ArmComposableMemoryBoard` implemented in `arm_main_board.py`. This will create the main host structure: 8 cores, a cache hierarchy and the memory system. There are some parameters hard coded in here according to our discussions for the experiments we need. You can find them here {https://github.com/darchr/papers/blob/master/disaggregated_memory/outline.md}
- `memories`: This directory contains `ExternalRemoteMemory` inherited from
  ExternalMemory. Users can use both gem5 and SST to model this remote memory.
- `cachehierarchies`: gem5's stdlib cachehierarchies were modified to handle
  more than one outgoing connection from the LLC. Currently the following
  cachehierarchies are supported:
  - `ClassicPrivateL1PrivateL2DMCache`: A 2-level private classic cache
    hierarchy
  - `ClassicPrivateL1PrivateL2SharedL3DMCache`: A 3-level classic cache
    hierarchy that has a shared LLC.
- `configs`: Top-level gem5 scripts that can be used to take checkpoints or run
  SST simulations by restoring a checkpoint.


## Workflow
SST does not allow untimed memory accesses at runtime as different gem5 nodes
might be reciding on different processes. Therefore, we split this simulation
into two phases. 

The first phase is entirely in gem5. This is represented by time t0 and t1. The
objective here is to reach the ROI asap take a checkpoint. In short, at t0 we start a simulation in gem5-only using this setup to fast-forward simulations using KVM CPU to reach the ROI and take a checkpoint then end the simulation at t1.

The second phase starts by loading the checkpoint back into the system but
using an SST-side script in which will call gem5 script inetrnally. The system remains identical except for the External Memory, which now sends requests and receives responses to and from SST's memory. In short, at t2, we start the simulation in SST while loading the checkpoint and it will continue until the end of simulation at t3 reaches.

The following diagram shows the workflow of the platform.

```
G t0 : starting simulation in gem5 (atomic/kvm)
E |
M |     t1 : simulation reached the start of ROI
5 |_____|____________________________________________________________ time ->
             |                                                  |
S            t2 : we start the simulation in SST (timing)       |
S                                                               |
T                                           end of simulation : t3
```
This can be scaled into N differnt gem5 nodes. Checkpoints need to be taken for
each of these nodes in their respective first phases.

## Important Notes By Maryam
1. I have tested only ARM ISA in this platform. Thus, eveything is based on ARM ISA in this README. This is the action item that Leo said is working on (adding X86, RISCV, etc)
2. The platform does not allow dynamic sharing of remote memory yet. Thus, the remote memory is shared statially by dividing it into multiple instanes. Whoever runs the scripts needs to take care of address ranges appropriately for this purpose. This is the action item that William said is working on.
3. As of now, we are able to run two suites in full-system simulation: 1. stream 2. a subset of NPB. Thus, our scripts only cover these two suites' experiments.
4. All the resources required to run things are in `/home/babaie/.cache/gem5/`, including the disk image, kernel and bootloader. This should move to the project directory instead.
4. Checkpointing uses KVM CPU. Thus if we are simulating ARM ISA, we need to use an ARM machine with KVM (e.g., azacca) and make sure you are in the KVM group for that.

## Installing SST
Each time you clone the repo you are required to install SST in the repo from scratch.

Easy few steps:

0. Stay in the gem5 root directory.

1. Complie gem5 for the components used for SST
Note: change the number of cpus `-j{N}` appropriately.
```sh
scons build/ARM/libgem5_opt.so -j10 --without-tcmalloc --duplicate-sources
```

2. Compile gem5 for the host nodes that are supposed to run ARM ISA.
```sh
scons build/ARM/gem5.opt -j10 --without-tcmalloc --duplicate-sources
```

3. Go to the sst directory. In this directory there is a script to install the SST libraries automatically.
```sh
cd ext/sst
./SST_INSTALL.sh
```
**Notes**:
1. SST released the latest version back in May 2024. The current script does not install the lates but one before that. We may need to chnage it to the latest version.
2. I suggest to pay attention to the outputs to make sure everything is being installed correctly. You can also try run each command in the script manually once to get to know each steps.

## Taking Chekpoints
A. Stream
Stream is a tiny michrobenchmark of 4 kernels: Copy, Scale, Add, Triad. The source code is available here: **getting the repo link from Hoa/Kaustav**. It creates 3 arrays (A, B, C) of a size (e.g., 64 MiB in our tests) and tries to do the kernels on them. This is already available in the disk image and annotated.
The script for stream to take a checkpoint is:
``` sh
disaggregated_memory/configs/exp-stream-checkpoint.py
```
It comes with two input arguments:
1. `--instance`
You can have a number of hosts , a.k.a. nodes, for instance 4, 16, 32. Each of them will be created based on `ArmComposableMemoryBoard`. Each will have 8 GiB of local memory and only 2 GiB of the total remote memory. The way the shared remote memory address range is divided is as follows:
```sh 
0-2   GiB: always given to the OS and IO. You cannot use this range.
2-10  GiB: given to the 8 GiB local memory.
10-12 GiB: given to 2 GiB remote memory range for host #0
12-14 GiB: given to 2 GiB remote memory range for host #1
14-16 GiB: given to 2 GiB remote memory range for host #2
16-18 GiB: given to 2 GiB remote memory range for host #3
...
```
Thus, to take a checkpoint we need to clarify the host id number and based upon that the appropriate address range (but fixed!) will be assigned to its remote memory range. This happens through the input argument `--instance`.

2. `--memory-allocation-policy`
There is also different memory allocation policy when the system has two sets of memory: 
a. `local`: nothing will be placed in remote and everything will be allocated in the local memory.
b. `interleaved`: page by page, it toggles between allocating from local and remote memories.
c. `remote`: nothing will be placed in local and everything will be allocated in the remote memory.

To run this script, you can use the following command from the gem5 root directory and repeat it per any number of nodes and any memory allocation policy you have, appropriately.

``` sh
build/ARM/gem5.opt --outdir=stream_checkpoints/m5out_0 disaggregated_memory/configs/exp-stream-checkpoint.py --instance=0 --memory-allocation-policy=remote

build/ARM/gem5.opt --outdir=stream_checkpoints/m5out_1 disaggregated_memory/configs/exp-stream-checkpoint.py --instance=1 --memory-allocation-policy=remote

build/ARM/gem5.opt --outdir=stream_checkpoints/m5out_2 disaggregated_memory/configs/exp-stream-checkpoint.py --instance=2 --memory-allocation-policy=remote

```

**Notes:**
1. There is a file named `common.py` in the directory. This file contains all the *common* things between multiple scripts to have them all in one place. Things will be imported from this file as needed.
2. The checkpoint is always saved in a given output directory in a folder named `ckpt_{args.instance}`. So, think about a wholistic folder that contains all the checkpoints in one place, similar to the example above, so when you restore it has access to all the checkpoints within the same diretory.


## Restoring Checkpoints

This is the actual simulation where we simulate multi-host all together and each of them restore their appropriate checkpoint given to them in a directory. The point here is that everything is maintained by SST. We have a script in python for SST that launches multiple gem5 host (node) that are described in a separate script. So, there are two scripts involved here:
1. `disaggregated_memory/SST/exp_arm_stream.py` which is the main SST script. inside of it calls the gem5-restore script below.
2. `disaggregated_memory/configs/exp-stream-restore.py` which is the gem5 script that defines the host node and restores from a checkpoint.

The second script receives some input arguments that are set inside the SST script properly once called. Thus, the SST script will also receive some input argument that based upon them makes the input arguments for the gem5-host-restore. 

The SST scripts receives:
1. `--ckpts-dir`: a wholistic directory containing all the checkpoints.
2. `--system-nodes`: number of nodes/hosts in the system
3. `--memory-allocation-policy`: whether it's local, interleaved or remote, just like in the checkpoint.

Note that the remote memory during the restore (the main simulation that we measure the performance) is simulated in SST and the host nodes are simulated in gem5. So, there is a lot of handshakes and interation between gem5s and SST. SST uses MPI to co-simulate gem5 nodes with itself. We need to define the number of total nodes in the whole system which is always the number of hosts + 1 node for remote memory. Thus, if you have 8 gem5 nodes (hosts) sharing a remote memory you can run it using the command below:
```sh
cd ext/sst
mpirun -np 9 -- bin/sst --add-lib-path=./ ../../disaggregated_memory/SST/exp_arm_stream.py -- --ckpts-dir=/PATH/to/the/checkpoints --system-nodes=9 --memory-allocation-policy=remote
```

**Notes:**
In the SST script:
1. pay attention to the line #86 where it passes the path to the gem5 script: `gem5_run_script`. If it complains, please put the absolute path to it accordingly.

2. `stat_output_directory` will create an output folder inside your checkpoint directory that you passed.

3. Pay attention to lines 136-180 that creates the input arguments to call the gem5 script.

4. in lines 194-195 creates an SST output directory. If it complains it cannot find that (sometimes it happens). manually add the file for it.


A. NPB
NPB scripts work in similar way to Stream; thus, their scripts are very similar.
The major difference is the size of memories and the memory allocation policies. 
We have two policies for NPB:
1. all-local: where everything is put int he local memory. The size of local memory is maxed to the size of maximum workload's required memory (`ft` in this suite that needs 85 GiB).
2. NUMA-local-preferred: where it starts to allocate from local memory, once runs out of memory it alloactes from remote memory. The size of local memory is fixed at 8GiB. The size of remote memory is either 2GiB if the workloads fits in local memory, or the total memory that the workload needs minus the 8GiB of local memory. These sizes are already taken care of in the scripts and in the common file.

There are three scripts involved here:
1. `disaggregated_memory/configs/exp-stream-checkpoint.py`: The script for stream to take a checkpoint. Just like stream, the checkpoints are taken separately per each workload. Workloads are indexed to maintain them easily for remote memory allocation. The indecies are written in the common file.
To take a checkpoint you can use a command as follows:
```sh
build/ARM/gem5.opt --outdir=/A/PATH/TO/SAVE/CHECKPOINTS disaggregated_memory/configs/exp-npb-checkpoint.py --benchmark=bt --size=D --memory-allocation-policy=numa-local-preferred
```
2. `disaggregated_memory/SST/exp_arm_npb.py` which is the main SST script. inside of it calls the gem5-restore script below.
3. `disaggregated_memory/configs/exp-stream-npb.py` which is the gem5 script that defines the host node and restores from a checkpoint.

To restore checkpoints and run all suite together you can use a command as follows:
```sh
cd ext/sst
mpirun -np 8 -- bin/sst --add-lib-path=./ ../../disaggregated_memory/SST/exp_arm_npb.py -- --memory-allocation-policy=numa-local-preferred --ckpts-dir=/A/PATH/TO/RESTORE/CHECKPOINTS
```

