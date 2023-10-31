# Copyright (c) 2021 The Regents of the University of California.
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
Script to run GAPBS benchmarks with gem5. The script expects the
benchmark program and the simulation size to run. The input is in the format
<benchmark_prog> <size> <synthetic>
The system is fixed with 2 CPU cores, MESI Two Level system cache and 3 GB
DDR4 memory. It uses the x86 board.

This script will count the total number of instructions executed
in the ROI. It also tracks how much wallclock and simulated time.

Usage:
------

```
scons build/X86/gem5.opt
./build/X86/gem5.opt \
    configs/example/gem5_library/x86-gabps-benchmarks.py \
    --benchmark <benchmark_name> \
    --synthetic <synthetic> \
    --size <simulation_size/graph_name>
```
"""

import argparse
import time
import sys

import m5
from m5.objects import Root

from gem5.utils.requires import requires
from gem5.components.boards.x86_board import X86Board
from gem5.components.memory import DualChannelDDR4_2400, SingleChannelDDR4_2400

# from gem5.components.processors.simple_switchable_processor import (
#     SimpleSwitchableProcessor,
# )
from x86_dm_board import X86DMBoard
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.coherence_protocol import CoherenceProtocol
from gem5.resources.resource import obtain_resource
from gem5.simulate.simulator import Simulator
from gem5.simulate.exit_event import ExitEvent
from dm_caches import ClassicPL1PL2DMCache

requires(
    isa_required=ISA.X86,
    kvm_required=True,
)

# Following are the list of benchmark programs for gapbs


cache_hierarchy = ClassicPL1PL2DMCache(
    l1d_size="2MB", l1i_size="2MB", l2_size="4MB"
)
# Memory: Dual Channel DDR4 2400 DRAM device.
# The X86 board only supports 3 GB of main memory.

local_memory = SingleChannelDDR4_2400(size="1GB")
remote_mem_size = "1GiB"
# remote_memory = SingleChannelDDR4_2400(size="3GB")

# Here we setup the processor. This is a special switchable processor in which
# a starting core type and a switch core type must be specified. Once a
# configuration is instantiated a user may call `processor.switch()` to switch
# from the starting core types to the switch core types. In this simulation
# we start with KVM cores to simulate the OS boot, then switch to the Timing
# cores for the command we wish to run after boot.

processor = SimpleProcessor(cpu_type=CPUTypes.TIMING, isa=ISA.X86, num_cores=1)

# Here we setup the board. The X86Board allows for Full-System X86 simulations

board = X86DMBoard(
    clk_freq="3GHz",
    processor=processor,
    memory=local_memory,
    remote_memory_size=remote_mem_size,
    # remote_memory=remote_memory,
    cache_hierarchy=cache_hierarchy,
)

# Here we set the FS workload, i.e., gapbs benchmark program
# After simulation has ended you may inspect
# `m5out/system.pc.com_1.device` to the stdout, if any.

board.set_kernel_disk_workload(
    # The x86 linux kernel will be automatically downloaded to the
    # `~/.cache/gem5` directory if not already present.
    # gapbs benchamarks was tested with kernel version 4.19.83
    kernel=obtain_resource("x86-linux-kernel-4.19.83"),
    # The x86-gapbs image will be automatically downloaded to the
    # `~/.cache/gem5` directory if not already present.
    disk_image=obtain_resource("x86-ubuntu-18.04-img"),
    # readfile_contents=command,
)
board._pre_instantiate()
root = Root(full_system=True, system=board)
# simulator = Simulator(board=board)
# simulator.run()
