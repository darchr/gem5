from gem5.components.boards.riscv_board import RiscvBoard
from gem5.components.memory.memory import ChanneledMemory
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.utils.requires import requires
from gem5.utils.override import overrides
from gem5.resources.resource import Resource, DiskImageResource
from gem5.simulate.simulator import Simulator
from gem5.components.processors.simple_switchable_processor import (
    SimpleSwitchableProcessor,
)
from gem5.components.processors.simple_processor import (
    SimpleProcessor,
)
from gem5.components.memory import CascadeLakeCache, OracleCache, RamCache
from gem5.simulate.simulator import ExitEvent
import m5
from components.Octopi import OctopiCache
from pathlib import Path

# Sample command to use this script
# build/RISCV/gem5.opt -re --outdir=/projects/gem5/sssp16-test/ Octopi-cache/riscv-2channel-1ccd-restore-timing-gapbs.py --benchmark sssp --size 16 --ckpt_path /projects/gem5/dramcache/jason-checkpoints/gapbs/16/sssp/ckpt
# build/RISCV/gem5.opt -re --outdir=/projects/gem5/sssp16-test/ Octopi-cache/riscv-2channel-1ccd-restore-timing-gapbs.py --benchmark sssp --size C --ckpt_path /projects/gem5/dramcache/jason-checkpoints/npb/c/bt/ckpt

requires(isa_required=ISA.RISCV)

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("--benchmark", type=str)
parser.add_argument("--size", type=str)
parser.add_argument("--ckpt_path", type=str)

args = parser.parse_args()

num_ccds = 1
num_cores = 8
#command = f"/home/ubuntu/gapbs/{args.benchmark} -g {args.size};"
command = f"/home/ubuntu/gem5-npb/NPB3.3-OMP/bin/{args.benchmark}.{args.size}.x;"

cache_hierarchy = OctopiCache(
    l1i_size  = "32KiB",
    l1i_assoc = 8,
    l1d_size  = "32KiB",
    l1d_assoc = 8,
    l2_size  = "512KiB",
    l2_assoc = 8,
    l3_size = "32MiB",
    l3_assoc = 16,
    num_core_complexes = 1,
    is_fullsystem = True,
)
memory = RamCache()

"""
processor = SimpleSwitchableProcessor(
    starting_core_type=CPUTypes.TIMING,
    switch_core_type=CPUTypes.TIMING, # TODO
    isa=ISA.RISCV,
    num_cores=num_cores
)
"""

processor = SimpleProcessor(
    cpu_type=CPUTypes.TIMING, isa=ISA.RISCV, num_cores=num_cores
)

class HighPerformanceRiscvBoard(RiscvBoard):
    @overrides(RiscvBoard)
    def get_default_kernel_args(self):
        return [
            "earlyprintk=ttyS0",
            "console=ttyS0",
            "lpj=7999923",
            "root=/dev/vda1",
            "init=/root/init.sh",
            "rw",
        ]

# Setup the board.
board = HighPerformanceRiscvBoard(
    clk_freq="4GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

# Set the Full System workload.
board.set_kernel_disk_workload(
    kernel=Resource("riscv-bootloader-vmlinux-5.10"),
    disk_image=DiskImageResource(
        "/projects/gem5/npb-gapbs-riscv.img", root_partition="1"
    ),
    readfile_contents=f"{command}",
    checkpoint=Path(args.ckpt_path)
)

# The first exit_event ends with a `workbegin` cause. This means that the
# system started successfully and the execution on the program started.
def handle_workbegin():
    print("Unexpected case!")
    print("Workload begin should have not hit again!")
    yield False

# This event is scheduled every 100ms to get an
# update on the progress of the simulation itself
def handle_progress_update():

    while True:
        print("Simulated another 100ms")

        print(
            f"Total sim time so far : {simulator.get_current_tick() / 1e12} sec"
        )

        m5.scheduleTickExitFromCurrent(100000000000, "progress_update")
        yield False

def handle_cachewarmup():
    # we probably should never hit this
    # case during ckpt restore
    print("DRAM Cache warmed up!")
    print("Will continue simulation!")
    yield False

def handle_schedtick():
    print("Terminating the simulation because : ")
    print(simulator.get_last_exit_event_cause())
    m5.stats.dump()
    yield True

def handle_workend():
    print("Terminating the simulation because : ")
    print(simulator.get_last_exit_event_cause())
    m5.stats.dump()
    yield True


simulator = Simulator(
    board=board,
    on_exit_event={
        ExitEvent.WORKBEGIN: handle_workbegin(),
        ExitEvent.WORKEND: handle_workend(),
        ExitEvent.CACHE_WARMUP: handle_cachewarmup(),
        ExitEvent.SCHEDULED_TICK: handle_schedtick(),
        #ExitEvent.SCHEDULED_TICK_PROGRESS: handle_progress_update(),
    },
)

print("Beginning simulation!")
print(f"Will restore the checkpoint from : {args.ckpt_path} ")

# progress update
#m5.scheduleTickExitFromCurrent(100000000000, "progress_update")

# Running the actual simulation for 1 second
#m5.scheduleTickExitFromCurrent(1000000000000)
simulator.run(1000000000000)

print("End of the simulation!")
