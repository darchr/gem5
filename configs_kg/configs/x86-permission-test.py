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


"""
import argparse
import os
import sys
import time

# all the source files are one directory above.
sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
)
from boards.x86_permission_board import X86PermissionBoard

import m5
from m5.objects import (
    AddrRange,
    Root,
)

from gem5.components.memory import SingleChannelDDR4_2400
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_switchable_processor import (
    SimpleSwitchableProcessor,
)
from gem5.isas import ISA
from gem5.resources.resource import *
from gem5.resources.resource import obtain_resource
from gem5.resources.workload import *
from gem5.simulate.exit_event import ExitEvent
from gem5.simulate.simulator import Simulator
from gem5.utils.requires import requires

requires(
    isa_required=ISA.X86,
    kvm_required=True,
)

parser = argparse.ArgumentParser(
    description="An example configuration script to run the gapbs benchmarks."
)

# The only positional argument accepted is the benchmark name in this script.
parser.add_argument(
    "--benchmark",
    type=str,
    required=False,
    help="Input the benchmark program to execute.",
)

args = parser.parse_args()


# Setting up all the fixed system parameters here
# Caches: MESI Two Level Cache Hierarchy

from cachehierarchies.dm_caches import *  # ClassicPrivateL1PrivateL2SharedL3CacheHierarchyWChecks

# cache_hierarchy = ClassicPrivateL1PrivateL2DMCache(
#     l1d_size = "32KiB",
#     l1i_size = "32KiB",
#     l2_size = "512 KiB",)

cache_hierarchy = ClassicPrivateL1PrivateL2SharedL3CacheHierarchyWChecks(
    l1d_size="32KiB",
    l1i_size="32KiB",
    l2_size="512 KiB",
    l3_size="8MiB",
)

# configure the permission table
cache_hierarchy.get_permission_table().enable_permission_check = True
cache_hierarchy.get_permission_table().total_memory_size = 0x20000000

# Memory: Dual Channel DDR4 2400 DRAM device.
# The X86 board only supports 3 GiB of main memory.

memory = SingleChannelDDR4_2400(size="1GiB")

# Here we setup the processor. This is a special switchable processor in which
# a starting core type and a switch core type must be specified. Once a
# configuration is instantiated a user may call `processor.switch()` to switch
# from the starting core types to the switch core types. In this simulation
# we start with KVM cores to simulate the OS boot, then switch to the Timing
# cores for the command we wish to run after boot.

processor = SimpleSwitchableProcessor(
    starting_core_type=CPUTypes.TIMING,
    switch_core_type=CPUTypes.TIMING,
    isa=ISA.X86,
    num_cores=2,
)

# Here we setup the board. The X86Board allows for Full-System X86 simulations

board = X86PermissionBoard(
    clk_freq="3GHz",
    processor=processor,
    cache_hierarchy=cache_hierarchy,
    memory=memory,
    os_memory_range="1G",
    permission_table_range=AddrRange(start=0x0, size="1GiB"),
)

# Here we set the FS workload, i.e., gapbs benchmark program
# After simulation has ended you may inspect
# `m5out/system.pc.com_1.device` to the stdout, if any.

# After the system boots, we execute the benchmark program and wait till the
# ROI `workbegin` annotation is reached. We start collecting the number of
# committed instructions till ROI ends (marked by `workend`). We then finish
# executing the rest of the benchmark.

# board.set_workload(obtain_resource(args.benchmark))

cmd = [
    "echo 'This is the bfs for gapbs!';",
    "echo '12345' | sudo -S ndctl create-namespace -f -enamespace0.0 -m devdax;",
    # Enable users to read/write to the device
    "echo '12345' | sudo -S chmod a+rw /dev/dax0.0;",
    "echo '12345' | sudo -S dmesg;",
    "ls /dev;",
    "sleep 1;",
    # Ignore the boot time stats. Allocate a tiny graph.
    "echo '12345' | sudo /home/gem5/shared-gapbs/allocator -S 1 -x 0 -g 10;",
    # This program can simply exit now.
    "m5 exit;",
]
workload = CustomWorkload(
    function="set_kernel_disk_workload",
    parameters={
        "kernel": CustomResource(
            "/home/kaustavg/kernel/x86/linux-6.9.9/vmlinux"
        ),
        "disk_image": DiskImageResource(
            "/home/kaustavg/projects/kg-resources-2/src/shared-gapbs/x86-disk-image-24-04/x86-ubuntu"
        ),
        "readfile_contents": " ".join(cmd),
        "kernel_args": [
            "earlyprintk=ttyS0",
            "console=ttyS0",
            "lpj=7999923",
            "root=/dev/sda2",
            "no_systemd=true",  # init=/bin/bash",
            # "memmap=8G!7G",
            "mem=2G",
        ],
    },
)
board.set_workload(workload)


def handle_workbegin():
    print("Done booting Linux")
    print("Resetting stats at the start of ROI!")
    m5.stats.reset()
    global start_tick
    start_tick = m5.curTick()
    processor.switch()
    yield False  # E.g., continue the simulation.


def handle_workend():
    print("Dump stats at the end of the ROI!")
    m5.stats.dump()
    yield True  # Stop the simulation. We're done.


def on_exit():
    yield False


simulator = Simulator(
    board=board,
    on_exit_event={ExitEvent.EXIT: on_exit()},
)

# We maintain the wall clock time.

globalStart = time.time()

print("Running the simulation")
print("Using KVM cpu")

# There are a few thihngs to note regarding the gapbs benchamrks. The first is
# that there are several ROI annotations in the code present in the disk image.
# These ROI begin and end calls are inside a loop. Therefore, we only simulate
# the first ROI annotation in details. The X86Board currently does not support
#  `work items started count reached`.

simulator.run()
simulator.run()
simulator.run()
simulator.run()
simulator.run()
end_tick = m5.curTick()
# Since we simulated the ROI in details, therefore, simulation is over at this
# point.

# Simulation is over at this point. We acknowledge that all the simulation
# events were successful.
print("All simulation events were successful.")

# We print the final simulation statistics.
print("Done with the simulation")
print()
print("Performance statistics:")

print(
    f"Simulated time in ROI: {(end_tick - start_tick) / 1000000000000.0:.2f}s"
)
print(
    "Ran a total of", simulator.get_current_tick() / 1e12, "simulated seconds"
)
print(
    "Total wallclock time: %.2fs, %.2f min"
    % (time.time() - globalStart, (time.time() - globalStart) / 60)
)
