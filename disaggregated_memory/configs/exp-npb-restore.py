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
from common import npb_mem_size, npb_benchmarks, npb_classes, npb_benchmarks_index, npb_D_remote_mem_size

import m5
from m5.objects import AddrRange
from gem5.isas import ISA
from gem5.resources.resource import *
from gem5.resources.workload import *
from gem5.simulate.exit_event import ExitEvent
from gem5.simulate.simulator import Simulator
from gem5.utils.requires import requires

parser = argparse.ArgumentParser()

parser.add_argument(
    "--memory-allocation-policy",
    type=str,
    required=True,
    help="The memory allocation policy can be all-local, or numa-local-preferred .",
)
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
    "--ckpts-dir",
    type=str,
    default="",
    required=True,
    help="Put a path to restore a checkpoint",
)
args = parser.parse_args()

benchmark = f"{args.benchmark}.{args.size}.x"
workload_size = npb_mem_size[benchmark]
command_list = []
npb_command = "/home/ubuntu/arm-bench/npb-hooks/NPB3.4.2/NPB3.4-OMP/bin/" + benchmark

if args.memory_allocation_policy == "all-local":
    # the first 2GiB = OS
    # the next 85 GiB = local memory (the max size of the workloads)
    # the next 1GiB = remote memory
    local_memory_size_GiB = str(85) + "GiB"
    index = npb_benchmarks_index[args.benchmark]
    # assigning 1GiB of remote memory
    remote_memory_range = AddrRange((2+85+index-1)*1024*1024*1024,(2+85+index)*1024*1024*1024)
    command_list = [
        f"echo 'starting to run {benchmark}, {args.memory_allocation_policy}';",
        f"{npb_command};",
        "m5 --addr=0x10010000 exit;"
    ]
elif args.memory_allocation_policy == "numa-local-preferred":
    # the first 2GiB = OS
    # the next 8GiB = local memory
    # the next XXX GiB = remote memory with the size of workload beyond 8GiB
    local_memory_size_GiB = "8GiB"
    remote_memory_range = AddrRange(npb_D_remote_mem_size[args.benchmark][0]*1024*1024*1024,
                                    npb_D_remote_mem_size[args.benchmark][1]*1024*1024*1024)
    command_list = [
        "numastat;",
        f"echo 'starting to run {benchmark}, {args.memory_allocation_policy}';",
        f"numactl --preferred=0 -- {npb_command};",
        "numastat;",
        "m5 --addr=0x10010000 exit;"
    ]

requires(isa_required=ISA.ARM)

board = ArmComposableMemoryBoard(
    use_sst=True,
    remote_memory_address_range=remote_memory_range,
    local_memory_size=local_memory_size_GiB,
)

workload = CustomWorkload(
    function="set_kernel_disk_workload",
    parameters={
        "kernel": CustomResource("/home/babaie/.cache/gem5/vmlinux-5.4.49-NUMA.arm64"),
        "bootloader": CustomResource(
            "/home/babaie/.cache/gem5/arm64-bootloader"
        ),
        "disk_image": DiskImageResource(
            "/home/babaie/.cache/gem5/arm64-hpc-2204-numa-kvm.img-20240304",
            root_partition="1",
        ),
        "readfile_contents": " ".join(command),
    },
)

# workload = obtain_resource("stream-workload-" + args.memory_allocation_policy)
# print(workload.get_parameters())

ckpt_path = (
    f"{args.ckpts_dir}/{args.memory_allocation_policy}/{args.size}/{args.benchmark}/ckpt_{args.benchmark}.{args.size}"
)
print("Checkpoint will be read from: " + ckpt_path)

board.set_workload(workload)

# define on_exit_event
def handle_exit_event():
    for num_iterations in range(19):
        print(f"Done with iteration #{num_iterations}")
        m5.stats.dump()
        print(f"Dumped stats at the end of the iteration #{num_iterations}")
        m5.setMaxTick(m5.curTick() + 50_000_000_000) # simulate another 50 ms
        yield False  # Continue the simulation.
    print(f"Dump stats since all the iterations completed")
    m5.stats.dump()
    yield True  # Stop the simulation. We're done.

simulator = Simulator(
    board=board,
    on_exit_event={
        ExitEvent.MAX_TICK : handle_exit_event(),
    },
    checkpoint_path=ckpt_path,
)

simulator._instantiate()

m5.setMaxTick(m5.curTick() + 50_000_000_000)