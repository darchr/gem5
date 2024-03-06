# Taking and restoring checkpoints in gem5 + SST

This readme contains the progress of checkpointing in gem5 + SST setup.

## Status of the bridge

The bridge only works with timing mode.
It is inherited from AbstractMem

## Code

```sh
git clone git@github.com:darchr/gem5.git
cd gem5
git checkout kg/dm-sst-checkpoints-ver2

# build libgem5
# build sst
```

## Approach

### Gem5 only setup

1. For taking and restoring checkpoints in gem5-only setup use the following
   command:
```sh
build/ARM/gem5.opt --outdir=<where-the-outputs-and-checkpoint-will-be-placed> disaggregated_memory/configs/arm-gem5-only-numa-checkpoint-restore.py --take-ckpt=True
```
2. To restore a checkpoint in a gem5-only setup, use the following command:
```sh
build/ARM/gem5.opt disaggregated_memory/configs/arm-gem5-only-numa-checkpoints.py --take-ckpt=False --restore-ckpt=True --ckpt-file<path/to/a/checkpoint/dir>
```

### Gem5 + SST setup

1. Currently, we take checkpoint using a gem5 only setup with a local and a remote memory.
2. The address ranges have to be modified when restoring the checkpoint's physical memory.
3. Make sure that the CPU type is either KVM or ATOMIC. This keeps the request
   and the data at the outgoing bridge.
```sh
build/ARM/gem5.opt disaggregated_memory/configs/arm-numa-checkpoints.py --cpu-clock-rate 3GHz --instance 0 --cpu-type atomic --local-memory-size 2GiB --remote-memory-addr-range 4294967296,38654705664 --remote-memory-latency 750 --take-ckpt=True --ckpt-file=<path/to/a/checkpoint/dir>
```
4. The physmem file 0 *usually* corresponds to the remote memory.
5. In the `outgoing_request_bridge.cc` file, make sure that the physical memory
   file is correctly read. In the `unserialize()` method, modify this line:
```cpp
std::string physical_memory_file = "board.physmem.store0.pmem";
```
6. Make sure that the delta to restore the checkpoint in the `ext/sst/gem5.cc`
   is set correctly. It should be correctly set by default.
```cpp
     // Tick conversion
     // The main logic for synchronize SST Tick and gem5 Tick is here.
     // next_end_tick = current_cycle * timeConverter->getFactor()
     if(flag == false) {
         flag = true;
         base_time = gem5::curTick();
         std::cout << base_time << std::endl;
     }
     uint64_t next_end_tick = \
         timeConverter->convertToCoreTime(current_cycle) + base_time;
```
7. We manually read and write the physical memory file. To debug this, print
   the contents of `initData` in the `outgoing_request_bridge.cc`.
8. To restore the previously created checkpoint from SST, use the following
   command:
```sh
bin/
```
9. The execution should resume normally.

