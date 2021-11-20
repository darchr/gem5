# Copyright (c) 2021 The Regents of the University of California
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
This example runs a simple linux boot.
Characteristics
---------------
* Runs exclusively on the RISC-V ISA with the classic caches
* Assumes that the kernel is compiled into the bootloader
* Automatically generates the DTB file
"""
import m5
from m5.objects import Root
import sys
import os
# This is a lame hack to get the imports working correctly.
# TODO: This needs fixed.
sys.path.append(
    os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        os.pardir,
        os.pardir,
        os.pardir,
    )
)
from src.python.gem5.runtime import get_runtime_isa
from src.python.gem5.components.boards.experimental.lupv_board import LupvBoard
from src.python.gem5.components.memory.single_channel import (
    SingleChannelDDR3_1600
)
from src.python.gem5.components.processors.simple_processor import (
    SimpleProcessor
)
from src.python.gem5.components.processors.cpu_types import CPUTypes
from src.python.gem5.isas import ISA
import os
import argparse
import subprocess
import gzip
import shutil
# Run a check to ensure the right version of gem5 is being used.
if get_runtime_isa() != ISA.RISCV:
    raise EnvironmentError(
        "The run_lupv.py should be run with RISCV ISA."
    )
from src.python.gem5.components.cachehierarchies.classic. \
    private_l1_private_l2_cache_hierarchy import (
    PrivateL1PrivateL2CacheHierarchy,
)

def parse_options():
    parser = argparse.ArgumentParser(description='Runs Linux fs test with'
                'RISCV.')
    parser.add_argument("bbl", help='Path to the bbl (berkeley bootloader)'
                                        'binary with kernel payload')
    parser.add_argument("disk", help="Path to the disk image to boot")
    parser.add_argument("cpu_type", help="The type of CPU in the system")
    parser.add_argument("num_cpus", type=int,
                        help="The number of CPU in the system")

    return parser.parse_args()

args = parse_options()

cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
    l1d_size='32KiB',
    l1i_size='32KiB',
    l2_size='512KiB'
)

# Setup the system memory.
memory = SingleChannelDDR3_1600(size='128MB')
# Setup a single core Processor.
if args.cpu_type == "atomic":
    processor = SimpleProcessor(cpu_type=CPUTypes.ATOMIC,
                                num_cores=args.num_cpus)
elif args.cpu_type == "simple":
    processor = SimpleProcessor(cpu_type=CPUTypes.TIMING,
                                num_cores=args.num_cpus)

# Setup the board.
board = LupvBoard(
    clk_freq="1GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)
board.connect_things()
# Set the Full System workload.
board.set_workload(disk_image=args.disk, bootloader=args.bbl)
# Begin running of the simulation. This will exit once the Linux system boot
# is complete.
print("Running with ISA: " + get_runtime_isa().name)
print()
root = Root(full_system=True, system=board)
m5.instantiate()
print("Beginning simulation!")

exit_event = m5.simulate()

print(
    "Exiting @ tick {} because {}.".format(m5.curTick(), exit_event.getCause())
)
