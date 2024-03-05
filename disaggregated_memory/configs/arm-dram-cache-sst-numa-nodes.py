# Copyright (c) 2023 The Regents of the University of California
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

import os
import sys
import argparse

# all the source files are one directory above.
sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
)

import m5
from m5.objects import Root, AddrRange

from boards.arm_dram_cache_sst_board import ArmSstDMBoardDRAMCache
from cachehierarchies.dm_caches_sst import (
    ClassicPrivateL1PrivateL2SstDMCache
)
from memories.dram_cache import TDRAMCache
from memories.remote_memory_outgoing_bridge import RemoteMemoryOutgoingBridge
from gem5.utils.requires import requires
from gem5.components.memory import SingleChannelDDR4_2400, DualChannelDDR4_2400
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.resources.workload import *
from gem5.resources.resource import *

# SST passes a couple of arguments for this system to simulate.
parser = argparse.ArgumentParser()
parser.add_argument("--command", type=str, help="Command run by guest")
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
    "--local-memory-size",
    type=str,
    required=True,
    help="Local memory size",
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
args = parser.parse_args()
cpu_type = {
    "o3": CPUTypes.O3,
    "atomic": CPUTypes.ATOMIC,
    "timing": CPUTypes.TIMING,
    "kvm": CPUTypes.KVM}[args.cpu_type]

remote_memory_range = list(map(int, args.remote_memory_addr_range.split(",")))
remote_memory_range = AddrRange(remote_memory_range[0], remote_memory_range[1])

# This runs a check to ensure the gem5 binary is compiled for RISCV.
requires(isa_required=ISA.ARM)
# Here we setup the parameters of the l1 and l2 caches.
cache_hierarchy = ClassicPrivateL1PrivateL2SstDMCache(
    l1d_size="32KiB", l1i_size="32KiB", l2_size="256KiB"
)
# Memory: Dual Channel DDR4 2400 DRAM device.

local_memory = SingleChannelDDR4_2400(size=args.local_memory_size)

dram_cache = TDRAMCache(cache_size = "128MiB")

# Either suppy the size of the remote memory or the address range of the
# remote memory. Since this is inside the external memory, it does not matter
# what type of memory is being simulated. This can either be initialized with
# a size or a memory address range, which is mroe flexible. Adding remote
# memory latency automatically adds a non-coherent crossbar to simulate latenyc
remote_memory_outgoing_bridge = RemoteMemoryOutgoingBridge(
    addr_range=remote_memory_range,
    link_latency=args.remote_memory_latency
)

# Here we setup the processor. We use a simple processor.
processor = SimpleProcessor(
        cpu_type=cpu_type, isa=ISA.ARM, num_cores=4
)

# Here we setup the board which allows us to do Full-System ARM simulations.
board = ArmSstDMBoardDRAMCache(
    clk_freq=args.cpu_clock_rate,
    processor=processor,
    local_memory=local_memory,
    dram_cache=dram_cache,
    remote_memory_outgoing_bridge=remote_memory_outgoing_bridge,
    cache_hierarchy=cache_hierarchy,
)

cmd = [
    "mount -t sysfs - /sys;",
    "mount -t proc - /proc;",
    "numastat;",
    "numactl --membind=0 -- " +
    "/home/ubuntu/simple-vectorizable-microbenchmarks/stream-annotated/" +
    "stream.hw.m5 1000000;",
    "numastat;",
    "numactl --interleave=0,1 -- " +
    "/home/ubuntu/simple-vectorizable-microbenchmarks/stream-annotated/" +
    "stream.hw.m5 1000000;",
    "numastat;",
    "numactl --membind=1 -- " +
    "/home/ubuntu/simple-vectorizable-microbenchmarks/stream-annotated/" +
    "stream.hw.m5 1000000;",
    "numastat;",
    "m5 exit;",
]

workload = CustomWorkload(
    function="set_kernel_disk_workload",
    parameters={
    "kernel" : CustomResource("/home/kaustavg/vmlinux-5.4.49-NUMA.arm64"),
    "bootloader" : CustomResource("/home/babaie/projects/disaggregated-cxl/1/gem5/kernel/arm64-bootloader-foundation"),
    "disk_image" : DiskImageResource(
        "/home/kaustavg/disk-images/arm/arm64sve-hpc-2204-20230526-numa.img",
        root_partition="1",
    ),
    "readfile_contents" : " ".join(cmd)
    },
)
# This disk image needs to have NUMA tools installed.
board.set_workload(workload)
# This script will boot two numa nodes in a full system simulation where the
# gem5 node will be sending instructions to the SST node. the simulation will
# after displaying numastat information on the terminal, whjic can be viewed
# from board.terminal.
board._pre_instantiate()
root = Root(full_system=True, board=board)
board._post_instantiate()
m5.instantiate()