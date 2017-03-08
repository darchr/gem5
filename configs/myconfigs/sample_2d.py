# -*- coding: utf-8 -*-
# Copyright (c) 2016 Jason Lowe-Power
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
#
# Authors: Jason Lowe-Power, Brian Coutinho

""" Statistical Sampling Library Example:

        This script gives an example of using the sampling library
    for a multi-threaded application. It runs in full system mode
    and could virtually support any multi-threaded application suite.

    -------------
    Background:

    The idea behind statistical sampling is to estimate the
    performance of a system by simulating small portions of the
    application in detail (sample points). The results can then be
    condensed to give a statistically bound estimate.
    For a mulit-threaded program it is not sufficient to simulate
    a sample only once because minute timing differences could lead
    to different execution paths and a large performance difference.
    Instead, we simulate the same sample multiple times with
    small random pertubations to cover most possible interleavings.

    -------------
    Working:

    Internally, the sampling library first selects a set of random
    sample points from the ROI of the application. A sample point
    is defined by an instruction count i.e. total instructions
    executed untill the start of the sample. It then fast-forwards
    to each sample point, warms up the caches and does multiple
    simulations at that point. It continues this process till the
    last sample is simulated.

    The actual process of sampling is abstracted by the sampleROI()
    function used here. Other example scripts in this dirctory
    use basic functions that allow more fine grained control

    -------------
    Usage:

    This script takes three arguments-
        1. The total number of instructions in the ROI
            (which you can find by using runkvm.py)
        2. The number of sample points you want to take in the ROI
        3. The number of runs i.e simulations at each sample point

    -------------
    Output:

    This script's output is a set of sub-directories, one per
    sample point, named by the instructioni count for the sample point.
    Each sample directory is further sub-divided into one directory
    for each simulation at this point. These are named numerically
    by the simulation number such as 0/, 1/, 2/

    For example, with 4 samples and 5 runs the output could look like
    |
    --49349553615/  --371088644479/  --192271861574/  --1049777390582/
      |- 0/           |- 0/            |- 0/            |- 0/
      |- 1/           |- 1/            |- 1/            |- 1/
      |- 2/           |- 2/            |- 2/            |- 2/
      |- 3/           |- 3/            |- 3/            |- 3/
      |- 4/           |- 4/            |- 4/            |- 4/

"""

import os
import sys
import time

import m5
from m5.objects import *
from m5.ticks import fromSeconds
from m5.util.convert import toLatency

sys.path.append('configs/common/') # For the next line...
import SimpleOpts

# Sampling library
from sampling import sampleROI, checkSystem

# ----------------------------------------------------------------
#    This is an example system object. You could replace this with
#  the system you want to simulate.
#  This script only expects that the system contains three
#  cpu object instances named as follows
#        system.cpu : should be a list of KVM cpu type to enable
#                     fast-forwarding
#  system.atomicCpu : should be a matching sized list of the
#                     AtomicSimpleCPU type for warming up caches.
#  system.timingCPU : should be a list of instances of the detailed
#                     cpu model you wish to measure in your system.
#  NOTE: the system should also have a source of non-determinism
#        such a random delay injected in memory acceses
#  See system/system.py for details our example system used here
#
#  The example system can easily be built on to suit a different
#  system configuration, you can also use configure it with
#  command line arguments  (run with --help to see available options)
from system import MySystem
# ----------------------------------------------------------------

SimpleOpts.add_option("--script", default='',
                      help="Script to execute in the simulated system")
SimpleOpts.set_usage("usage: %prog [options] roi_instructions "\
                      "samples=1 runs=1")



if __name__ == "__m5_main__":
    (opts, args) = SimpleOpts.parse_args()

    # create the system we are going to simulate
    system = MySystem(opts)

    # Check if the system is compatible with sampling library
    checkSystem(system)

    if not (len(args) == 1 or len(args) == 2 or len(args) == 3):
        SimpleOpts.print_help()
        fatal("Simulate script requires one or two or three arguments")

    roiInstructions = int(args[0])

    if len(args) == 2 or len(args) == 3:
        samples = int(args[1])
    else:
        samples = 1

    if len(args) == 3:
        runs = int(args[2])
    else:
        runs = 1

    print "Arguments given:  roi_instructions = %d ," % roiInstructions, \
          "samples = %d , runs = %d" % (samples, runs)

    # For workitems to work correctly
    # This will cause the simulator to exit simulation when the first work
    # item is reached and when the first work item is finished.
    system.work_begin_exit_count = 1
    system.work_end_exit_count = 1

    # Read in the script file passed in via an option.
    # This file gets read and executed by the simulated system after boot.
    # Note: The disk image needs to be configured to do this.
    system.readfile = opts.script

    # set up the root SimObject and start the simulation
    root = Root(full_system = True, system = system)

    if system.getHostParallel():
        # Required for running kvm on multiple host cores.
        # Uses gem5's parallel event queue feature
        # Note: The simulator is quite picky about this number!
        root.sim_quantum = int(1e9) # 1 ms

    # Disable the gdb ports. Required for high core counts and forking.
    m5.disableAllListeners()

    # instantiate all of the objects we've created above
    m5.instantiate()

    globalStart = time.time()

    # Keep running until we are done.
    print "Running the simulation"
    exit_event = m5.simulate()
    while exit_event.getCause() != "m5_exit instruction encountered":
        if exit_event.getCause() == "user interrupt received":
            print "User interrupt. Exiting"
            break
        print "Exited because", exit_event.getCause()

        if exit_event.getCause() == "work started count reach":

            # sampleROI - The main sampling function!
            #  sampleROI randomly picks points in the ROI
            #  that are run in detailed mode for measurements.
            #  Each sample point is simulated multiple times
            #  to adequately cover non-determinism in the system
            sampleROI(system, opts, roiInstructions, samples, runs)

        elif exit_event.getCause() == "work items exit count reached":
            end_tick = m5.curTick()

        print "Continuing"
        exit_event = m5.simulate()

    print
    print "Performance statistics"

    print "Ran a total of", m5.curTick()/1e12, "simulated seconds"
    print "Total wallclock time: %.2fs, %.2f min" % \
                (time.time()-globalStart, (time.time()-globalStart)/60)
