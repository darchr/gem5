import argparse
import os
import sys

# all the source files are one directory above.
sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
)

from cachehierarchies.saga.cache_hierarchy import SagaCacheHierarchy

from gem5.components.memory.memory import ChanneledMemory
from gem5.components.memory.dram_interfaces.ddr4 import DDR4_2400_8x8
from gem5.isas import ISA
from gem5.components.boards.arm_board import ArmBoard
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
from m5.objects.ArmSystem import ArmDefaultRelease
from m5.objects import (
    ArmDefaultRelease,
    VExpress_GEM5_Foundation,
)
from common import npb_mem_size, npb_benchmarks, npb_classes

import m5
from gem5.isas import ISA
from gem5.resources.resource import *
from gem5.resources.workload import *
from gem5.simulate.exit_event import ExitEvent
from gem5.simulate.simulator import Simulator
from gem5.utils.requires import requires

parser = argparse.ArgumentParser()
parser.add_argument(
    "--benchmark",
    type=str,
    required=True,
    help="Input the NPB benchmark name",
    choices=npb_benchmarks
)
parser.add_argument(
    "--size",
    type=str,
    required=True,
    help="Input the NPB benchmark size",
    choices=npb_classes
)
parser.add_argument(
    "--ckpt-path",
    type=str,
    default="",
    required=True,
    help="Put a path to restore a checkpoint",
)
args = parser.parse_args()

benchmark = f"{args.benchmark}.{args.size}.x"
local_memory_size = npb_mem_size[benchmark]
local_memory_size_GiB = str(local_memory_size) + "GiB"
command_list = []
command = "echo 12345 | sudo -S /home/gem5/NPB3.4-OMP/bin/" + benchmark
command_list = [
    f"{command};",
]


requires(isa_required=ISA.ARM)

clk_freq="4GHz"
num_cores=16
cores_per_socket=2
num_sockets=num_cores/cores_per_socket
processor=SimpleProcessor(cpu_type=CPUTypes.O3, isa=ISA.ARM, num_cores=num_cores)
memory=ChanneledMemory(DDR4_2400_8x8, num_sockets, 64, size=local_memory_size_GiB)
cache_hierarchy=SagaCacheHierarchy()
platform=VExpress_GEM5_Foundation()
release=ArmDefaultRelease.for_kvm()

board = ArmBoard(
    clk_freq = clk_freq,
    processor = processor,
    memory = memory,
    cache_hierarchy = cache_hierarchy,
    platform = platform,
    release = release,
)

workload = CustomWorkload(
    function="set_kernel_disk_workload",
    parameters={
        "kernel": CustomResource("/home/babaie/.cache/gem5/vmlinux-5.4.49-NUMA.arm64"),
        "bootloader": CustomResource(
            "/home/babaie/.cache/gem5/arm64-bootloader"
        ),
        "disk_image": DiskImageResource(
            "/home/babaie/Downloads/arm-ubuntu-20240828",
            root_partition="2",
        ),
        "readfile_contents": " ".join(command_list),
    },
)

board.set_workload(workload)

# define on_exit_event
def handle_exit_event():
    for num_iterations in range(4):
        print(f"Done with iteration #{num_iterations}")
        m5.stats.dump()
        print(f"Dumped stats at the end of the iteration #{num_iterations}")
        m5.setMaxTick(m5.curTick() + 100_000_000_000) # simulate another 100 ms
        yield False  # Continue the simulation.
    print(f"Dump stats since all the iterations completed")
    m5.stats.dump()
    yield True  # Stop the simulation. We're done.

simulator = Simulator(
    board=board,
    on_exit_event={
        ExitEvent.MAX_TICK : handle_exit_event(),
    },
    checkpoint_path=args.ckpt_path,
)

simulator._instantiate()

m5.setMaxTick(m5.curTick() + 100_000_000_000)

simulator.run()