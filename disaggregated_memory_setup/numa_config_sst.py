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
from m5.objects import Root, NoncoherentXBar

from gem5.utils.requires import requires
from riscv_dm_board import RiscvDMBoard
from riscv_sst_board import RiscvSstBoard
from dm_caches import ClassicPL1PL2DMCache
from gem5.components.memory import SingleChannelDDR4_2400, DualChannelDDR4_2400
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

# Here we setup the parameters of the l1 and l2 caches.

cache_hierarchy = ClassicPL1PL2DMCache(
    l1d_size="4MB", l1i_size="4MB", l2_size="32MB"
)

# Memory: Dual Channel DDR4 2400 DRAM device.

local_memory = SingleChannelDDR4_2400(size="2GiB")
# This has to be an argument coming from SST's side.
remote_memory_size = "2GiB"
# remote_memory = DualChannelDDR4_2400(size="4GB")

# remote_memory = DualChannelHBM_1000(size="4GB")

# Here we setup the processor. We use a simple processor.
processor = SimpleProcessor(
    cpu_type=CPUTypes.TIMING, isa=ISA.RISCV, num_cores=1
)

# Here we setup the board. The RiscvBoard allows for Full-System RISCV
# simulations.
board = RiscvSstBoard(
    clk_freq="3GHz",
    processor=processor,
    local_memory=local_memory,
    remote_memory_size=remote_memory_size,
    cache_hierarchy=cache_hierarchy,
)

workload = CustomWorkload(
    function="set_kernel_disk_workload",
    parameters={
        "disk_image": DiskImageResource(
            # CustomDiskImageResource(
            local_path=os.path.join(
                os.getcwd(), "/home/kaustavg/ubuntu-numa.img"
            ),
            root_partition="1",
        ),
        "kernel": CustomResource(
            os.path.join(os.getcwd(), "/home/kaustavg/bbl")
        ),
    },
)

# board.cache_hierarchy.membus = NoncoherentXBar(
#         frontend_latency=0,
#         forward_latency=0,
#         response_latency=0,
#         header_latency=0,
#         width=64,
#     )

# Here we a full system workload: "riscv-ubuntu-20.04-boot" which boots
# Ubuntu 20.04. Once the system successfully boots it encounters an `m5_exit`
# instruction which stops the simulation. When the simulation has ended you may
# inspect `m5out/system.pc.com_1.device` to see the stdout.
# board.set_workload(Workload("riscv-ubuntu-20.04-boot"))

# This disk image has NUMA tools installed.
board.set_workload(workload)
board._pre_instantiate()
root = Root(full_system=True, system=board)

# simulator = Simulator(board=board)
# simulator.run()
# simulator.run()
