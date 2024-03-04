# Taking and restoring checkpoints in gem5 + SST

This readme contains the progress of checkpointing in gem5 + SST setup.

## Status of the bridge

The bridge only works with timing mode.
It is inherited from SimObject.

## Code

```sh
git clone git@github.com:kaustav-goswami/gem5.git
cd gem5
git checkout kg/sst-checkpoints

# build libgem5
# build sst
```

## Approach

1. Currently, we take checkpoint using a gem5 only setup with a local and a remote memory.
2. The address ranges have to be modified when restoring the checkpoint's physical memory.
3. The `m5.cpt` file has to be modified to remove the non-existent SimObjects (this has to be corrected ASAP (see TODO section.))
4. We manually read and write the physical memory file.

## Taking a checkpoint

```sh
# inside the gem5 directory
build/ALL/gem5.opt disaggregated_memory/configs/checkpoints/arm-gem5-take-ckpt.py
# The checkpoint will be created in `gem5-ckpt` directory.
```

## Restoring the checkpoint in SST

1. First we need to modify the `m5.cpt` file.
    Do not restore the final `physmem` so modify the following line from `nbr_of_stores=9` to `nbr_of_stores=8`
2. You'd need to modify the SimObjects which does not exist in the restore system, if any.
3. The final `physmem` file is manually traversed and loaded into the memory right now.
4. Look at `unserialize()` method on how the restoring part is implemented.
5. We are using the `initData` object to push the data into SST's backend memory.

```sh
# in the ext/sst directory
bin/sst  --add-lib-path=./ sst/example_arm_dm_board_ckpt.py
# the restore script is here
# disaggregated_memory/configs/checkpoints/arm-gem5-restore-ckpt.py
```

## TODO Items in the short time

1. RemoteExternalMemory should inherit from `AbstractMemorySystem`.