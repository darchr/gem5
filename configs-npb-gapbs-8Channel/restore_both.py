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
from info import (
    text_info,
    interval_info_1hr,
    interval_info_3hr,
    interval_info_6hr,
    interval_info_12hr,
    interval_info_24hr,
    benchmark_choices_gapbs,
    benchmark_choices_npb,
    interval_info_1hr_512MiB,
    interval_info_1GBdramCache_3hr,
)


def writeBenchScriptNPB(dir, bench):
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


def writeBenchScriptGAPBS(dir, benchmark_name):
    """
    This method creates a script in dir which will be eventually
    passed to the simulated system (to run a specific benchmark
    at bootup).
    """
    synthetic = True
    benchmark, size = benchmark_name.split("-")
    input_file_name = "{}/run_{}_{}".format(dir, benchmark, size)
    if synthetic:
        with open(input_file_name, "w") as f:
            f.write("./{} -g {}\n".format(benchmark, size))
    elif synthetic == 0:
        with open(input_file_name, "w") as f:
            # The workloads that are copied to the disk image using Packer
            # should be located in /home/gem5/.
            # Since the command running the workload will be executed with
            # pwd = /home/gem5/gapbs, the path to the copied workload is
            # ../{workload-name}
            f.write("./{} -sf ../{}".format(benchmark, size))

    return input_file_name


supported_protocols = [
    "classic",
    "MI_example",
    "MESI_Two_Level",
    "MOESI_CMP_directory",
]
supported_cpu_types = ["kvm", "atomic", "timing"]


def parse_options():
    parser = argparse.ArgumentParser(
        description="For use with gem5. This "
        "runs a NAS Parallel Benchmark application. This only works "
        "with x86 ISA."
    )

    # The manditry position arguments.
    parser.add_argument(
        "benchmark",
        type=str,
        choices=benchmark_choices_npb + benchmark_choices_gapbs,
        help="The NPB application to run",
    )
    parser.add_argument(
        "dcache_policy",
        type=str,
        help="The architecture of DRAM cache: "
        "CascadeLakeNoPartWrs, Oracle, BearWriteOpt, Rambus",
    )
    parser.add_argument(
        "assoc",
        type=int,
        help="THe associativity of the DRAM cache",
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
        "bypass",
        type=int,
        help="bypass DRAM cache",
    )
    parser.add_argument("--do_analysis", action="store_true", default=False)
    return parser.parse_args()


def do_analysis():
    print(
        "**************** Doing analysis! Simulating 100 intervals of 10ms each! ********************\n"
    )
    start = time.time()

    for interval_number in range(100):
        print(f"Working on interval number: {interval_number}")
        exit_event = m5.simulate(10_000_000_000)  # 10 ms
        m5.stats.dump()

        print(
            f"Done with interval {interval_number} at {(time.time() - start)/60:0.2f}"
        )
        mostRecentPc = lpmanager.getMostRecentPc()
        print(f"Exit because {exit_event.getCause()}, before for")
        for pc, tick in mostRecentPc:
            count = lpmanager.getPcCount(pc)
            print("in for loop")
            print(f"{hex(pc)},{count[0]},{count[1]}")
        if exit_event.getCause() != "simulate() limit reached":
            if (
                exit_event.getCause() == "workend"
                or exit_event.getCause() == "workbegin"
            ):
                print(f"Exit because {exit_event.getCause()}, continuing...")
            else:
                print(f"Exiting because {exit_event.getCause()}")
                break


def run():
    print("Simulating 100 intervals of 10ms each! \n")

    for interval_number in range(100):
        print("Interval number: {}".format(interval_number))
        exit_event = m5.simulate(10_000_000_000)  # 10 ms
        # m5.stats.dump()

        if exit_event.getCause() != "simulate() limit reached":
            if (
                exit_event.getCause() == "workend"
                or exit_event.getCause() == "workbegin"
            ):
                print("Workload finished, continuing...")
            else:
                print(f"Exiting because {exit_event.getCause()}")
                break


if __name__ == "__m5_main__":
    args = parse_options()

    kernel = "/home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83"
    disk = "/home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb"
    ckpt_base = "/home/babaie/projects/rambusDesign/1gigDRAMCache/dramCacheController/chkpt1GigDC/"

    num_cpus = 8
    cpu_type = "Timing"
    mem_sys = "MESI_Two_Level"

    dcache_size = "1GB"
    mem_size = ""
    mem_size_per_channel = "64GiB"
    checkpoint_dir = ""

    if args.benchmark in benchmark_choices_npb:
        if args.benchmark.split(".")[1] == "C":
            checkpoint_dir = (
                ckpt_base
                + "1GB_8GB_g22_nC_1halfSec/NPB/"
                + args.benchmark.split(".")[0]
                + "/cpt"
            )
            mem_size = "8GiB"
        elif args.benchmark.split(".")[1] == "D":
            checkpoint_dir = (
                ckpt_base
                + "1GB_85GB_g25_nD_1halfSec/NPB/"
                + args.benchmark.split(".")[0]
                + "/cpt"
            )
            mem_size = "85GiB"
    else:
        if args.benchmark.split("-")[1] == "22":
            checkpoint_dir = (
                ckpt_base
                + "1GB_8GB_g22_nC_1halfSec/GAPBS/"
                + args.benchmark.split("-")[0]
                + "/cpt"
            )
            mem_size = "8GiB"
        elif args.benchmark.split("-")[1] == "25":
            checkpoint_dir = (
                ckpt_base
                + "1GB_85GB_g25_nD_1halfSec/GAPBS/"
                + args.benchmark.split("-")[0]
                + "/cpt"
            )
            mem_size = "85GiB"

    benchmark = args.benchmark

    system = MyRubySystem(
        kernel,
        disk,
        mem_sys,
        num_cpus,
        args.assoc,
        dcache_size,
        mem_size_per_channel,
        mem_size,
        args.dcache_policy,
        args.is_link,
        args.link_lat,
        args.bypass,
        args,
        restore=True,
    )

    if args.do_analysis:
        lpmanager = O3LooppointAnalysisManager()

        for core in system.o3Cpu:
            lplistener = O3LooppointAnalysis()
            lplistener.ptmanager = lpmanager
            lplistener.validAddrRangeStart = text_info[args.benchmark][1]
            lplistener.validAddrRangeSize = text_info[args.benchmark][0]
            core.probeListener = lplistener
    else:
        pc, count = interval_info_1GBdramCache_3hr[args.benchmark]
        system.global_tracker = PcCountTrackerManager(
            targets=[PcCountPair(pc, count)]
        )

        for core in system.o3Cpu:
            core.core_tracker = PcCountTracker(
                targets=[PcCountPair(pc, count)],
                core=core,
                ptmanager=system.global_tracker,
            )

    system.m5ops_base = 0xFFFF0000

    # Exit from guest on workbegin/workend
    system.exit_on_work_items = True

    # Create and pass a script to the simulated system to run the reuired
    # benchmark
    if args.benchmark in benchmark_choices_npb:
        system.readfile = writeBenchScriptNPB(
            m5.options.outdir, args.benchmark
        )
    else:
        system.readfile = writeBenchScriptGAPBS(
            m5.options.outdir, args.benchmark
        )

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
    m5.instantiate(checkpoint_dir)

    print("Running the simulation ************************************** \n")

    if args.do_analysis:
        do_analysis()
    else:
        run()

    print("End of simulation ******************************************** \n")
