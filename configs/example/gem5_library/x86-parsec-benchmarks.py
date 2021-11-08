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
Script to run PARSEC benchmarks with gem5.
The script expects a benchmark program name and the simulation
size. The system is fixed with 2 CPU cores, MESI Two Level system
cache and 3 GB DDR4 memory. It uses the x86 board.

This script will count the total number of instructions executed
in the ROI. It also tracks how much wallclock and simulated time.

Usage:
------

```
scons build/X86_MESI_Two_Level/gem5.opt
./build/X86_MESI_Two_Level/gem5.opt \
    configs/example/gem5_library/x86-parsec-benchmarks.py \
    <benchmark> <simulation size>
```
"""
import argparse
import time
import json

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

# We check for the required gem5 build.

requires(
    isa_required = ISA.X86,
    coherence_protocol_required=CoherenceProtocol.MESI_TWO_LEVEL,
    kvm_required=True,
)

# Following are the list of benchmark programs for parsec.

benchmark_choices = ["blackscholes", "bodytrack", "canneal", "dedup",
                     "facesim", "ferret", "fluidanimate", "freqmine",
                     "raytrace", "streamcluster", "swaptions", "vips", "x264"]

# Following are the input size.

size_choices=["simsmall", "simmedium", "simlarge"]

parser = argparse.ArgumentParser(
    description="An example configuration script to run the npb benchmarks."
)

# The arguments accepted are the benchmark name and the simulation size.

parser.add_argument(
    "benchmark",
    type = str,
    help = "Input the benchmark program to execute.",
    choices = benchmark_choices,
)

parser.add_argument(
    "size",
    type = str,
    help = "Sumulation size the benchmark program.",
    choices = size_choices,
)
args = parser.parse_args()

# Setting up all the fixed system parameters here
# Caches: MESI Two Level Cache Hierarchy

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
# We will replace single channel memory in the future.
# Right now, multichannel memories does not work properly
# with the x86 I/O hole.

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

# Here we setup the board. The X86Board allows for Full-System X86 simulations

board = X86Board(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,

    # We need to exit from the simulation at ROI begin and end annotations via
    # m5 calls. The system exits from guest on workbegin/workend.
    exit_on_work_items=True,
)

board.connect_things()

# Here we set the FS workload, i.e., parsec benchmark
# After simulation has ended you may inspect
# `m5out/system.pc.com_1.device` to the output, if any.

# After the system starts, we execute the benchmark program and wait till the
# ROI `workbegin` annotation is reached (m5_work_begin()). We start collecting
# the number of committed instructions till ROI ends (marked by `workend`).
# We then finish executing the rest of the benchmark. 

# Also, we sleep the system for some time so that the output is printed
# properly.


command = "cd /home/gem5/parsec-benchmark;".format(args.benchmark) \
    + "source env.sh;" \
    + "parsecmgmt -a run -p {} -c gcc-hooks -i {} \
        -n {};".format(args.benchmark, args.size, "2") \
    + "sleep 5;" \
    + "m5 exit;" \

board.set_workload(
    # The x86 linux kernel will be automatically downloaded to the
    # `~/.cache/gem5` directory if not already present.
    # SPEC CPU2006 benchamarks were tested with kernel version 4.19.83
    kernel=Resource(
        "x86-linux-kernel-4.19.83",
        override=True,
    ),
    # The x86-parsec image will be automatically downloaded to the
    # `~/.cache/gem5` directory if not already present.
    disk_image=Resource(
        "x86-parsec",
        override=True,
    ),
    command=command,
)

# We need this for long running processes.
m5.disableAllListeners()

root = Root(full_system = True, system = board)

# sim_quantum must be set if KVM cores are used.

root.sim_quantum = int(1e9)

m5.instantiate()

# We maintain the wall clock time.

globalStart = time.time()

print("Running the simulation")
print("Using KVM cpu")

start_tick = m5.curTick()
end_tick = m5.curTick()
m5.stats.reset()

# We start the simulation

exit_event = m5.simulate()

# The first exit_event ends with a `workbegin` cause. This means that the
# system started successfully and the execution on the program started. The
# ROI begin is encountered (via m5_work_begin()).

if exit_event.getCause() == "workbegin":

    print("Done booting Linux")
    print("Resetting stats at the start of ROI!")

    m5.stats.reset()
    start_tick = m5.curTick()

    # We have completed up to this step using KVM cpu. Now we switch to timing
    # cpu for detailed simulation.

    processor.switch()
else:
    # `workbegin` call was never encountered.

    print("Unexpected termination of simulation before ROI was reached!")

    m5.stats.dump()
    end_tick = m5.curTick()

    gem5stats = get_simstat(root)

    try:
        # We get the number of committed instructions from the timing
        # cores (2, 3). We then sum and print them at the end.

        roi_insts = float(\
            json.loads(gem5stats.dumps())\
            ["system"]["processor"]["cores2"]["core"]["exec_context.thread_0"]\
            ["numInsts"]["value"]) + float(\
            json.loads(gem5stats.dumps())\
            ["system"]["processor"]["cores3"]["core"]["exec_context.thread_0"]\
            ["numInsts"]["value"]\
        )
    except KeyError:
        roi_insts = float(\
            json.loads(gem5stats.dumps())\
            ["system"]["processor"]["cores2"]["core"]["committedInsts"]\
            ["value"]\
        ) + float(\
            json.loads(gem5stats.dumps())\
            ["system"]["processor"]["cores3"]["core"]["committedInsts"]\
            ["value"]\
        )
    except:
        roi_insts = 0
        print("warn: Unable to retriveve the total number of committed \
        instructions")

    m5.stats.reset()
    print("Performance statistics:")

    print("Simulated time: %.2fs" % ((end_tick-start_tick)/1e12))
    print("Instructions executed: %d" % ((roi_insts)))
    print("Ran a total of", m5.curTick()/1e12, "simulated seconds")
    print("Total wallclock time: %.2fs, %.2f min" % \
                (time.time()-globalStart, (time.time()-globalStart)/60))
    exit(-1)

# The next exit_event is to simulate the ROI. It should be exited with a cause
# marked by `workend`.

exit_event = m5.simulate()

# Reached the end of ROI.
# We dump the stats here.

# We exepect that ROI ends with `workend`. Otherwise the simulation ended
# unexpectedly.
if exit_event.getCause() == "workend":
    print("Dump stats at the end of the ROI!")

    m5.stats.dump()
    end_tick = m5.curTick()
else:
    print("Unexpected termination of simulation while ROI was being executed!")

    m5.stats.dump()
    end_tick = m5.curTick()

    gem5stats = get_simstat(root)

    try:
        # We get the number of committed instructions from the timing
        # cores. We then sum and print them at the end.

        roi_insts = float(\
            json.loads(gem5stats.dumps())\
            ["system"]["processor"]["cores2"]["core"]["exec_context.thread_0"]\
            ["numInsts"]["value"]) + float(\
            json.loads(gem5stats.dumps())\
            ["system"]["processor"]["cores3"]["core"]["exec_context.thread_0"]\
            ["numInsts"]["value"]\
        )
    except KeyError:
        roi_insts = float(\
            json.loads(gem5stats.dumps())\
            ["system"]["processor"]["cores2"]["core"]["committedInsts"]\
            ["value"]\
        ) + float(\
            json.loads(gem5stats.dumps())\
            ["system"]["processor"]["cores3"]["core"]["committedInsts"]\
            ["value"]\
        )
    except:
        roi_insts = 0
        print("warn: Unable to retriveve the total number of committed \
        instructions")

    m5.stats.reset()
    print("Performance statistics:")

    print("Simulated time: %.2fs" % ((end_tick-start_tick)/1e12))
    print("Instructions executed: %d" % ((roi_insts)))
    print("Ran a total of", m5.curTick()/1e12, "simulated seconds")
    print("Total wallclock time: %.2fs, %.2f min" % \
                (time.time()-globalStart, (time.time()-globalStart)/60))
    exit(-1)

# We need to note that the benchmark is not executed completely till this
# point, but, the ROI has. We collect the essential statistics here before
# resuming the simulation again.

# We get simInsts using get_simstat and output it in the final
# print statement.

gem5stats = get_simstat(root)

try:
    # We get the number of committed instructions from the timing
    # cores (2, 3). We then sum and print them at the end.

    roi_insts = float(\
        json.loads(gem5stats.dumps())\
        ["system"]["processor"]["cores2"]["core"]["exec_context.thread_0"]\
        ["numInsts"]["value"]) + float(\
        json.loads(gem5stats.dumps())\
        ["system"]["processor"]["cores3"]["core"]["exec_context.thread_0"]\
        ["numInsts"]["value"]\
)
except KeyError:
    roi_insts = float(\
        json.loads(gem5stats.dumps())\
        ["system"]["processor"]["cores2"]["core"]["committedInsts"]["value"]\
    ) + float(\
        json.loads(gem5stats.dumps())\
        ["system"]["processor"]["cores3"]["core"]["committedInsts"]["value"]\
    )
except:
    roi_insts = 0
    print("warn: Unable to retriveve the total number of committed \
    instructions")

# We need to end the execution of the program. Ideally, we want to switch back
# from a timing cpu to a KVM cpu. However, we cannot maintain the `MESI Two
# Level` cache states. Therefore, we continue executing the benchmark with
# the timing cpu.

exit_event = m5.simulate()

# Simulation is over at this point. But we need to verify the instruction that
# we encountered. Since there is an `m5 exit` instruction, we should receive
# a `m5_exit instruction encountered` as the exit_event's cause.

if exit_event.getCause() == "m5_exit instruction encountered":
    # We acknowledge that all the simulation events were successful.
    print("All simulation events were successful.")

else:
    print("Unexpected termination of simulation!")
    print()

    m5.stats.dump()
    end_tick = m5.curTick()
    m5.stats.reset()
    print("Performance statistics:")

    print("Simulated time: %.2fs" % ((end_tick-start_tick)/1e12))
    print("Instructions executed: %d" % ((roi_insts)))
    print("Ran a total of", m5.curTick()/1e12, "simulated seconds")
    print("Total wallclock time: %.2fs, %.2f min" % \
                (time.time()-globalStart, (time.time()-globalStart)/60))
    exit(-1)


# We print the final simulation statistics.

print("Done with the simulation")
print()
print("Performance statistics:")

print("Simulated time in ROI: %.2fs" % ((end_tick-start_tick)/1e12))
print("Instructions executed in ROI: %d" % ((roi_insts)))
print("Ran a total of", m5.curTick()/1e12, "simulated seconds")
print("Total wallclock time: %.2fs, %.2f min" % \
            (time.time()-globalStart, (time.time()-globalStart)/60))
exit(0)

