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
This script shows an example of running a full system RISCV Ubuntu boot
simulation using the gem5 library. This simulation boots Ubuntu 20.04 using
1 TIMING CPU cores and executes `numastat`. The simulation ends when the
startup is completed successfully.
"""

import os
import sys

# all the source files are one directory above.
sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
)

import m5
from m5.objects import Root

from boards.riscv_gem5_board import RiscvGem5DMBoard
from cachehierarchies.dm_caches import ClassicPL1PL2DMCache
from gem5.utils.requires import requires
from gem5.components.memory import DualChannelDDR4_2400
from memories.remote_memory import RemoteChanneledMemory
from gem5.components.memory.dram_interfaces.ddr4 import DDR4_2400_8x8
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.simulate.simulator import Simulator
from gem5.resources.workload import Workload
from gem5.resources.workload import *
from gem5.resources.resource import *

# This runs a check to ensure the gem5 binary is compiled for RISCV.

requires(isa_required=ISA.RISCV)

# defining a new type of memory with latency added.
def RemoteDualChannelDDR4_2400(
    size: Optional[str] = None, remote_offset_latency=300
) -> AbstractMemorySystem:
    """
    A dual channel memory system using DDR4_2400_8x8 based DIMM
    """
    return RemoteChanneledMemory(
        DDR4_2400_8x8,
        2,
        64,
        size=size,
        remote_offset_latency=remote_offset_latency,
    )
# Here we setup the parameters of the l1 and l2 caches.
cache_hierarchy = ClassicPrivateL1PrivateL2SstDMCache(
    l1d_size="32KiB", l1i_size="32KiB", l2_size="1MB"
)
# Memory: Dual Channel DDR4 2400 DRAM device.
local_memory = DualChannelDDR4_2400(size="2GiB")
# The remote meomry can either be a simple Memory Interface, which is from a
# different memory arange or it can be a Remote Memory Range, which has an
# inherent delay while performing reads and writes into that memory. For simple
# memory, use any MemInterfaces available in gem5 standard library. For remtoe
# memory, please refer to the `RemoteDualChannelDDR4_2400` method in this
# config script to extend any existing MemInterface class and add latency value
# to that memory.
remote_memory = RemoteDualChannelDDR4_2400(
    size="1GB", remote_offset_latency=750
)
# Here we setup the processor. We use a simple processor.
processor = SimpleProcessor(
    cpu_type=CPUTypes.ATOMIC, isa=ISA.RISCV, num_cores=1
)
# Here we setup the board which allows us to do Full-System RISCV simulations.
board = RiscvGem5DMBoard(
    clk_freq="3GHz",
    processor=processor,
    local_memory=local_memory,
    remote_memory=remote_memory,
    cache_hierarchy=cache_hierarchy,
)
cmd = [
    "mount -t sysfs - /sys;",
    "mount -t proc - /proc;",
    "numastat;"
    "m5 exit;"
]
workload = CustomWorkload(
    function="set_kernel_disk_workload",
    parameters={
        "disk_image": DiskImageResource(
            local_path="/home/kaustavg/disk-images/rv64gc-hpc-2204.img",
            root_partition="1",
        ),
        "kernel": CustomResource(
            "/scr/kaustavg/simulators-at-scratch/DArchR/WorkingDir/SST13/kernels/gem5-resources/src/riscv-boot-exit-nodisk/riscv-pk/build/bbl"
        ),
        "readfile_contents": " ".join(cmd),
    },
)
# This script will boot two numa nodes in a full system simulation where the
# gem5 node will be sending instructions to the SST node. the simulation will
# after displaying numastat information on the terminal, whjic can be viewed
# from board.terminal.
board.set_workload(workload)
simulator = Simulator(board=board)
simulator.run()
