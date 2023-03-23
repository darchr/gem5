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

supported_protocols = ["MESI_Two_Level"]
supported_cpu_types = ['kvm', 'atomic', 'timing']

def writeBenchScript(dir, benchmark_name, size, synthetic):
    """
    This method creates a script in dir which will be eventually
    passed to the simulated system (to run a specific benchmark
    at bootup).
    """
    input_file_name = '{}/run_{}_{}'.format(dir, benchmark_name, size)
    if (synthetic):
        with open(input_file_name,"w") as f:
            f.write('./{} -g {}\n'.format(benchmark_name, size))
    elif(synthetic==0):
        with open(input_file_name,"w") as f:
            # The workloads that are copied to the disk image using Packer
            # should be located in /home/gem5/.
            # Since the command running the workload will be executed with
            # pwd = /home/gem5/gapbs, the path to the copied workload is
            # ../{workload-name}
            f.write('./{} -sf ../{}'.format(benchmark_name, size))

    return input_file_name

def parse_options():

    parser = argparse.ArgumentParser(description='For use with gem5. This '
                'runs a GAPBS applications. This only works '
                'with x86 ISA.')

    # The manditry position arguments.
    parser.add_argument("kernel", type=str,
                        help="Path to the kernel binary to boot")
    parser.add_argument("disk", type=str,
                        help="Path to the disk image to boot")
    parser.add_argument("cpu", type=str, choices=supported_cpu_types,
                        help="The type of CPU to use in the system")
    parser.add_argument("num_cpus", type=int, help="Number of CPU cores")
    
    parser.add_argument("mem_sys", type=str, choices=supported_protocols,
                        help="Type of memory system or coherence protocol")
    parser.add_argument("benchmark", type=str,
                        help="The NPB application to run")
    parser.add_argument("synthetic", type = int,
                        help = "1 for synthetic graph, 0 for real graph")
    parser.add_argument("graph", type = str,
                        help = "synthetic=1: integer number. synthetic=0: graph")
    parser.add_argument("checkpoint_path", help="Path to checkpoint dir")

    return parser.parse_args()

if __name__ == "__m5_main__":
    args = parse_options()


    # create the system we are going to simulate
    system = MyRubySystem(args.kernel, args.disk, args.mem_sys,
                          args.num_cpus, args, restore=True)

    system.m5ops_base = 0xffff0000

    # Exit from guest on workbegin/workend
    system.exit_on_work_items = True

    # Create and pass a script to the simulated system to run the reuired
    # benchmark
    system.readfile = writeBenchScript(m5.options.outdir, args.benchmark,
                                       args.graph, args.synthetic)

    # set up the root SimObject and start the simulation
    root = Root(full_system = True, system = system)

    if system.getHostParallel():
        # Required for running kvm on multiple host cores.
        # Uses gem5's parallel event queue feature
        # Note: The simulator is quite picky about this number!
        root.sim_quantum = int(1e9) # 1 ms

    #needed for long running jobs
    m5.disableAllListeners()
    
    # instantiate all of the objects we've created above
    m5.instantiate(args.checkpoint_path)
    
    globalStart = time.time()
    
    print("Running the simulation ************************************** \n")
    print("Simulating 10 intervals of 100ms each! \n")

    numIteration = 0

    if args.benchmark=="bfs":
        numIteration = 40
    elif args.benchmark=="cc":
        numIteration = 40
    elif args.benchmark=="sssp":
        numIteration = 20
    else:
        numIteration = 10

    for interval_number in range(numIteration):
        print("Interval number: {} \n".format(interval_number))
        exit_event = m5.simulate(100000000000)
        m5.stats.dump()

    print("End of simulation ******************************************** \n")
