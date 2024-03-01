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
from gem5.components.memory import CascadeLakeCache, OracleCache, RamCache
from gem5.simulate.simulator import ExitEvent
import m5
from components.Octopi import OctopiCache

# Sample command to use this script
# build/RISCV/gem5.opt --outdir=bt.D riscv-2channel-1ccd-checkpoint.py
# --benchmark bt --size D --ckpt_path checkpoints-npb/bt.D.ckpt/
#

requires(isa_required=ISA.RISCV)

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("--benchmark", type=str)
parser.add_argument("--size", type=str)
parser.add_argument("--ckpt_path", type=str)

args = parser.parse_args()

num_ccds = 1
num_cores = 8
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

processor = SimpleSwitchableProcessor(
    starting_core_type=CPUTypes.TIMING,
    switch_core_type=CPUTypes.TIMING, # TODO
    isa=ISA.RISCV,
    num_cores=num_cores
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
    readfile_contents=f"{command}"
)

# The first exit_event ends with a `workbegin` cause. This means that the
# system started successfully and the execution on the program started.
def handle_workbegin():
    print("Done booting Linux")
    print("Resetting stats at the start of ROI!")

    m5.stats.dump()
    m5.stats.reset()

    # switch the processor to timing CPU for warmup
    print("Switching the CPU")
    processor.switch()
    # schedule an exit event for 1 second.
    m5.scheduleTickExitFromCurrent(1000000000000)
    yield False

"""
def handle_cachewarmup():
    print("Cache warmed up as we reached : ")
    print(simulator.get_last_exit_event_cause())
    print("Taking the checkpoint!")

    m5.stats.dump()
    m5.stats.reset()
    save_checkpoint()
    yield True
"""
# Running things for 1sec and will
# not take ckpt if cache gets
# warmed up during Linux boot
def handle_cachewarmup():
    print("Cache warmed up as we reached : ")
    print(simulator.get_last_exit_event_cause())
    print("Will continue simulation!")

    m5.stats.dump()
    m5.stats.reset()
    #save_checkpoint()
    yield False

def handle_schedtick():
    print("Cache warmed up as we reached : ")
    print(simulator.get_last_exit_event_cause())
    print("Taking the checkpoint!")

    m5.stats.dump()
    m5.stats.reset()
    save_checkpoint()
    yield True

def handle_workend():
    print("Dump stats at the end of the ROI!")

    m5.stats.dump()
    yield True


simulator = Simulator(
    board=board,
    on_exit_event={
        ExitEvent.WORKBEGIN: handle_workbegin(),
        ExitEvent.WORKEND: handle_workend(),
        ExitEvent.CACHE_WARMUP: handle_cachewarmup(),
        ExitEvent.SCHEDULED_TICK: handle_schedtick(),
    },
)

def save_checkpoint():
    simulator.save_checkpoint(args.ckpt_path)


print("Beginning simulation!")

simulator.run()

print("End of the simulation, we should have created a ckpt.")
