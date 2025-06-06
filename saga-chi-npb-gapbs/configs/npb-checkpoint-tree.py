import argparse
import os
import sys

# all the source files are one directory above.
sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
)

from cachehierarchies_TreeTopology.saga.cache_hierarchy import (
    SagaCacheHierarchy,
)
from common import (
    npb_benchmarks,
    npb_classes,
    npb_mem_size,
)

import m5
from m5.objects import (
    ArmDefaultRelease,
    VExpress_GEM5_Foundation,
)
from m5.objects.ArmSystem import ArmDefaultRelease

from gem5.components.boards.arm_board import ArmBoard
from gem5.components.memory.dram_interfaces.ddr4 import DDR4_2400_8x8
from gem5.components.memory.memory import ChanneledMemory
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
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
    choices=npb_benchmarks,
)
parser.add_argument(
    "--size",
    type=str,
    required=True,
    help="Input the NPB benchmark size",
    choices=npb_classes,
)
parser.add_argument(
    "--ckpt-path",
    type=str,
    required=True,
    help="Input a path to save the checkpoint",
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

# tot_cores = 128
# racks_per_board = 4
# chassis_per_rack = 4
# cluster_per_chassis = 4
# cores_per_cluster = 2

clk_freq = "4GHz"
num_cores = 128
cores_per_socket = 2
num_sockets = num_cores / cores_per_socket
processor = SimpleProcessor(
    cpu_type=CPUTypes.ATOMIC, isa=ISA.ARM, num_cores=num_cores
)
memory = ChanneledMemory(
    DDR4_2400_8x8, num_sockets, 64, size=local_memory_size_GiB
)
cache_hierarchy = SagaCacheHierarchy()
platform = VExpress_GEM5_Foundation()
release = ArmDefaultRelease.for_kvm()

board = ArmBoard(
    clk_freq=clk_freq,
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
    platform=platform,
    release=release,
)

workload = WorkloadResource(
    function="set_kernel_disk_workload",
    parameters={
        "kernel": CustomResource(
            "/home/babaie/.cache/gem5/vmlinux-5.4.49-NUMA.arm64"
        ),
        "bootloader": CustomResource(
            "/home/babaie/.cache/gem5/arm64-bootloader"
        ),
        "disk_image": DiskImageResource(
            "/home/babaie/projects/coherence/npb-x86-diskimage/1/gem5-resources-repo/src/npb-24.04-imgs/disk-image-arm-npb/arm-ubuntu",
            root_partition="2",
        ),
        "readfile_contents": " ".join(command_list),
    },
)

board.set_workload(workload)


def ignore_exit():
    print("Received first expected exit event. Continuing the simulation.")
    yield False  # Continue the simulation.
    print("Received second expected exit event. Continuing the simulation.")
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
