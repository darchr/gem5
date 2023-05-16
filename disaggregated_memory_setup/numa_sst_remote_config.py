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
2 TIMING CPU cores. The simulation ends when the startup is completed
successfully.

Usage
-----

```
scons build/RISCV/gem5.opt
./build/RISCV/gem5.opt \
    configs/example/gem5_library/riscv-ubuntu-run.py
```
"""

import m5
from m5.objects import Root, OutgoingRequestBridge, AddrRange

from gem5.utils.requires import requires
from riscv_dm_sst_board import RiscvDMSSTBoard
from dm_caches import ClassicPL1PL2DMCache
from gem5.components.memory import DualChannelDDR4_2400
from gem5.components.memory.multi_channel import *
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.simulate.simulator import Simulator
from gem5.resources.workload import Workload
from gem5.resources.workload import *
from gem5.resources.resource import *
from gem5.utils.override import overrides
from gem5.components.cachehierarchies.classic.no_cache import NoCache
from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import (
    PrivateL1PrivateL2CacheHierarchy,
)
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--command", type=str, help="Command run by guest")
parser.add_argument(
    "--cpu-type",
    type=str,
    choices=["atomic", "timing"],
    default="atomic",
    help="CPU type",
)
args = parser.parse_args()
command = args.command

# This runs a check to ensure the gem5 binary is compiled for RISCV.

requires(isa_required=ISA.RISCV)

# Here we setup the parameters of the l1 and l2 caches.

cache_hierarchy = ClassicPL1PL2DMCache(
    l1d_size="16kB", l1i_size="16kB", l2_size="256kB"
)
# cache_hierarchy = NoCache()
# cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
#    l1d_size="16kB", l1i_size="16kB", l2_size="256kB"
# )

# Memory: Dual Channel DDR4 2400 DRAM device.

local_memory = DualChannelDDR4_2400(size="2GB")
oneGiB = 1 << 30
oneMiB = 1 << 20
offset = 1 << 32
remote_memory_addr_range = AddrRange(offset, size=32 * oneMiB)

# Here we setup the processor. We use a simple processor.
cpu_type = {"atomic": CPUTypes.ATOMIC, "timing": CPUTypes.TIMING}[
    args.cpu_type
]
processor = SimpleProcessor(cpu_type=cpu_type, isa=ISA.RISCV, num_cores=1)

# Here we setup the board. The RiscvBoard allows for Full-System RISCV
# simulations.


class MyBoard(RiscvDMSSTBoard):
    @overrides(RiscvDMSSTBoard)
    def _pre_instantiate(self):
        super()._pre_instantiate()
        self.remote_memory_outgoing_bridge = OutgoingRequestBridge()
        self.remote_memory_outgoing_bridge.physical_address_ranges = [
            remote_memory_addr_range
        ]
        self.remote_memory_outgoing_bridge.port = (
            self.cache_hierarchy.membus.mem_side_ports
        )
        # self.system_outgoing_bridge = (
        #    OutgoingRequestBridge()
        # )  # this should not do anything
        # self.system_outgoing_bridge.port = self.system_port

    @overrides(RiscvDMSSTBoard)
    def get_default_kernel_args(self):
        return [
            "earlyprintk=ttyS0",
            "console=ttyS0",
            "lpj=7999923",
            "root=/dev/vda1",
            "init=/root/gem5-init.sh",
            "rw",
        ]


board = MyBoard(
    clk_freq="4GHz",
    processor=processor,
    local_memory=local_memory,
    remote_memory_addr_range=remote_memory_addr_range,
    cache_hierarchy=cache_hierarchy,
)

board.set_kernel_disk_workload(
    kernel=CustomResource("/projects/gem5/numa/bbl"),
    # kernel=CustomResource("/home/hn/.cache/gem5/riscv-bootloader-vmlinux-5.10"),
    disk_image=DiskImageResource("/scr/hn/DISK_IMAGES/rv64gc-hpc-2204.img"),
    # disk_image=DiskImageResource("/scr/hn/DISK_IMAGES/test.img"),
    readfile_contents=f"{command}",
)

simulator = Simulator(board=board)
simulator._instantiate()
