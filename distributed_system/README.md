# System Descriptions

## Vanilla System

This system is essentially equivalent to `configs/example/gem5_library/riscv-ubuntu-run.py` with an exception of using a different version of the disk image.

- `gem5_node_configs/riscv-vanilla-system.py`: gem5 configuration specifying a whole RISC-V node without any outside connection. OpenSBI bootloader, Ubuntu 22.04 disk image.
- `sst_configs/3-vanilla-node.py`: SST configuration specifying a cluster containing 3 gem5 nodes. No communication between the nodes or between each node and SST.

To start the simulation,

```sh
mpiexec sst --partitioner=sst.self --add-lib-path=./ --output-directory=sst_output/ ../../distributed_system/sst_configs/3-vanilla-nodes.py
```
