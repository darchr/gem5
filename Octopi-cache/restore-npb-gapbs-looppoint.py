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
    MySimpleProcessor,
)
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.memory import CascadeLakeCache, OracleCache, RambusCache
from gem5.simulate.simulator import ExitEvent
import m5
from components.Octopi import OctopiCache
from pathlib import Path
from m5.objects import O3LooppointAnalysis, O3LooppointAnalysisManager


# Sample command to use this script
# build/RISCV_MESI_Three_Level/gem5.opt -re --outdir=m5out Octopi-cache/restore-npb-gapbs-looppoint.py RambusBaseline bt 128MiB

requires(isa_required=ISA.RISCV)

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("experiment", type=str)
parser.add_argument("benchmark", type=str)
parser.add_argument("cache_size", type=str)

args = parser.parse_args()

ckpt_path = "/projects/gem5/dramcache/jason-checkpoints/npb/c/"+args.benchmark+"/ckpt"
print("Checkpoint directory: "+ckpt_path)

# Valid addresses for looppoint analysis
# The addresses are the .text section range
text_start = {
    "bt.C": "111b0",
    "cg.C": "112a0",
    "ft.C": "113a0",
    "is.C": "10df0",
    "lu.C": "11450",
    "mg.C": "11630",
    "sp.C": "11250",
    "ua.C": "11aa0",
    "bt.D": "111b0",
    "cg.D": "112a0",
    "ft.D": "113a0",
    "lu.D": "11450",
    "mg.D": "11630",
    "sp.D": "11250",
    "ua.D": "11aa0",
    "bc": "12c80",
    "bfs": "12bb0",
    "cc": "12e60",
    "cc_sv": "12ca0",
    "pr": "12dd0",
    "sssp": "12d80",
    "tc": "12c00",
}

text_size = {
    "bt.C": "e14a",
    "cg.C": "2b84",
    "ft.C": "353a",
    "is.C": "1692",
    "lu.C": "df12",
    "mg.C": "66ca",
    "sp.C": "b31e",
    "ua.C": "190f4",
    "bt.D": "e500",
    "cg.D": "2a4a",
    "ft.D": "35ba",
    "lu.D": "e0e0",
    "mg.D": "687a",
    "sp.D": "b7d4",
    "ua.D": "191f0",
    "bc": "af82",
    "bfs": "a008",
    "cc": "a51a",
    "cc_sv": "98a4",
    "pr": "9b14",
    "sssp": "acf8",
    "tc": "9cc2",
}


num_ccds = 1
num_cores = 8
command = (
    f"/home/ubuntu/gem5-npb/NPB3.3-OMP/bin/{args.benchmark}.C.x;"
)

cache_hierarchy = OctopiCache(
    l1i_size="32KiB",
    l1i_assoc=8,
    l1d_size="32KiB",
    l1d_assoc=8,
    l2_size="512KiB",
    l2_assoc=8,
    l3_size="32MiB",
    l3_assoc=16,
    num_core_complexes=1,
    is_fullsystem=True,
)

if args.experiment == "RambusBaseline":
    memory = RambusCache(args.cache_size)
elif args.experiment == "CascadeLakeBaseline":
    memory = CascadeLakeCache(args.cache_size)
elif args.experiment == "OracleBaseline":
    memory = OracleCache(args.cache_size)

processor = MySimpleProcessor(
    starting_core_type=CPUTypes.O3,
    switch_core_type=CPUTypes.O3,
    isa=ISA.RISCV,
    num_cores=num_cores,
)

lpmanager = O3LooppointAnalysisManager()

# dump the tick and the PC counter as well

for core in processor.get_cores():
    lplistener = O3LooppointAnalysis()
    lplistener.ptmanager = lpmanager
    lplistener.validAddrRangeStart = int(text_start[args.benchmark+".C"], 16)
    lplistener.validAddrRangeSize = int(text_size[args.benchmark+".C"], 16)
    core.core.probeListener = lplistener


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
    checkpoint=Path(ckpt_path),
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

        print("Most recent (PC, count, tick) at this point in simulation")
        mostRecentPc = lpmanager.getMostRecentPc()
        for pc, tick in mostRecentPc:
            count = lpmanager.getPcCount(pc)
            print(f"{hex(pc)},{count[0]},{count[1]}\n")

        m5.scheduleTickExitFromCurrent(100_000_000_000, "progress_update")
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
        ExitEvent.SCHEDULED_TICK_PROGRESS: handle_progress_update(),
    },
)

print("Beginning simulation!")
print(f"Will restore the checkpoint from : {ckpt_path} ")

# Start the simulation so that the 
# scheduleTickExitFromCurrent() can be used
#simulator.run(1000)
# progress update
#m5.scheduleTickExitFromCurrent(100_000_000_000, "progress_update")

def DumpLoopPointCounters():
    mostRecentPc = lpmanager.getMostRecentPc()
    print("Most recent (PC, count, tick) at the end of the simulation")
    for pc, tick in mostRecentPc:
        count = lpmanager.getPcCount(pc)
        print(f"{hex(pc)},{count[0]},{count[1]}")

# Simulate 1s in 10 intervals
numIteration = 10

print("Simulating ten intervals of 100ms!")

for interval_number in range(numIteration):
    print("Interval number: {} \n".format(interval_number))
    simulator.run(100_000_000_000)
    print(f"Exiting simulation loop because of : {simulator.get_last_exit_event_cause()}")
    DumpLoopPointCounters()
    m5.stats.dump()
    
print("End of the simulation!")
