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

""" Script to run and take checkpoints for both GAPBS and NPB
"""
import argparse
import time
import m5
import m5.ticks
from m5.objects import *
from system import *
from m5.stats.gem5stats import get_simstat

from info import (
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
        description="For use with gem5. This script "
        "runs a GAPBS/NPB application and only works "
        "with x86 ISA."
    )
    parser.add_argument(
        "benchmark", type=str, help="The application to run"
    )
    parser.add_argument(
        "size", type=str, help="The problem size to run"
    )
    return parser.parse_args()


if __name__ == "__m5_main__":
    args = parse_options()

    kernel = "/home/babaie/projects/TDRAM-resubmission/fsTools/x86-linux-kernel-4.19.83"
    disk = ""
    if args.benchmark in gapbs_benchmarks:
        disk = "/home/babaie/projects/TDRAM-resubmission/fsTools/x86-gapbs"
    elif args.benchmark in npb_benchmarks:
        disk = "/home/babaie/projects/TDRAM-resubmission/fsTools/x86-npb"
    else:
        print("Wrong benchmark choice!")
        exit(1)

    synthetic = 1
    num_cpus = 8
    mem_sys = "MESI_Two_Level"
    dcache_policy = "RambusTagProbOpt"
    dcache_size = "1GiB" # size of each channel
    mem_size = "8GiB" # size of total main memory
    mem_size_per_channel = "8GiB"
    assoc = 1
    singleChannel = True

    # create the system we are going to simulate
    if singleChannel:
        system = RubySystem1Channel(
            kernel,
            disk,
            mem_sys,
            num_cpus,
            assoc,
            dcache_size,
            mem_size,
            mem_size_per_channel,
            dcache_policy,
            0,
            0,
            0,
            args,
        )
    else:
        system = RubySystem8Channel(
            kernel,
            disk,
            mem_sys,
            num_cpus,
            assoc,
            dcache_size,
            mem_size,
            mem_size_per_channel,
            dcache_policy,
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
            args.benchmark+"."+args.size+".x"
        )

    # set up the root SimObject and start the simulation
    root = Root(full_system=True, system=system)

    if system.getHostParallel():
        # Required for running kvm on multiple host cores.
        # Uses gem5's parallel event queue feature
        # Note: The simulator is quite picky about this number!
        root.sim_quantum = int(1e9)  # 1 ms

    # instantiate all of the objects we've created above
    m5.instantiate()

    print("Running the simulation")
    exit_event = m5.simulate()

    if exit_event.getCause() == "workbegin":
        # Reached the start of ROI
        # start of ROI is marked by an
        # m5_work_begin() call
        print("Done booting Linux and reached to ROI")
        m5.stats.reset()
        print("Reset stats at the start of ROI")
        start_tick = m5.curTick()
        start_insts = system.totalInsts()
        # switching CPU to timing
        system.switchCpus(system.cpu, system.timingCpu)
        print("Switched CPU from KVM to Timing!")
    else:
        print(exit_event.getCause())
        print("Unexpected termination of simulation !")
        exit(1)

    print("Start to running intervals!")
    totalColdMisses = 0
    numOfCacheBlks = 0
    if singleChannel:
        numOfCacheBlks = system.mem_ctrl.dram_cache_size / system.mem_ctrl.block_size
    else:
        numOfCacheBlks = num_cpus * system.mem_ctrl[0].dram_cache_size / system.mem_ctrl[0].block_size
    print(numOfCacheBlks)

    for interval_number in range(1): # 2.5 seconds
        print("Interval number: {}\n".format(interval_number))
        intervalColdMisses = 0
        intervalTotalReqs = 0

        exit_event = m5.simulate(100000000) # 100 ms
        #if exit_event.getCause() != "simulate() limit reached":
        #    if (
        #        exit_event.getCause() == "workend"
        #        or exit_event.getCause() == "workbegin"
        #    ):
        #        print("ROI bounds, continuing to stats ...")
        #    else:
        #        print(f"Exiting because {exit_event.getCause()}")
        #        exit(1)
        #
        #simstats = get_simstat([polMan for polMan in system.mem_ctrl], prepare_stats=True)
        #for i in range(num_cpus):
        #    ctrl = simstats.__dict__[f"mem_ctrl{i}"]
        #    intervalColdMisses += ctrl.numColdMisses.value
        #    intervalTotalReqs += (ctrl.readReqs.value + ctrl.writeReqs.value)
#
        #totalColdMisses += intervalColdMisses
#
        #if totalColdMisses >= (numOfCacheBlks*0.95):
        #    print("95%% of system's total DRAM cache is warmed up")
        #    break
        #elif (interval_number >= 150 and
        #      float(intervalColdMisses/intervalTotalReqs) <= 0.05 and
        #      exit_event.getCause() == "simulate() limit reached"):
        #    print("Have run at about 1.5 sec and cold misses in a 100 ms"
        #          "interval has been less than 5%%")
        #    break
        #m5.stats.reset()
        #print("Warmup ratio so far: {}\n".format(float(totalColdMisses/numOfCacheBlks)))
  
    print("Exited warmup iterations")
    if interval_number == 249 :
        print("TIMEOUT!\n")
    m5.stats.dump()
    system.switchCpus(system.timingCpu, system.o3Cpu)
    print("switched from timing to O3")
    m5.checkpoint(m5.options.outdir + "/cpt")

