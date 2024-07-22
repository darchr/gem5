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

sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
)

from boards.arm_saga_board import ArmComposableMemoryBoard
from common import stream_run_commands, stream_remote_memory_address_ranges

from m5.objects import AddrRange
from gem5.isas import ISA
from gem5.resources.resource import *
from gem5.resources.workload import *
from gem5.utils.requires import requires
from gem5.simulate import exit_event_generators
from gem5.simulate.exit_event import ExitEvent
from gem5.simulate.simulator import Simulator

parser = argparse.ArgumentParser()
parser.add_argument(
    "--instance",
    type=int,
    required=True,
    help="Instance id is need to correctly read and write to the "
    + "checkpoint in a multi-node simulation.",
)
parser.add_argument(
    "--memory-allocation-policy",
    type=str,
    required=True,
    help="The memory allocation policy can be local, interleaved, or remote.",
)
parser.add_argument(
    "--ckpts-dir",
    type=str,
    default="",
    required=True,
    help="Put a path to restore a checkpoint",
)
args = parser.parse_args()

remote_memory_range = AddrRange(stream_remote_memory_address_ranges[args.instance][0]*1024*1024*1024,
                                stream_remote_memory_address_ranges[args.instance][1]*1024*1024*1024)

requires(isa_required=ISA.ARM)

board = ArmComposableMemoryBoard(
    use_sst=True,
    remote_memory_address_range=remote_memory_range,
)

cmd = stream_run_commands[args.memory_allocation_policy]

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

# workload = obtain_resource("stream-workload-" + args.memory_allocation_policy)

ckpt_path = (
    f"{args.ckpts_dir}/{args.memory_allocation_policy}/"
    f"{args.instance}/ckpt_{args.instance}"
)

board.set_workload(workload)

exit_event = exit_event_generators.exit_generator

simulator = Simulator(
    board=board,
    on_exit_event={
        ExitEvent.EXIT: exit_event,
    },
    checkpoint_path=ckpt_path,
)

simulator._instantiate()