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
from common import gapbs_mem_size, gapbs_benchmarks

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
    help="Input the GAPBS benchmark name",
    choices=gapbs_benchmarks
)
parser.add_argument(
    "--size",
    type=str,
    required=True,
    help="Input the GAPBS benchmark size",
    choices=["22", "25"]
)
parser.add_argument(
    "--ckpt-path",
    type=str,
    required=True,
    help="Input a path to save the checkpoint",
)
args = parser.parse_args()

local_memory_size_GiB = str(gapbs_mem_size[f"{args.benchmark}.{args.size}"]) + "GiB"
command_list = []
command = "echo 12345 | sudo -S ./gapbs/" + args.benchmark + " -g " + args.size
print(command)
command_list = [
    f"{command};",
]

requires(isa_required=ISA.ARM)

clk_freq="4GHz"
num_cores=16
cores_per_socket=2
num_sockets=num_cores/cores_per_socket
processor=SimpleProcessor(cpu_type=CPUTypes.ATOMIC, isa=ISA.ARM, num_cores=num_cores)
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

workload = WorkloadResource(
    function="set_kernel_disk_workload",
    parameters={
        "kernel": CustomResource("/home/babaie/.cache/gem5/vmlinux-5.4.49-NUMA.arm64"),
        "bootloader": CustomResource(
            "/home/babaie/.cache/gem5/arm64-bootloader"
        ),
        "disk_image": DiskImageResource(
            "/home/babaie/projects/coherence/gapbs-diskimage/1/gem5-resources-repo/src/gapbs/arm-disk-image-24-04/arm-ubuntu",
            root_partition="2",
        ),
        "readfile_contents": " ".join(command_list),
    },
)

board.set_workload(workload)
# board.append_kernel_arg("interactive=true")

def ignore_exit():
    yield False  # Continue the simulation.
    yield False  # Continue the simulation.
# define on_exit_event
def take_checkpoint():
    m5.checkpoint(args.ckpt_path)
    yield True  # Stop the simulation. We're done.

simulator = Simulator(
    board=board,
    on_exit_event={
        ExitEvent.EXIT: ignore_exit(),
        ExitEvent.WORKBEGIN: take_checkpoint(),
    },
)

simulator.run()