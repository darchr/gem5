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

""" Script to restore both GAPBS and NPB with a checkpoint
"""
import argparse
import time
import m5
import m5.ticks
from m5.objects import *

from system import *

from info import (
    text_info,
    interval_info_1GBdramCache_3hr,
    benchmark_choices_gapbs,
    benchmark_choices_npb,
    gapbs_benchmarks,
    npb_benchmarks,
)

def writeBenchScript_GAPBS(dir, benchmark_name, size, synthetic):
    """
    This method creates a script in dir which will be eventually
    passed to the simulated system (to run a specific benchmark
    at bootup).
    """
    input_file_name = "{}/run_{}_{}".format(dir, benchmark_name, size)
    if synthetic:
        with open(input_file_name, "w") as f:
            f.write("./{} -g {}\n".format(benchmark_name, size))
    elif synthetic == 0:
        with open(input_file_name, "w") as f:
            # The workloads that are copied to the disk image using Packer
            # should be located in /home/gem5/.
            # Since the command running the workload will be executed with
            # pwd = /home/gem5/gapbs, the path to the copied workload is
            # ../{workload-name}
            f.write("./{} -sf ../{}".format(benchmark_name, size))

    return input_file_name

def writeBenchScript_NPB(dir, bench):
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


def parse_options():
    parser = argparse.ArgumentParser(
        description="Restores a checkpoint for NPB and GAPBS"
    )
    parser.add_argument(
        "benchmark", type=str, help="The application to run"
    )
    parser.add_argument(
        "size", type=str, help="The problem size to run"
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

    kernel = "/home/babaie/projects/TDRAM-resubmission/fsTools/x86-linux-kernel-4.19.83"
    disk = ""
    if args.benchmark in gapbs_benchmarks:
        disk = "/home/babaie/projects/TDRAM-resubmission/fsTools/x86-gapbs"
    elif args.benchmark in npb_benchmarks:
        disk = "/home/babaie/projects/TDRAM-resubmission/fsTools/x86-npb"
    else:
        print("wrong benchmark choice!")
        exit(1)

    synthetic = 1
    num_cpus = 8
    mem_sys = "MESI_Two_Level"
    dcache_size = "1GiB" # size of each channel
    mem_size = "128GiB" # size of total main memory
    mem_size_per_channel = "64GiB"
    single_channel = False

    checkpoint_dir = ""
    
    if single_channel:
            system = RubySystem1Channel(
            kernel,
            disk,
            mem_sys,
            num_cpus,
            args.assoc,
            dcache_size,
            mem_size,
            mem_size_per_channel,
            args.dcache_policy,
            args.is_link,
            args.link_lat,
            args.bypass,
            args,
            restore=True,
        )
    else:
        system = RubySystem8Channel(
            kernel,
            disk,
            mem_sys,
            num_cpus,
            args.assoc,
            dcache_size,
            mem_size,
            mem_size_per_channel,
            args.dcache_policy,
            args.is_link,
            args.link_lat,
            args.bypass,
            args,
            restore=True,
        )

    app = ""
    if args.benchmark in gapbs_benchmarks:
        app = args.benchmark + "-" + args.size
    elif args.benchmark in npb_benchmarks:
        app = args.benchmark+"."+args.size+".x"

    if args.do_analysis:
        lpmanager = O3LooppointAnalysisManager()
        for core in system.o3Cpu:
            lplistener = O3LooppointAnalysis()
            lplistener.ptmanager = lpmanager
            lplistener.validAddrRangeStart = text_info[app][1]
            lplistener.validAddrRangeSize = text_info[app][0]
            core.probeListener = lplistener
    else:
        pc, count = interval_info_1GBdramCache_3hr[app]
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
    if args.benchmark in gapbs_benchmarks:
        system.readfile = writeBenchScript_GAPBS(
            m5.options.outdir,
            args.benchmark,
            args.size,
            synthetic
        )
    elif args.benchmark in npb_benchmarks:
        system.readfile = writeBenchScript_NPB(
            m5.options.outdir,
            app
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
