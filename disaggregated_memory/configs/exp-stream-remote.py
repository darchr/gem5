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
simulation with local and remote memory. These memories are exposed to the OS
as NUMA and zNUMA nodes. This simulation boots Ubuntu 20.04.

This script can be executed both from gem5 and SST.
"""

import argparse
import os
import sys

# all the source files are one directory above.
sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
)

from boards.arm_main_board import ArmComposableMemoryBoard
from memories.external_remote_memory import ExternalRemoteMemory

import m5
from m5.objects import (
    AddrRange,
    ArmDefaultRelease,
    Root,
)
from m5.objects.RealView import VExpress_GEM5_V1
from m5.util import warn

from gem5.resources.resource import *
from gem5.resources.workload import *
from gem5.resources.workload import Workload
from gem5.simulate import exit_event_generators
from gem5.simulate.exit_event import ExitEvent
from gem5.simulate.simulator import Simulator
from gem5.utils.requires import requires

# SST passes a couple of arguments for this system to simulate.
parser = argparse.ArgumentParser()

parser.add_argument(
    "--instance",
    type=int,
    required=True,
    help="Instance id is need to correctly read and write to the "
    + "checkpoint in a multi-node simulation.",
)
parser.add_argument(
    "--remote-memory-addr-range",
    type=str,
    required=True,
    help="Remote memory range",
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
    type=bool,
    default=False,
    required=True,
    help="optionally put a path to restore a checkpoint",
)

args = parser.parse_args()

remote_memory_range = list(map(int, args.remote_memory_addr_range.split(",")))
remote_memory_range = AddrRange(remote_memory_range[0], remote_memory_range[1])

# Here we setup the board which allows us to do Full-System ARM simulations.
board = ArmComposableMemoryBoard(remote_memory_range, use_sst=True)

# commands to execute to run the simulation.
mount_cmd = ["mount -t sysfs - /sys;", "mount -t proc - /proc;"]

warn("The command list to execute has to be manually set!")

remote_stream = [
    'echo "starting STREAM remotely!";',
    "numastat;",
    "numactl --membind=1 -- "
    + "/home/ubuntu/simple-vectorizable-benchmarks/stream/"
    + "stream.hw.m5 8388608;",
    "numastat;",
]

# Since we are using kvm to boot the system, we can boot the system with
# systemd enabled!

###############
cmd = remote_stream + ["m5 --addr=0x10010000 exit;"]
###############


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

if args.take_ckpt:
    exit_event = exit_event_generators.save_checkpoint_generator
else:
    exit_event = exit_event_generators.exit_generator

simulator = Simulator(
    board=board,
    on_exit_event={
        ExitEvent.EXIT: exit_event,
    },
)

if not use_sst:
    simulator.run()
else:
    # This is called in SST. SST will take care of running this script.
    # SST won't call instantiate, though, since it doesn't use the simulator
    # object
    simulator._instantiate()
