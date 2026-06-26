# Parallel Efficiency

## Updated time comparison

In the revised version of the manuscript, we directly compare the host times to execute the 4 STREAM kernels when the number of hosts were increased in gem5 + SST and only gem5.
See the main REAMDE-DM.md to start this experiment and generate the results.

## Experiment 1

This experiment demonstrates how efficient the gem5-SST setup is in terms of parallel efficiency.
We define parallel efficiency as:

$$
P.E = \frac{1}{host processes} \times \frac{linear time taken to simulate N nodes}{time taken using the gem5 SST setup}
$$

We simulate the ROI of a single node remotely pinned STREAM and record the simulated time.
Then we linearly scale this time for N number of system nodes.
We compare this time against our gem5 SST simulations.

*Note*
For this experiment, we simulate memory node on the same process as one of the gem5 nodes.
The number of processes in both the gem5 and the gem5-SST setup is same when N nodes are simulated.

To start the simulation:
```sh
# For 1 node system
python3 disaggregated_memory/unified_run.py --exp-name=pe_1_node \
                    --count=1 \
                    --joblist=disaggregated_memory/joblist/parallel_efficiency/parallel_efficiency/joblist_stream_remote_1_nodes.json | tee mpi_1_node_pe.txt
# For 2 nodes system
python3 disaggregated_memory/unified_run.py --exp-name=pe_2_nodes \
                    --count=2 \
                    --joblist=disaggregated_memory/joblist/parallel_efficiency/parallel_efficiency/joblist_stream_remote_2_nodes.json | tee mpi_2_node_pe.txt
# For 4 nodes system
python3 disaggregated_memory/unified_run.py --exp-name=pe_4_nodes \
                    --count=4 \
                    --joblist=disaggregated_memory/joblist/parallel_efficiency/parallel_efficiency/joblist_stream_remote_4_nodes.json | tee mpi_4_node_pe.txt
# For 8 nodes system
python3 disaggregated_memory/unified_run.py --exp-name=pe_8_nodes \
                    --count=8 \
                    --joblist=disaggregated_memory/joblist/parallel_efficiency/parallel_efficiency/joblist_stream_remote_8_nodes.json | tee mpi_8_node_pe.txt
# For 16 nodes system
python3 disaggregated_memory/unified_run.py --exp-name=pe_16_nodes \
                    --count=16 \
                    --joblist=disaggregated_memory/joblist/parallel_efficiency/parallel_efficiency/joblist_stream_remote_16_nodes.json | tee mpi_16_node_pe.txt
```

The simulation statistics of simulating multiple MPI ranks is in the output of `mpirun`.
For the gem5 simulation, the stats.txt has `hostTicks`.

## Experiment 2

We also compare the actual runtimes of the two setup, we also model a similar setup using gem5 only.
One gem5 process models 1 system node, 2 system nodes and 4 system nodes alongside the remote memory node as our baseline.
The runtimes are then compared with an equal number of system nodes simulated using gem5 SST.
The gem5-only setup does not support KVM for fast-forwarding as the local memory ranges overlap across the memory nodes.
Therefore, we do en entire boot without systemd, run the 4 kernels and note the time.

To fast-forward the setup, we use timing cores.
There are no checkpoints taken in the gem5-SST setup to keep the comparison fair.

To start the experiment:
```sh
# For 1 node system
python3 disaggregated_memory/unified_run.py --exp-name=time_compare_1_node \
                    --count=1 \
                    --checkpoints=false \
                    --joblist=disaggregated_memory/joblist/parallel_efficiency/time_compare/joblist_stream_remote_1_nodes.json | tee mpi_1_node.txt
# For 2 nodes system
python3 disaggregated_memory/unified_run.py --exp-name=time_compare_1_node \
                    --count=2 \
                    --checkpoints=false \
                    --joblist=disaggregated_memory/joblist/parallel_efficiency/time_compare/joblist_stream_remote_2_nodes.json | tee mpi_2_node.txt
# For 4 nodes system
python3 disaggregated_memory/unified_run.py --exp-name=time_compare_1_node \
                    --count=4 \
                    --checkpoints=false \
                    --joblist=disaggregated_memory/joblist/parallel_efficiency/time_compare/joblist_stream_remote_4_nodes.json | tee mpi_4_node.txt
```

To simulate the gem5,
```sh
# For 1 node system
build/ALL/gem5.opt disaggregated_memory/joblist/parallel_efficiency/time_compare/gem5_script.py --nodes=1
# For 2 nodes system
build/ALL/gem5.opt disaggregated_memory/joblist/parallel_efficiency/time_compare/gem5_script.py --nodes=2
# For 4 nodes system
build/ALL/gem5.opt disaggregated_memory/joblist/parallel_efficiency/time_compare/gem5_script.py --nodes=1
```

The simulation statistics of simulating multiple MPI ranks is in the output of `mpirun`.
For the gem5 simulation, the stats.txt has `hostTicks`.
