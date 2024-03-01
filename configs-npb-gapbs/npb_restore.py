# -*- coding: utf-8 -*-
# Copyright (c) 2019 The Regents of the University of California.
# All rights reserved.
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
#
# Authors: Jason Lowe-Power, Ayaz Akram

""" Script to run NAS parallel benchmarks with gem5.
    The script expects kernel, diskimage, mem_sys,
    cpu (kvm, atomic, or timing), benchmark to run
    and number of cpus as arguments.

    If your application has ROI annotations, this script will count the total
    number of instructions executed in the ROI. It also tracks how much
    wallclock and simulated time.
"""
import argparse
import time
import m5
import m5.ticks
from m5.objects import *

from system import *


def writeBenchScript(dir, bench):
    """
    This method creates a script in dir which will be eventually
    passed to the simulated system (to run a specific benchmark
    at bootup).
    """
    file_name = "{}/run_{}".format(dir, bench)
    bench_file = open(file_name, "w+")
    bench_file.write("/home/gem5/NPB3.3-OMP/bin/{} \n".format(bench))

    # sleeping for sometime (5 seconds here) makes sure
    # that the benchmark's output has been
    # printed to the console
    bench_file.write("sleep 5 \n")
    bench_file.write("m5 exit \n")
    bench_file.close()
    return file_name


supported_protocols = [
    "classic",
    "MI_example",
    "MESI_Two_Level",
    "MOESI_CMP_directory",
]
supported_cpu_types = ["kvm", "atomic", "timing"]
benchmark_choices = [
    "bt.A.x",
    "cg.A.x",
    "ep.A.x",
    "ft.A.x",
    "is.A.x",
    "lu.A.x",
    "mg.A.x",
    "sp.A.x",
    "bt.B.x",
    "cg.B.x",
    "ep.B.x",
    "ft.B.x",
    "is.B.x",
    "lu.B.x",
    "mg.B.x",
    "sp.B.x",
    "bt.C.x",
    "cg.C.x",
    "ep.C.x",
    "ft.C.x",
    "is.C.x",
    "lu.C.x",
    "mg.C.x",
    "sp.C.x",
    "bt.D.x",
    "cg.D.x",
    "ep.D.x",
    "ft.D.x",
    "is.D.x",
    "lu.D.x",
    "mg.D.x",
    "sp.D.x",
    "bt.F.x",
    "cg.F.x",
    "ep.F.x",
    "ft.F.x",
    "is.F.x",
    "lu.F.x",
    "mg.F.x",
    "sp.F.x",
]


def parse_options():

    parser = argparse.ArgumentParser(
        description="For use with gem5. This "
        "runs a NAS Parallel Benchmark application. This only works "
        "with x86 ISA."
    )

    # The manditry position arguments.
    parser.add_argument(
        "benchmark",
        type=str,  # choices=benchmark_choices,
        help="The NPB application to run",
    )
    parser.add_argument(
        "class_size", type=str, help="The NPB application class to run"
    )
    parser.add_argument(
        "dcache_policy",
        type=str,
        help="The architecture of DRAM cache: "
        "CascadeLakeNoPartWrs, Oracle, BearWriteOpt, Rambus",
    )
    parser.add_argument(
        "is_link",
        type=int,
        help="whether to use a link for backing store or not",
    )
    parser.add_argument(
        "link_lat", type=str, help="latency of the link to backing store"
    )
    parser.add_argument(
        "checkpoint_path", type=str, help="Path to checkpoint dir"
    )
    return parser.parse_args()


if __name__ == "__m5_main__":
    args = parse_options()

    kernel = "/home/mbabaie/code-review/dramCacheController/fs-resources/x86-linux-kernel-4.19.83"
    disk = "/home/mbabaie/code-review/dramCacheController/fs-resources/x86-npb"
    num_cpus = 8
    cpu_type = "Timing"
    mem_sys = "MESI_Two_Level"

    dcache_size = ""
    mem_size = ""
    if args.class_size == "C":
        dcache_size = "128MiB"
        mem_size = "16GiB"
    elif args.class_size == "D":
        dcache_size = "512MiB"
        mem_size = "85GiB"

    benchmark = args.benchmark + "." + args.class_size + ".x"

    system = MyRubySystem(
        kernel,
        disk,
        mem_sys,
        num_cpus,
        dcache_size,
        mem_size,
        args.dcache_policy,
        args.is_link,
        args.link_lat,
        args,
        restore=True,
    )

    system.m5ops_base = 0xFFFF0000

    # Exit from guest on workbegin/workend
    system.exit_on_work_items = True

    # Create and pass a script to the simulated system to run the reuired
    # benchmark
    system.readfile = writeBenchScript(m5.options.outdir, benchmark)

    # set up the root SimObject and start the simulation
    root = Root(full_system=True, system=system)

    if system.getHostParallel():
        # Required for running kvm on multiple host cores.
        # Uses gem5's parallel event queue feature
        # Note: The simulator is quite picky about this number!
        root.sim_quantum = int(1e9)  # 1 ms

    # needed for long running jobs
    m5.disableAllListeners()

    # instantiate all of the objects we've created above
    m5.instantiate(args.checkpoint_path)

    globalStart = time.time()

    print("Running the simulation ************************************** \n")
    print("Simulating 100 intervals of 10ms each! \n")

    for interval_number in range(100):
        print("Interval number: {} \n".format(interval_number))
        exit_event = m5.simulate(10000000000)
        m5.stats.dump()

    print("End of simulation ******************************************** \n")
