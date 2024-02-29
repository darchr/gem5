# Copyright (c) 2021-2024 The Regents of the University of California
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
from m5.objects import Root

from gem5.components.boards.riscv_board import RiscvBoard
from gem5.components.memory import DualChannelDDR4_2400
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.isas import ISA
from gem5.utils.override import overrides
from gem5.resources.resource import obtain_resource, BootloaderResource, KernelResource, DiskImageResource
from gem5.simulate.simulator import Simulator
from gem5.utils.requires import requires

# This runs a check to ensure the gem5 binary is compiled for RISCV.

requires(isa_required=ISA.RISCV)

# With RISCV, we use simple caches.
from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import (
    PrivateL1PrivateL2CacheHierarchy,
)

cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
    l1d_size="16kB", l1i_size="16kB", l2_size="256kB"
)

memory = DualChannelDDR4_2400(size="3GB")

processor = SimpleProcessor(
    cpu_type=CPUTypes.TIMING, isa=ISA.RISCV, num_cores=1
)

class EarlyKernelRiscvBoard(RiscvBoard):
    def __init__(a,b,c,d):
        super().__init__(a,b,c,d)
    @overrides(RiscvBoard)
    def get_default_kernel_args(self):
        return [
            "earlycon=uart8250,0x10000000",
            "console=uart8250,0x10000000",
            # "console=ttyS0",
            "lpj=7999923",
            "root=/dev/vda1",
            "rw",
            "init=/root/gem5-init.sh"
        ]

board = RiscvBoard(
    clk_freq="4GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

command = "m5 exit;"

board.set_kernel_disk_workload(
    bootloader=BootloaderResource("/workdir/resources/riscv-bootloader-opensbi-1.3.1-20231129"),
    kernel=KernelResource("/workdir/resources/linux-6.5.5"),
    #disk_image=DiskImageResource("/workdir/hpc-disk-image/rv64gc-hpc-2204.img", root_partition="1"),
    disk_image=DiskImageResource("/workdir/resources/riscv-ubuntu-20221118.img", root_partition="1"),
    readfile_contents=command,
)

simulator = Simulator(board=board)
simulator._instantiate()
