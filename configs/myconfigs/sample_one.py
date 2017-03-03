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

""" Statistical Sampling Library Example - Simulating one sample:

       This scripts fast forwards to a single sample point in the
    ROI and then runs multiple simulations at this sample point.
    This potentially allows you to simulate multiple samples in
    parallel.

    The intent behind this script is to parallelize the sampling
    process rather than running a single full-blown gem5 process
    (as in sample_2d.py). Each sample could run in parallel on
    a large machine or on different machines if a network file
    system is supported.

    It also demonstrates using some of the lower level functions
    in the sampling library.

    -------------
    Example Usage:

        The sampling process is broken down into two steps.
    The first step is to determine at which points the samples
    should be taken (runkvm.py). The second step is to use
    sample_one.py to then run one of the samples.

    Step 1:

    To generate the sample points use runkvm.py with a suitable
    output directory
    For ex, to select 50 sample points use-

    ./build/X86/gem5.opt --outdir examples/streamcluster   \
        configs/myconfigs/runkvm.py  <..options..>  50

    This will create a summary of the chosen sample points in
    the file  examples/streamcluster/random.samples

    Step 2:
    Now, this script could be used to simulate one of the samples
    by specifying the index number of ths sample to simulate.
    (in the above example the sample index could be from 0 to 49)
    Also, set the same output directory as used with run_kvm.py

    For ex, to simulate the sample index '3' with 10 simulations
    ./build/X86/gem5.opt --outdir examples/streamcluster   \
        configs/myconfigs/simulate_one.py  <..options..>  3 10

    -------------
    Usage:

    This script takes two arguments-
        1. The sample index number to simulate.
           (could vary from 0 to samples-1 )
        2. The number of runs i.e simulations at this sample point

    -------------
    Output:

    This script generates a sub-directory for the sample specified
    on the command line. The instruction count of the sample is
    used as the name. This further is sub-divided into one directory
    for each simulation at this sample point.

    For example, if the instruction count for sample 3 is
    49349553615 and the number of simulations is 10,  this would
    generate an output dirctory as -
    --49349553615/
        |- 0/       |- 1/       |- 2/       !- 3/      |- 4/
        |- 5/       |- 6/       |- 7/       !- 8/      |- 9/

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
from sampling import forwardToSample, Sample, makeSampleDir, \
                     checkSystem, loadSamples

# This is an example system object. You could replace this with
# the system you want to simulate.
# See a more detailed explanation in sample_2d.py
from system import MySystem

SimpleOpts.add_option("--script", default='',
                      help="Script to execute in the simulated system")
SimpleOpts.set_usage("usage: %prog [options] sample_index runs=1")


def simulateROI(system, opts, sampleidx, runs):
    """ Fast forward to and simulate a single sample point """
    # Create the sample directory with our friendly method
    makeSampleDir(sampleidx)

    # First forward to the specific sample number
    print "Fast forwarding to sample", sampleidx
    forwardToSample(system, sampleidx)

    # Take runs number of measurements at this point
    Sample(system, opts, runs)
    # ... and we are done!


if __name__ == "__m5_main__":
    (opts, args) = SimpleOpts.parse_args()

    # create the system we are going to simulate
    system = MySystem(opts)

    # Check if the system is compatible with sampling library
    checkSystem(system)

    if not (len(args) == 1 or len(args) == 2):
        SimpleOpts.print_help()
        fatal("Simulate script requires one or two arguments")

    sampleidx = int(args[0])

    if len(args) == 2:
        runs = int(args[1])
    else:
        runs = 1

    print "Arguments given: sample index = %d , runs = %d" % (sampleidx, runs)

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

    # Read the sample points from the output directory
    # Note: this assumes you have a file random.samples
    # generated in the output directory of this gem5 process
    loadSamples(m5.options.outdir + '/random.samples')


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
            simulateROI(system, opts, sampleidx, runs)
            break

        elif exit_event.getCause() == "work items exit count reached":
            end_tick = m5.curTick()

        print "Continuing"
        exit_event = m5.simulate()

    print
    print "Performance statistics"

    print "Ran a total of", m5.curTick()/1e12, "simulated seconds"
    print "Total wallclock time: %.2fs, %.2f min" % \
                (time.time()-globalStart, (time.time()-globalStart)/60)


