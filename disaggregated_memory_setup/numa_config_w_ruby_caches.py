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
from m5.objects import Root

from gem5.utils.requires import requires
from riscv_dm_board import RiscvDMBoard
from dm_caches import ClassicPL1PL2DMCache
from mi_example_dm_caches import MIExampleDMCache
from gem5.components.memory import DualChannelDDR4_2400
from gem5.components.memory.multi_channel import *
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.simulate.simulator import Simulator
from gem5.resources.workload import Workload
from gem5.resources.workload import *
from gem5.resources.resource import *

# This runs a check to ensure the gem5 binary is compiled for RISCV.

requires(isa_required=ISA.RISCV)

# With RISCV, we use simple caches.
# from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import (
#     PrivateL1PrivateL2CacheHierarchy,
# )
##
# from gem5.components.cachehierarchies.classic.no_cache import NoCache

# # Here we setup the parameters of the l1 and l2 caches.
# cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
#     l1d_size="16kB", l1i_size="16kB", l2_size="256kB"
# )
##
# from gem5.components.cachehierarchies.ruby.mi_example_cache_hierarchy import (
#     MIExampleCacheHierarchy
# )

# Here we setup the parameters of the l1 and l2 caches.
# cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
#     l1d_size="16kB", l1i_size="16kB", l2_size="256kB"
# )
# Here we setup the parameters of the l1 and l2 caches.

# cache_hierarchy = ClassicPL1PL2DMCache(
#     l1d_size="16kB", l1i_size="16kB", l2_size="256kB"
# )

cache_hierarchy = MIExampleDMCache(size="256kB", assoc=8)

# cache_hierarchy = MIExampleCacheHierarchy(
#     size="16kB", assoc="8"
# )

# cache_hierarchy = NoCache()

# Memory: Dual Channel DDR4 2400 DRAM device.

# local_memory = DualChannelDDR4_2400(size="512MB")
local_memory = DualChannelDDR4_2400(size="512MB")
remote_memory = DualChannelDDR4_2400(size="2GB")

# remote_memory = DualChannelHBM_1000(size="4GB")

# Here we setup the processor. We use a simple processor.
processor = SimpleProcessor(
    cpu_type=CPUTypes.TIMING, isa=ISA.RISCV, num_cores=4
)

# Here we setup the board. The RiscvBoard allows for Full-System RISCV
# simulations.
board = RiscvDMBoard(
    clk_freq="3GHz",
    processor=processor,
    local_memory=local_memory,
    remote_memory=remote_memory,
    cache_hierarchy=cache_hierarchy,
)

workload = CustomWorkload(
    function="set_kernel_disk_workload",
    parameters={
        # "disk_image" : DiskImageResource(os.path.join(os.getcwd(),
        #                                 "arm64-ubuntu-numa"),
        #                                 root_partition = "1"),
        "disk_image": CustomDiskImageResource(
            local_path=os.path.join(
                os.getcwd(),
                # "/home/kaustavg/ubuntu-numa.img"),
                # "/home/kaustavg/ubuntu-numa-bench.img"),
                "/home/kaustavg/disk-images/rv64gc-hpc-2204.img",
            ),
            # local_path = "/home/kaustavg/kernel/gem5-resources/src/riscv-ubuntu/disk-image/base/ubuntu-ML.img",
            # "/scr/kaustavg/simulators-at-scratch/DArchR/WorkingDir/gem5-simpoint/gem5x/gem5/riscv-ubuntu-20221118.img"),
            disk_root_partition="1",
        ),
        # root_partition = "1"),
        "kernel": CustomResource(
            os.path.join(
                os.getcwd(),
                # "x86-linux-kernel-5.4.49"))
                # "/scr/kaustavg/simulators-at-scratch/DArchR/WorkingDir/gem5-simpoint/gem5x/gem5/riscv-bootloader-vmlinux-5.10"))
                "/scr/kaustavg/simulators-at-scratch/DArchR/WorkingDir/gem5-simpoint/gem5x/gem5/bbl",
            )
        ),
        # "bootloader": CustomResource(os.path.join(os.getcwd(),
        #                         "vmlinux-5.4.49-NUMA.riscv"))
    },
)

# Here we a full system workload: "riscv-ubuntu-20.04-boot" which boots
# Ubuntu 20.04. Once the system successfully boots it encounters an `m5_exit`
# instruction which stops the simulation. When the simulation has ended you may
# inspect `m5out/system.pc.com_1.device` to see the stdout.
# board.set_workload(Workload("riscv-ubuntu-20.04-boot"))
board.set_workload(workload)
simulator = Simulator(board=board)
simulator.run()
simulator.run()
