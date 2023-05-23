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
scons build/X86/gem5.opt
./build/X86/gem5.opt \
    configs/example/gem5_library/x86-npb-benchmarks.py \
    --benchmark <benchmark_name> \
    --size <benchmark_class>
```
"""

import argparse
import time

import m5
from m5.objects import Root

from gem5.utils.requires import requires
from gem5.components.boards.x86_board import X86Board
from gem5.components.memory import DualChannelDDR4_2400
from gem5.components.processors.simple_processor import (
    SimpleProcessor,
)
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.coherence_protocol import CoherenceProtocol
from gem5.resources.resource import Resource
from gem5.simulate.simulator import Simulator
from gem5.simulate.simulator import ExitEvent
from m5.params import PcCountPair
import json
from pathlib import Path

from m5.objects import LooppointAnalysis, LooppointAnalysisManager

from m5.stats.gem5stats import get_simstat
from m5.util import warn

requires(isa_required=ISA.X86)

# Following are the list of benchmark programs for npb.

benchmark_choices = ["bt", "cg", "ep", "ft", "is", "lu", "mg", "sp"]

# We are restricting classes of NPB to A, B and C as the other classes (D and
# F) require main memory size of more than 3 GB. The X86Board is currently
# limited to 3 GB of memory. This limitation is explained later in line 136.

# The resource disk has binaries for class D. However, only `ep` benchmark
# works with class D in the current configuration. More information on the
# memory footprint for NPB is available at https://arxiv.org/abs/2010.13216

size_choices = ["A", "B", "C"]

parser = argparse.ArgumentParser(
    description="An example configuration script to run the npb benchmarks."
)

# The only positional argument accepted is the benchmark name in this script.

parser.add_argument(
    "--benchmark",
    type=str,
    required=True,
    help="Input the benchmark program to execute.",
    choices=benchmark_choices,
)

parser.add_argument(
    "--size",
    type=str,
    required=True,
    help="Input the class of the program to simulate.",
    choices=size_choices,
)

parser.add_argument(
    "--ticks",
    type=int,
    help="Optionally put the maximum number of ticks to execute during the "
    "ROI. It accepts an integer value.",
)

args = parser.parse_args()

# The simulation may fail in the case of `mg` with class C as it uses 3.3 GB
# of memory (more information is availabe at https://arxiv.org/abs/2010.13216).
# We warn the user here.

if args.benchmark == "mg" and args.size == "C":
    warn(
        "mg.C uses 3.3 GB of memory. Currently we are simulating 3 GB\
    of main memory in the system."
    )

# The simulation will fail in the case of `ft` with class C. We warn the user
# here.
elif args.benchmark == "ft" and args.size == "C":
    warn(
        "There is not enough memory for ft.C. Currently we are\
    simulating 3 GB of main memory in the system."
    )


from gem5.components.cachehierarchies.classic.no_cache import NoCache

cache_hierarchy = NoCache()

memory = DualChannelDDR4_2400(size="3GB")

num_cores = 5
lpmanager = LooppointAnalysisManager()
lpmanager.regionLen = num_cores * 100000000
lptrackers = []

print(
    f"setting as {num_cores} cores and"
    f" region length as {lpmanager.regionLen}\n"
)

processor = SimpleProcessor(
    cpu_type=CPUTypes.ATOMIC,
    isa=ISA.X86,
    num_cores=num_cores,
)

for core in processor.get_cores():
    lplistener = LooppointAnalysis()
    lptrackers.append(lplistener)
    lplistener.ptmanager = lpmanager
    # lplistener.validAddrRangeStart=0x4011b0
    lplistener.validAddrRangeStart = int("0x400e50", 16)
    # lplistener.validAddrRangeSize=0x18b15
    lplistener.validAddrRangeSize = int("0x18402", 16)
    core.core.probeListener = lplistener


print(f"lptrackers size : {len(lptrackers)}\n")

board = X86Board(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

command = (
    f"/home/gem5/NPB3.3-OMP/bin/{args.benchmark}.{args.size}.x;"
    + "sleep 5;"
    + "m5 exit;"
)

board.set_kernel_disk_workload(
    kernel=Resource("x86-linux-kernel-4.19.83"),
    disk_image=Resource("x86-npb"),
    readfile_contents=command,
)


def dump_n_reset_lpinfo(i):
    info = {}
    info["global inst"] = lpmanager.getGlobalInstCounter()
    info["global counter"] = lpmanager.getCounter()
    info["global BBinst profile"] = lpmanager.getBBinst()
    lpmanager.clearGlobalInstCounter()
    for index, thread in enumerate(lptrackers):
        info[f"core {index}"] = {
            "BBfreq": thread.getBBfreq(),
            "mostRecentPcCount": {
                f"{item[0].get_pc()}": {
                    "count": item[0].get_count(),
                    "Tick": item[1],
                }
                for item in thread.getlocalMostRecentPcCount()
            },
        }
        thread.clearBBfreq()
    with open(f"lpinfo{i}.json", "w") as file:
        json.dump(info, file, indent=4)


def handle_lpevent():
    counter = 0
    while True:
        dump_n_reset_lpinfo(counter)
        counter += 1
        yield False


def handle_workend():
    print("Dump stats at the end of the ROI!")
    dump_n_reset_lpinfo("end")
    m5.stats.dump()
    yield True


simulator = Simulator(
    board=board,
    full_system=True,
    on_exit_event={
        ExitEvent.WORKEND: handle_workend(),
        ExitEvent.SIMPOINT_BEGIN: handle_lpevent(),
    },
    checkpoint_path=Path(
        "/home/studyztp/test_ground/looppoint_analysis/data/take-cpt-after-boot/checkpoint-after-boot"
    ).as_posix(),
)

print("Restoring the simulation")

simulator.run()
