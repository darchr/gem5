# Copyright (c) 2023-24 The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""
This script shows an example of running a full system ARM Ubuntu boot
simulation using the gem5 library. This simulation boots Ubuntu 20.04 using
1 TIMING CPU cores and executes `STREAM`. The simulation ends when the
startup is completed successfully.

* This script has to be executed from SST
"""

import argparse
import os
import sys

# all the source files are one directory above.
sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
)

from boards.arm_main_board import ArmComposableMemoryBoard
from cachehierarchies.dm_caches import ClassicPrivateL1PrivateL2SharedL3DMCache
from memories.external_remote_memory_v2 import ExternalRemoteMemoryV2

import m5
from m5.objects import (
    AddrRange,
    ArmDefaultRelease,
    Root,
)
from m5.objects.RealView import VExpress_GEM5_V1
from m5.util import warn

from gem5.components.memory import (
    DualChannelDDR4_2400,
    SingleChannelDDR4_2400,
)
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.isas import ISA
from gem5.resources.resource import *
from gem5.resources.workload import *
from gem5.resources.workload import Workload
from gem5.simulate.simulator import Simulator
from gem5.utils.requires import requires

# SST passes a couple of arguments for this system to simulate.
parser = argparse.ArgumentParser()

# basic parameters.
parser.add_argument(
    "--cpu-type",
    type=str,
    choices=["atomic", "timing", "o3", "kvm"],
    default="atomic",
    help="CPU type",
)
parser.add_argument(
    "--cpu-clock-rate",
    type=str,
    required=True,
    help="CPU Clock",
)
parser.add_argument(
    "--instance",
    type=int,
    required=True,
    help="Instance id is need to correctly read and write to the "
    + "checkpoint in a multi-node simulation.",
)

# Parameters related to local memory
parser.add_argument(
    "--local-memory-size",
    type=str,
    required=True,
    help="Local memory size",
)

# Parameters related to remote memory
parser.add_argument(
    "--is-composable",
    type=str,
    required=True,
    choices=["True", "False"],
    help="Tell the simulation to either use gem5 or SST as the remote memory.",
)
parser.add_argument(
    "--remote-memory-addr-range",
    type=str,
    required=True,
    help="Remote memory range",
)
parser.add_argument(
    "--remote-memory-latency",
    type=int,
    required=True,
    help="Remote memory latency in Ticks (has to be converted prior)",
)

# Parameters related to checkpoints.
parser.add_argument(
    "--ckpt-file",
    type=str,
    default="",
    required=False,
    help="optionally put a path to restore a checkpoint",
)
parser.add_argument(
    "--take-ckpt",
    type=str,
    default="False",
    required=True,
    help="optionally put a path to restore a checkpoint",
)

args = parser.parse_args()

cpu_type = {
    "o3": CPUTypes.O3,
    "atomic": CPUTypes.ATOMIC,
    "timing": CPUTypes.TIMING,
    "kvm": CPUTypes.KVM,
}[args.cpu_type]
use_sst = {"True": True, "False": False}[args.is_composable]

remote_memory_range = list(map(int, args.remote_memory_addr_range.split(",")))
remote_memory_range = AddrRange(remote_memory_range[0], remote_memory_range[1])

# This runs a check to ensure the gem5 binary is compiled for ARM.
requires(isa_required=ISA.ARM)

# Here we setup the parameters of the l1 and l2 caches.
cache_hierarchy = ClassicPrivateL1PrivateL2SharedL3DMCache(
    l1d_size="32KiB", l1i_size="32KiB", l2_size="256KiB", l3_size="4MiB"
)

# Memory: Dual Channel DDR4 2400 DRAM device.
local_memory = DualChannelDDR4_2400(size=args.local_memory_size)

# Either suppy the size of the remote memory or the address range of the
# remote memory. Since this is inside the external memory, it does not matter
# what type of memory is being simulated. This can either be initialized with
# a size or a memory address range, which is mroe flexible. Adding remote
# memory latency automatically adds a non-coherent crossbar to simulate latency
remote_memory = ExternalRemoteMemoryV2(
    addr_range=remote_memory_range, use_sst_sim=use_sst
)

# Here we setup the processor. We use a simple processor.
processor = SimpleProcessor(cpu_type=cpu_type, isa=ISA.ARM, num_cores=4)

# Here we setup the board which allows us to do Full-System ARM simulations.
board = ArmComposableMemoryBoard(
    clk_freq=args.cpu_clock_rate,
    processor=processor,
    local_memory=local_memory,
    remote_memory=remote_memory,
    cache_hierarchy=cache_hierarchy,
    platform=VExpress_GEM5_V1(),
    release=ArmDefaultRelease.for_kvm(),
)

# commands to execute to run the simulation.
mount_cmd = ["mount -t sysfs - /sys;", "mount -t proc - /proc;"]

warn("The command list to execute has to be manually set!")

local_stream = [
    'echo "starting STREAM locally!";',
    "numastat;",
    "numactl --membind=0 -- "
    + "/home/ubuntu/simple-vectorizable-benchmarks/stream/"
    + "stream.hw.m5 10000000;",
    "numastat;",
]

interleave_stream = [
    'echo "starting interleaved STREAM!";',
    "numastat;",
    "numactl --interleave=0,1 -- "
    + "/home/ubuntu/simple-vectorizable-benchmarks/stream/"
    + "stream.hw.m5 10000000;",
    "numastat;",
]

remote_stream = [
    'echo "starting STREAM remotely!";',
    "numastat;",
    "numactl --membind=1 -- "
    + "/home/ubuntu/simple-vectorizable-benchmarks/stream/"
    + "stream.hw.m5 10000000;",
    "numastat;",
]

# Since we are using kvm to boot the system, we can boot the system with
# systemd enabled!
cmd = (
    ["m5 --addr=0x10010000 exit;"]
    + local_stream
    + interleave_stream
    + remote_stream
    + ["m5 --addr=0x10010000 exit;"]
)

workload = CustomWorkload(
    function="set_kernel_disk_workload",
    parameters={
        "kernel": CustomResource("/home/kaustavg/vmlinux-5.4.49-NUMA.arm64"),
        "bootloader": CustomResource(
            "/home/kaustavg/kernel/arm/bootloader/arm64-bootloader"
        ),
        "disk_image": DiskImageResource(
            "/home/kaustavg/disk-images/arm/arm64-hpc-2204-numa-kvm.img-20240304",
            root_partition="1",
        ),
        "readfile_contents": " ".join(cmd),
    },
)

ckpt_to_read_write = ""
if args.ckpt_file != "":
    ckpt_to_read_write = (
        os.getcwd()
        + "/"
        + m5.options.outdir
        + "/"
        + args.ckpt_file
        + str(args.instance)
    )
    # inform the user where the checkpoint will be saved
    print("Checkpoint will be saved in " + ckpt_to_read_write)
else:
    warn("A checkpoint path was not provided!")

# This disk image needs to have NUMA tools installed.
board.set_workload(workload)

# This script will boot two NUMA nodes in a full system simulation where the
# gem5 node will be sending instructions to the SST node. the simulation will
# after displaying numastat information on the terminal, which can be viewed
# from board.terminal.
board._pre_instantiate()
root = Root(full_system=True, board=board)
board._post_instantiate()


# define on_exit_event
def handle_exit():
    yield True  # Stop the simulation. We're done.


# Here are the different scenarios:
# no checkpoint, run everything in gem5
if args.take_ckpt == "True":
    if args.cpu_type == "kvm":
        # ensure that sst is not being used here.
        assert use_sst == False
        root.sim_quantum = int(1e9)
    m5.instantiate()

    # probably this script is being called only in gem5. Since we are not using
    # the simulator module, we might have to add more m5.simulate()
    m5.simulate()
    if ckpt_to_read_write != "":
        m5.checkpoint(os.path.join(os.getcwd(), ckpt_to_read_write))
else:
    # This is called in SST. SST will take care of running this script.
    # Instantiate the system regardless of the simulator.
    m5.instantiate(ckpt_to_read_write)

    # we can still use gem5. So making another if-else
    if use_sst == False:
        m5.simulate()
    # otherwise just let SST do the simulation.
