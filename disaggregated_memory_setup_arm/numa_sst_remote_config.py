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

import m5
from m5.objects import Root, OutgoingRequestBridge, AddrRange

from gem5.utils.requires import requires
from arm_dm_sst_board import ArmDMSSTBoard
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
parser.add_argument(
    "--remote-memory-range",
    type=str,
    required=True,
    help="Remote memory range",
)
args = parser.parse_args()
command = args.command
remote_memory_range = list(map(int, args.remote_memory_range.split(",")))
remote_memory_range = AddrRange(remote_memory_range[0], remote_memory_range[1])

requires(isa_required=ISA.ARM)

cache_hierarchy = ClassicPL1PL2DMCache(
    l1d_size="16kB", l1i_size="16kB", l2_size="256kB"
)

local_memory = DualChannelDDR4_2400(size="256MiB")

cpu_type = {"atomic": CPUTypes.ATOMIC, "timing": CPUTypes.TIMING}[
    args.cpu_type
]
processor = SimpleProcessor(cpu_type=cpu_type, isa=ISA.ARM, num_cores=1)


class MyBoard(ArmDMSSTBoard):
    @overrides(ArmDMSSTBoard)
    def _pre_instantiate(self):
        super()._pre_instantiate()
        self.remote_memory_outgoing_bridge = OutgoingRequestBridge()
        self.remote_memory_outgoing_bridge.physical_address_ranges = [
            self.get_remote_memory_addr_range()
        ]
        self.remote_memory_outgoing_bridge.port = (
            self.cache_hierarchy.membus.mem_side_ports
        )

    @overrides(ArmDMSSTBoard)
    def get_default_kernel_args(self):
        return [
            "root=/dev/vda1",
            "init=/root/gem5-init.sh",
            "console=ttyAMA0",
            "lpj=19988480",
            "norandmaps",
            "rw",
            # f"mem={self.get_memory().get_size()}",
        ]


board = MyBoard(
    clk_freq="4GHz",
    processor=processor,
    memory=local_memory,
    remote_memory_range=remote_memory_range,
    cache_hierarchy=cache_hierarchy,
)

board.set_kernel_disk_workload(
    kernel=CustomResource("/scr/hn/linux-5.15.114/vmlinux"),
    bootloader=Resource("arm64-bootloader-foundation"),
    disk_image=DiskImageResource(
        "/projects/gem5/hn/DISK_IMAGES/arm64sve-hpc-2204-20230526-numa.img"
    ),
    readfile_contents=f"mount -t sysfs - /sys; mount -t proc - /proc; {command};",
)

simulator = Simulator(board=board)
simulator._instantiate()
