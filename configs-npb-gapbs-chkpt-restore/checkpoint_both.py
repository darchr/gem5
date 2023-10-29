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

""" Script to run GAP Benchmark suites workloads.
    The workloads have two modes: synthetic and real graphs.
"""
import argparse
import time
import m5
import m5.ticks
from m5.objects import *
from system import *

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
        description="For use with gem5. This script "
        "runs a GAPBS/NPB application and only works "
        "with x86 ISA."
    )
    parser.add_argument(
        "isGAPBS", type=int, help="GAPBS (1) application to run or NPB (0)"
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

    return parser.parse_args()


if __name__ == "__m5_main__":
    args = parse_options()

    kernel = "/home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83"
    disk = ""
    if args.isGAPBS == 1:
        disk = "/home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-gapbs"
    elif args.isGAPBS == 0:
        disk = "/home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb"


    num_cpus = 8
    cpu_type = "Timing"
    mem_sys = "MESI_Two_Level"
    synthetic = 1
    dcache_size = "1GiB" # size of each channel
    mem_size = "128GiB"
    mem_size_per_channel = "64GiB"
    assoc = 1

    # create the system we are going to simulate
    system = RubySystem8Channel(
        kernel,
        disk,
        mem_sys,
        num_cpus,
        assoc,
        dcache_size,
        mem_size,
        mem_size_per_channel,
        args.dcache_policy,
        0,
        0,
        0,
        args,
    )

    system.m5ops_base = 0xFFFF0000

    # Exit from guest on workbegin/workend
    system.exit_on_work_items = True

    # Create and pass a script to the simulated system to run the reuired
    # benchmark
    if args.isGAPBS == 1:
        system.readfile = writeBenchScript_GAPBS(
            m5.options.outdir,
            args.benchmark,
            args.size,
            synthetic
        )
    elif args.isGAPBS == 0:
        system.readfile = writeBenchScript_NPB(
            m5.options.outdir,
            args.benchmark+"."+args.size+".x"
        )

    # set up the root SimObject and start the simulation
    root = Root(full_system=True, system=system)

    if system.getHostParallel():
        # Required for running kvm on multiple host cores.
        # Uses gem5's parallel event queue feature
        # Note: The simulator is quite picky about this number!
        root.sim_quantum = int(1e9)  # 1 ms

    # needed for long running jobs
    # m5.disableAllListeners()

    # instantiate all of the objects we've created above
    m5.instantiate()

    globalStart = time.time()

    print("Running the simulation")
    print("Using cpu: {}".format(cpu_type))
    exit_event = m5.simulate()

    if exit_event.getCause() == "workbegin":
        print("Done booting Linux")
        # Reached the start of ROI
        # start of ROI is marked by an
        # m5_work_begin() call
        print("Resetting stats at the start of ROI!")
        m5.stats.reset()
        start_tick = m5.curTick()
        start_insts = system.totalInsts()
        # switching CPU to timing
        system.switchCpus(system.cpu, system.timingCpu)
    else:
        print(exit_event.getCause())
        print("Unexpected termination of simulation !")
        exit(1)

    m5.stats.reset()
    print(
        "After reset ************************************************ statring smiulation:\n"
    )
    for interval_number in range(1):
        print("Interval number: {} \n".format(interval_number))
        exit_event = m5.simulate(1000000000)
        if exit_event.getCause() == "cacheIsWarmedup":
            print("Caught cacheIsWarmedup exit event!")
            break
        print(
            "-------------------------------------------------------------------"
        )

    print(
        "After sim ************************************************ End of warm-up \n"
    )
    m5.stats.dump()
    system.switchCpus(system.timingCpu, system.o3Cpu)
    print("switched from timing to O3")
    m5.checkpoint(m5.options.outdir + "/cpt")
