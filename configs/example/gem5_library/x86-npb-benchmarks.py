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
Script to run NAS parallel benchmarks with gem5. The script expects the
benchmark program to run. The input is in the format
<benchmark_prog>.<class>.x .The system is fixed with 2 CPU cores, MESI
Two Level system cache and 3 GB DDR4 memory. It uses the x86 board.

This script will count the total number of instructions executed
in the ROI. It also tracks how much wallclock and simulated time.

Usage:
------
    
```
scons build/X86_MESI_Two_Level/gem5.opt
./build/X86_MESI_Two_Level/gem5.opt \
    configs/example/gem5_library/x86-npb-benchmarks.py \
    <benchmark>
```
"""

import argparse
import time
import m5
from m5.objects import Root

from gem5.utils.requires import requires
from gem5.components.boards.x86_board import X86Board
from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.processors.simple_switchable_processor import(
    SimpleSwitchableProcessor,
)
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.coherence_protocol import CoherenceProtocol
from gem5.resources.resource import Resource

from m5.stats.gem5stats import get_simstat

import os, json

requires(
    isa_required = ISA.X86,
    coherence_protocol_required=CoherenceProtocol.MESI_TWO_LEVEL,
    kvm_required=True,
)

# Following are the list of benchmark programs for npb. Sizes are merged
# within the benchmark names (as class A, B, C, D or F)

benchmark_choices = ['bt.A.x', 'cg.A.x', 'ep.A.x', 'ft.A.x',
                     'is.A.x', 'lu.A.x', 'mg.A.x', 'sp.A.x',
                     'bt.B.x', 'cg.B.x', 'ep.B.x', 'ft.B.x',
                     'is.B.x', 'lu.B.x', 'mg.B.x', 'sp.B.x',
                     'bt.C.x', 'cg.C.x', 'ep.C.x', 'ft.C.x',
                     'is.C.x', 'lu.C.x', 'mg.C.x', 'sp.C.x',
                     'bt.D.x', 'cg.D.x', 'ep.D.x', 'ft.D.x',
                     'is.D.x', 'lu.D.x', 'mg.D.x', 'sp.D.x',
                     'bt.F.x', 'cg.F.x', 'ep.F.x', 'ft.F.x',
                     'is.F.x', 'lu.F.x', 'mg.F.x', 'sp.F.x']

# Setting up all the fixed system parameters here
# Caches: MESI Two Level Cache Hierarchy


parser = argparse.ArgumentParser(
    description="An example configuration script to run the npb benchmarks."
)

# The only positional argument accepted is the benchmark name in this script.
 
parser.add_argument(
    "benchmark",
    type = str,
    help = "Input the benchmark program to execute.",
    choices = benchmark_choices,
)

args = parser.parse_args()

from gem5.components.cachehierarchies.ruby.\
    mesi_two_level_cache_hierarchy import(
    MESITwoLevelCacheHierarchy,
)

cache_hierarchy = MESITwoLevelCacheHierarchy(
    l1d_size = "32kB",
    l1d_assoc = 8,
    l1i_size="32kB",
    l1i_assoc=8,
    l2_size="256kB",
    l2_assoc=16,
    num_l2_banks=2,
)
# Memory: Single Channel DDR4 2400 DRAM device.
# The X86 board only supports 3 GB of main memory.
# TODO: Need to replace single channel memory with MultiChannelMemory

memory = SingleChannelDDR4_2400(size = "3GB")

# Here we setup the processor. This is a special switchable processor in which
# a starting core type and a switch core type must be specified. Once a
# configuration is instantiated a user may call `processor.switch()` to switch
# from the starting core types to the switch core types. In this simulation
# we start with KVM cores to simulate the OS boot, then switch to the Timing
# cores for the command we wish to run after boot.

processor = SimpleSwitchableProcessor(
    starting_core_type=CPUTypes.KVM,
    switch_core_type=CPUTypes.TIMING,
    num_cores=2,
)

# Here we setup the board. The X86Board allows for Full-System X86 simulations.
board = X86Board(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

board.connect_things()

# Here we set the FS workload, i.e., npb benchmark program
command = "/home/gem5/NPB3.3-OMP/bin/{} \n".format(args.benchmark) + ";" \
    + "sleep 5;" \
    + "m5 exit;"

board.set_workload(
    # The x86 linux kernel will be automatically downloaded to the
    # `tests/gem5/resources` directory if not already present.
    # npb benchamarks was tested with kernel version 4.19.83
    kernel=Resource(
        "x86-linux-kernel-4.19.83",
        override=True,
    ),
    # The x86-npb image will be automatically downloaded to the
    # `tests/gem5/resources` directory if not already present.
    disk_image=Resource(
        "x86-npb",
        override=True,
    ),
    command=command,
)

root = Root(full_system = True, system = board)
root.sim_quantum = int(1e9)  # sim_quantum must be st if KVM cores are used.

m5.instantiate()

# We maintain the wall clock time.

globalStart = time.time()

print("Running the simulation")
print("Using KVM cpu")

exit_event = m5.simulate()

if exit_event.getCause() == "m5_exit instruction encountered":
    
    # We have completed booting the OS

    print("Done booting Linux")
    print("Resetting stats at the start of ROI!")
    m5.stats.reset()
    start_tick = m5.curTick()

    # We switch to timing cpu for detailed simulation.

    processor.switch()
else:
    print("Unexpected termination of simulation !")
    exit(-1)

# Simulate the ROI
exit_event = m5.simulate()

# Reached the end of ROI
# Finished executing the benchmark
# We dump the stats here

print("Dump stats at the end of the ROI!")
m5.stats.dump()
end_tick = m5.curTick()

# We dump the stats to a json file to get the simInsts in the ROI
# We then output simInsts in the final print statement.

gem5stats = get_simstat(root)

# Stats are printed into "stats.json" file in the output directory
with open(os.path.join(m5.options.outdir, "stats.json"), "w") as json_out:
    gem5stats.dump(json_out, indent = 2)

# TODO: Need to write this in a proper manner.
# TODO: Also, need to verify this.
# We iterate over the json file to get the number of committed instructions
# by the timing cores (2, 3). We sum and print them at the end.

roi_insts = float(\
    json.load(open(os.path.join\
    (m5.options.outdir, "stats.json",)))\
    ["system"]["processor"]["cores2"]["core"]["exec_context.thread_0"]\
    ["numInsts"]["value"]) + float(\
    json.load(open(os.path.join\
    (m5.options.outdir, "stats.json",)))\
    ["system"]["processor"]["cores3"]["core"]["exec_context.thread_0"]\
    ["numInsts"]["value"]\
)

# Simulation is over at this point.

print("Done with the simulation")
print()
print("Performance statistics:")

print("Simulated time in ROI: %.2fs" % ((end_tick-start_tick)/1e12))
print("Instructions executed in ROI: %d" % ((roi_insts)))
print("Ran a total of", m5.curTick()/1e12, "simulated seconds")
print("Total wallclock time: %.2fs, %.2f min" % \
            (time.time()-globalStart, (time.time()-globalStart)/60))
exit ()