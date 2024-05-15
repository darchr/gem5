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

from ..boards.arm_main_board import ArmComposableMemoryBoard
from common import cmd_dic

import m5
from m5.objects import (
    AddrRange,
    Root,
)
from gem5.isas import ISA
from gem5.resources.resource import *
from gem5.resources.workload import *
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

# Parameters related to checkpoints.
parser.add_argument(
    "--ckpt-file",
    type=str,
    default="",
    required=False,
    help="optionally put a path to restore a checkpoint",
)

args = parser.parse_args()

use_sst = {"True": True, "False": False}[args.is_composable]

remote_memory_range = list(map(int, args.remote_memory_addr_range.split(",")))
remote_memory_range = AddrRange(remote_memory_range[0], remote_memory_range[1])

# This runs a check to ensure the gem5 binary is compiled for ARM.
requires(isa_required=ISA.ARM)

# Here we setup the board which allows us to do Full-System ARM simulations.
board = ArmComposableMemoryBoard(
    use_sst=use_sst,
    remote_memory_address_range=remote_memory_range,
)

cmd = cmd_dic["remote"]

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

# workload = obtain_resource("stream-workload")
# workload.set_parameter(parameter="readfile_contents", value=" ".join(cmd))

ckpt_to_read_write = ""
if args.is_composable == "False":
    ckpt_to_read_write = (
        m5.options.outdir
        + "/ckpt_"
        + str(args.instance)
    )
    # inform the user where the checkpoint will be saved
    print("Checkpoint will be saved in " + ckpt_to_read_write)
else:
    assert args.ckpt_file != ""
    ckpt_to_read_write = args.ckpt_file

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
if use_sst == False:
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
