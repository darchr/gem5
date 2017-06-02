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
# Authors: Jason Lowe-Power

""" Script to run a workload in KVM with gem5
    When this script is run without the --script parameter, gem5 will boot
    Linux and sit at the command line, assuming this is how your disk is
    configured. With the script parameter, the script will be executed on the
    guest system.

    If your application has ROI annotations, this script will count the total
    number of instructions executed in the ROI. It also tracks how much
    wallclock and simulated time.

    Finally, for applications with an ROI this script also generates
    randomly chosen sample points. This can be used with the sample_one.py
    script to perform simulation on a single sample point.
"""

import sys
import time

import m5
import m5.ticks
from m5.objects import *

sys.path.append('configs/common/') # For the next line...
import SimpleOpts

from sampling import generateSamples, dumpSamples

from create_system import createSystem

SimpleOpts.set_usage("usage: %prog [options] suite.app")

if __name__ == "__m5_main__":
    (opts, args) = SimpleOpts.parse_args()

    if not len(args) == 1:
        SimpleOpts.print_help()
        fatal("Simulate script requires one argument")

    system, rois, roiInstructions = createSystem(args)

    # set up the root SimObject and start the simulation
    root = Root(full_system = True, system = system)

    if system.getHostParallel():
        # Required for running kvm on multiple host cores.
        # Uses gem5's parallel event queue feature
        # Note: The simulator is quite picky about this number!
        root.sim_quantum = int(1e9) # 1 ms

    # Disable the gdb ports. Required for high core counts and forking.
    if args[0] != 'interactive':
        m5.disableAllListeners()

    # instantiate all of the objects we've created above
    m5.instantiate()

    globalStart = time.time()

    print "Running the simulation"
    exit_event = m5.simulate()

    # While there is still something to do in the guest keep executing.
    # This is needed since we exit for the ROI begin/end
    foundROI = 0
    while exit_event.getCause() != "m5_exit instruction encountered":
        # If the user pressed ctrl-c on the host, then we really should exit
        if exit_event.getCause() == "user interrupt received":
            print "User interrupt. Exiting"
            break

        print "Exited because", exit_event.getCause()

        if exit_event.getCause() == "workbegin":
            foundROI += 1
            if foundROI == rois:
                start_tick = m5.curTick()
                start_insts = system.totalInsts()
        elif exit_event.getCause() == "workend":
            end_tick = m5.curTick()
            end_insts = system.totalInsts()

        print "Continuing"
        exit_event = m5.simulate()

    print
    print "Performance statistics"

    print "Ran a total of", m5.curTick()/1e12, "simulated seconds"
    print "Total wallclock time: %.2fs, %.2f min" % \
                (time.time()-globalStart, (time.time()-globalStart)/60)

    if foundROI:
        roiinstructions = end_insts - start_insts
        print "Simulated time in ROI: %.2fs" % ((end_tick-start_tick)/1e12)
        print "Instructions executed in ROI: %d" % ((roiinstructions))
