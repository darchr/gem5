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
from m5.ticks import fromSeconds
from m5.util.convert import toLatency
from m5.objects import *

sys.path.append('configs/common/') # For the next line...
import SimpleOpts

from system import MySystem

jump_time = '30s'
warmup_insts = int(200e6) #6-8 minutes
detailed_warmup_insts = int(10e6) #~1 minute.
simulation_insts = int(100e6)

def finish(retval=7):
    print
    print "Performance statistics"

    print "Ran a total of", m5.curTick()/1e12, "simulated seconds"
    print "Total wallclock time: %.2fs, %.2f min" % \
                (time.time()-globalStart, (time.time()-globalStart)/60)

    if len(args) == 1:
        from spec import rm_script
        rm_script()

    from sys import exit
    exit(retval)

pids = []
# Signal handler for when the processes exit. This is how we know when
# to remove the child from the list of pids.
def handler(signum, frame):
    import os
    import signal
    assert(signum == signal.SIGCHLD)
    try:
        while 1:
            pid,status = os.wait()
            if status != 0:
                print "pid", pid, "failed!"
                sys.exit(status)
            global pids
            pids.remove(pid)
    except OSError:
        pass

import signal
# install the signal handler
signal.signal(signal.SIGCHLD, handler)

def runTestBase(system):
    pid = m5.fork('%(parent)s/baseline')
    if pid == 0: # in child
        system.switchCpus(system.atomicCpu, system.timingCpuBase)

        system.timingCpuBase[0].scheduleInstStop(0, detailed_warmup_insts,
                                                 "Max Insts")
        print "Detailed warmup simulation"
        exit_event = m5.simulate()
        if exit_event.getCause() != "Max Insts":
            print "Exited because", exit_event.getCause()
            print "giving up"
            finish(8)

        m5.stats.reset()
        system.timingCpuBase[0].scheduleInstStop(0, simulation_insts,
                                                 "Max Insts")
        print "Running real simulation for baseline"
        exit_event = m5.simulate()
        if exit_event.getCause() != "Max Insts":
            print "Exited because", exit_event.getCause()
            print "giving up"
            finish(8)
        sys.exit(0)
    else:
        global pids
        pids.append(pid)

def runTestNoSpec(system):
    pid = m5.fork('%(parent)s/no_speculation')
    if pid == 0: # in child
        system.switchCpus(system.atomicCpu, system.timingCpuNoSpec)

        system.timingCpuNoSpec[0].scheduleInstStop(0, detailed_warmup_insts,
                                                 "Max Insts")
        print "Detailed warmup simulation"
        exit_event = m5.simulate()
        if exit_event.getCause() != "Max Insts":
            print "Exited because", exit_event.getCause()
            print "giving up"
            finish(8)

        m5.stats.reset()
        system.timingCpuNoSpec[0].scheduleInstStop(0, simulation_insts,
                                                 "Max Insts")
        print "Running real simulation for no spec"
        exit_event = m5.simulate()
        if exit_event.getCause() != "Max Insts":
            print "Exited because", exit_event.getCause()
            print "giving up"
            finish(8)
        sys.exit(0)
    else:
        global pids
        pids.append(pid)

def runTestNoSpecLoad(system):
    pid = m5.fork('%(parent)s/no_load_speculation')
    if pid == 0: # in child
        system.switchCpus(system.atomicCpu, system.timingCpuNoLoad)

        system.timingCpuNoLoad[0].scheduleInstStop(0, detailed_warmup_insts,
                                                 "Max Insts")
        print "Detailed warmup simulation"
        exit_event = m5.simulate()
        if exit_event.getCause() != "Max Insts":
            print "Exited because", exit_event.getCause()
            print "giving up"
            finish(8)

        m5.stats.reset()
        system.timingCpuNoLoad[0].scheduleInstStop(0, simulation_insts,
                                                 "Max Insts")
        print "Running real simulation for no spec loads"
        exit_event = m5.simulate()
        if exit_event.getCause() != "Max Insts":
            print "Exited because", exit_event.getCause()
            print "giving up"
            finish(8)
        sys.exit(0)
    else:
        global pids
        pids.append(pid)

def runTestSLB(system):
    pid = m5.fork('%(parent)s/slb')
    if pid == 0: # in child
        system.switchCpus(system.atomicCpu, system.timingCpuSLB)

        system.timingCpuSLB[0].scheduleInstStop(0, detailed_warmup_insts,
                                                 "Max Insts")
        print "Detailed warmup simulation"
        exit_event = m5.simulate()
        if exit_event.getCause() != "Max Insts":
            print "Exited because", exit_event.getCause()
            print "giving up"
            finish(8)

        m5.stats.reset()
        system.timingCpuSLB[0].scheduleInstStop(0, simulation_insts,
                                                 "Max Insts")
        print "Running real simulation for SLB"
        exit_event = m5.simulate()
        if exit_event.getCause() != "Max Insts":
            print "Exited because", exit_event.getCause()
            print "giving up"
            finish(8)
        sys.exit(0)
    else:
        global pids
        pids.append(pid)

if __name__ == "__m5_main__":
    (opts, args) = SimpleOpts.parse_args()

    # create the system we are going to simulate
    system = MySystem(opts)

    # For workitems to work correctly
    # This will cause the simulator to exit simulation when the first work
    # item is reached and when the first work item is finished.
    system.work_begin_exit_count = 1
    system.work_end_exit_count = 1

    if len(args) == 1:
        workload = args[0]
        from spec import make_script
        filename = make_script(workload)

        # Read in the script file passed in via an option.
        # This file gets read and executed by the simulated system after boot.
        # Note: The disk image needs to be configured to do this.
        system.readfile = filename

    # set up the root SimObject and start the simulation
    root = Root(full_system = True, system = system,)

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

    print "Running the simulation"
    exit_event = m5.simulate()

    # The script should call exit
    if exit_event.getCause() != "m5_exit instruction encountered":
        print "Exited because", exit_event.getCause()
        print "giving up"
        finish()

    # Run for 30 seconds into the application
    ticks = fromSeconds(toLatency(jump_time))
    abs_ticks = m5.curTick() + ticks
    print "Jumping into workload"
    exit_event = m5.simulate(ticks)

    if exit_event.getCause() != "simulate() limit reached":
        print "Exited because", exit_event.getCause()
        print "giving up"
        finish()

    # Switch to atomic to warm up
    system.switchCpus(system.cpu, system.atomicCpu)

    system.atomicCpu[0].scheduleInstStop(0, warmup_insts, "Max Insts")
    print "Warming up"
    exit_event = m5.simulate()

    if exit_event.getCause() != "Max Insts":
        print "Exited because", exit_event.getCause()
        print "giving up"
        finish()

    m5.stats.dump()
    runTestBase(system)
    runTestNoSpec(system)
    runTestNoSpecLoad(system)
    runTestSLB(system)

    from time import sleep
    print "Waiting for children"
    while pids:
        sleep(1)

    finish(0)
