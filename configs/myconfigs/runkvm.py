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
"""

import sys
import time

import m5
import m5.ticks
from m5.objects import *

sys.path.append('configs/common/') # For the next line...
import SimpleOpts

from system import MySystem

SimpleOpts.add_option("--script", default='',
                      help="Script to execute in the simulated system")

if __name__ == "__m5_main__":
    (opts, args) = SimpleOpts.parse_args()

    # create the system we are going to simulate
    system = MySystem(opts)

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

    # instantiate all of the objects we've created above
    m5.instantiate()

    globalStart = time.time()


    print "Running the simulation"
    exit_event = m5.simulate()

    # While there is still something to do in the guest keep executing.
    # This is needed since we exit for the ROI begin/end
    foundROI = False
    while exit_event.getCause() != "m5_exit instruction encountered":
        # If the user pressed ctrl-c on the host, then we really should exit
        if exit_event.getCause() == "user interrupt received":
            print "User interrupt. Exiting"
            break

        print "Exited because", exit_event.getCause()

        if exit_event.getCause() == "work started count reach":
            start_tick = m5.curTick()
            start_insts = system.totalInsts()
            foundROI = True
        elif exit_event.getCause() == "work items exit count reached":
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
        print "Simulated time in ROI: %.2fs" % ((end_tick-start_tick)/1e12)
        print "Instructions executed in ROI: %d" % ((end_insts-start_insts))
