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
# Authors: Brian Coutinho, Keshav Mathur, Jason Lowe-Power

""" Statistical Sampling library for gem5:

    Functions that perform most of the basic simulation and
    fast-forward for sampling
"""

import os
import sys

import m5
from m5.objects import *
from m5.ticks import fromSeconds
from m5.util.convert import toLatency

class UnexpectedExit(Exception):
    pass

def fastforward(system, instructions):
    """ Fast forward the simulation by instructions.
        This only works with system.cpu (kvm CPU), not other CPUs.
        @param instructions the number of instructions to simulate before
               exit. This is the total instructions across all cores.
        @return The number of instructions past the goal
    """
    goal = system.totalInsts() + instructions

    for i,cpu in enumerate(system.cpu):
        if not hasattr(cpu, "_outstandingExit"):
            cpu._outstandingExit = False
        if not cpu._outstandingExit:
            exit_inst = int(instructions/len(system.cpu))
            cpu.scheduleInstStop(0, exit_inst, "Max Insts CPU %d" % (i))
            cpu._outstandingExit = True

    # There isn't a way to stop when we hit a specific global number of insts
    # Therefore, we have to exit after each CPU executes X insts. When the
    # first few CPUs exit, we won't have executed enough global insts, so
    # keep going until we do.
    while 1:
        exit_event = m5.simulate()

        if not exit_event.getCause().startswith("Max Insts"):
            raise UnexpectedExit(exit_event.getCause())

        cpu_id = int(exit_event.getCause().split(' ')[-1])
        system.cpu[cpu_id]._outstandingExit = False

        if system.totalInsts() > goal:
            return system.totalInsts() - goal

        exit_inst = min(int(instructions/len(system.cpu)),
                        goal-system.totalInsts())
        system.cpu[cpu_id].scheduleInstStop(0, exit_inst,
                                            "Max Insts CPU %d" % (cpu_id))
        system.cpu[cpu_id]._outstandingExit = True


def runSim(sim_time):
    """ Run the simulation.
        This will run the simulation ignoring exits for "Max Insts", but
        raising an exception for all other exit types.
        @param sim_time the amount of guest-time to simulate as a string
               (e.g., "10ms")
    """
    ticks = fromSeconds(toLatency(sim_time))
    abs_ticks = m5.curTick() + ticks

    exit_event = m5.simulate(ticks)
    while exit_event.getCause() != "simulate() limit reached":
        # It's possible (likely) there are a few max insts exit events still
        # scheduled. We should ignore these. See fastforward
        print "Exited:", exit_event.getCause()
        if not exit_event.getCause().startswith("Max Insts"):
            raise UnexpectedExit(exit_event.getCause())

        exit_event = m5.simulate(abs_ticks - m5.curTick())


def funcWarmup(system, warmup_time):
    """ Only performs functional warmup and returns
        Assumes we are in KVM cpu mode
        Return value: warmup_timing ie wall clocktime of warmup
    """
    print "Warming up"
    system.switchCpus(system.cpu, system.atomicCpu)
    warmup_timing = runSim(warmup_time)
    return warmup_timing


def detailedWarmup(system, detailed_warmup_time):
    """ Perform detailed warmup for uarch structures
        Return value: warmup_timing ie wall clocktime of warmup
    """
    print "Detailed warmup"
    system.switchCpus(system.atomicCpu, system.timingCpu)
    detailed_warmup_timing = runSim(detailed_warmup_time)
    return detailed_warmup_timing


def warmupAndRun(system, warmup_time, detailed_warmup_time, sim_time):
    """ Warmup and run the detailed simulation
        This function first executes the system with the atomic CPU to warmup
        the caches and branch predictors. Next, it runs in detailed warmup
        mode with the detailed CPU to warmup the pipeline. Once the system is
        warm, we reset the stats and start the detailed simulation.
        NOTE: This function assumes the KVM CPU is currently active
              (system.cpu). After this function the detailed CPU (timingCPU)
              is active.

        @param system the system we are running
        @param warmup_time amount of guest-time to warmup
        @param detailed_warmup_time amout of guest-time for detailed warming
        @param sim_time amount of guest-time for detailed simulation
    """
    print "Warming up"
    system.switchCpus(system.cpu, system.atomicCpu)
    runSim(warmup_time)
    print "Detailed warmup"
    system.switchCpus(system.atomicCpu, system.timingCpu)
    runSim(detailed_warmup_time)
    print "Running the detailed simulation!"
    # Reset the stats so we only record things in the detailed simulation
    m5.stats.reset()
    runSim(sim_time)
