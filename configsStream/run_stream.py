#Copyright (c) 2020 The Regents of the University of California.
#All Rights Reserved
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
#


""" Script to run GAP Benchmark suites workloads.
    The workloads have two modes: synthetic and real graphs.
"""

import m5
import m5.ticks
from m5.objects import *

import argparse

from system import *

def parse_arguments():
    parser = argparse.ArgumentParser(description=
                                "gem5 config file to run GAPBS")
    parser.add_argument("kernel", type = str, help = "Path to vmlinux")
    parser.add_argument("disk", type = str,
                        help = "Path to the disk image containing GAPBS")
    parser.add_argument("cpu_type", type = str, help = "Name of the detailed CPU")
    parser.add_argument("num_cpus", type = int, help = "Number of CPUs")
    parser.add_argument("mem_sys", type = str,
                        help = "Memory model, Classic or MI_example")
    parser.add_argument("num_elements", type=str,
                        help = "Number of elements of the STREAM arrays, size in bytes = 8 * num_elements")
    parser.add_argument("-z", "--allow-listeners", default = False,
                        action = "store_true",
                        help = "Turn on listeners (e.g. gdb listener port);"
                               "The listeners are off by default")
    return parser.parse_args()


def writeBenchScript(dir, num_elements):
    """
    This method creates a script in dir which will be eventually
    passed to the simulated system (to run a specific benchmark
    at bootup).
    """
    input_file_name = f'{dir}/run_{num_elements}'
    with open(input_file_name,"w") as f:
        f.write(f"{num_elements}")

    return input_file_name

if __name__ == "__m5_main__":
    args = parse_arguments()

    kernel = args.kernel
    disk = args.disk
    cpu_type = args.cpu_type
    num_cpus = args.num_cpus
    mem_sys = args.mem_sys
    num_elements = args.num_elements
    allow_listeners = args.allow_listeners

    if (mem_sys == "classic"):
        system = MySystem(kernel, disk, cpu_type, num_cpus)
    elif (mem_sys == "MI_example" or "MESI_Two_Level"):
        system = MyRubySystem(kernel, disk, mem_sys, num_cpus)

    system.m5ops_base = 0xffff0000

    # Read in the script file passed in via an option.
    # This file gets read and executed by the simulated system after boot.
    # Note: The disk image needs to be configured to do this.

    system.readfile = writeBenchScript(m5.options.outdir, num_elements)

    # set up the root SimObject and start the simulation
    root = Root(full_system = True, system = system)

    if system.getHostParallel():
        # Required for running kvm on multiple host cores.
        # Uses gem5's parallel event queue feature
        # Note: The simulator is quite picky about this number!
        root.sim_quantum = int(1e9) # 1 ms

    # if not allow_listeners:
    #     m5.disableAllListeners()

    # instantiate all of the objects we've created above
    m5.instantiate()

    print("Running the simulation")
    exit_event = m5.simulate()
    print(f"Exit 1: {exit_event.getCause()}")

    # we are at the point right before STREAM is called
    print("Done booting Linux")
    # switching to atomic cpu if argument cpu == atomic
    if cpu_type != 'kvm':
        system.switchCpus(system.cpu, system.timingCpu)
        print("Switch to detailed cpu model")

    print("Before 2 second *************************************** \n")
    m5.stats.reset()
    exit_event = m5.simulate(2000000000000)
    m5.stats.dump()
    
    m5.stats.reset()
    exit_event = m5.simulate(3000000000000)
    m5.stats.dump()

    print('Exiting @ tick {} because {}'
        .format(m5.curTick(), exit_event.getCause()))
# 16777216
# build/X86_MESI_Two_Level/gem5.opt --outdir=results/stream/64MBby2 configsStream/run_stream.py  configsStream/run_stream.py/x86-linux-kernel-4.19.83 configsStream/x86-ubuntu-stream.img timing 8 MESI_Two_Level 16777216 --allow-listeners