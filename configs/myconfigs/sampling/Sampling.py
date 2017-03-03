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

""" Statistical Sampling library for gem5 (TARDIS)

    This library provides support for performing statistical sampling
    on multi-threaded workloads.  See the accompanying example script
    files for examples on using this library.

    ----------------------
    What this module expects:
    The system object must have three cpu models named as follows-

          system.cpu : should be a list of KVM cpu type to enable
                       fast-forwarding
    system.atomicCpu : should be a matching sized list of the
                       AtomicSimpleCPU type for warming up caches.
    system.timingCPU : should be a list of instances of the detailed
                       cpu model you wish to measure in your system.

    Secondly, if you simulate a multi-threaded application the system
    should have some component that adds a small amount of non-determinism
    to each simulation. In the example scripts we add a small random delay
    on every last level cache miss to create this effect. The module
    changes the random seed for each simulation at a sample point in order
    to cover all divergent paths.

    See system/system.py for the example system used here
    ----------------------

    Please look at the technical report below for details on how to
    determine sampling paramters and a recommended flowchart for the
    sampling process.
    <TBD>
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


SimpleOpts.add_option("--warmup_time", default="10ms",
                      help="Simulated functional warmup time")
SimpleOpts.add_option("--detailed_warmup_time", default="10us",
                      help="Simulated detailed warmup time")
SimpleOpts.add_option("--detailed_time", default="1ms",
                      help="Simulated time for detailed measurements,"\
                         " (size of measurment window)")

from sim_tools import *


_valid_system = False
sample_insts = []            #-- list storing instructions for sample points
_sample = 0                  #-- current sample index


def generateSamples(system, opts, instructions, samples, \
                    start_insts):
    """ Generates a fixed number of random sample points that can
        be used by other methods of this module.
        The sample instruction counts are loaded internally into
        the sample_insts variable

        @param system the system we are running
        @param opts command line options as an instance of optparse
        @param instructions total instructions in the ROI (summed over all
               CPUs)
        @param samples number of sample points in the ROI
        @param start_insts instructions executed till the start of roi
    """
    from random import randint, sample, seed
    global sample_insts
    seed()

    warmup_time = opts.warmup_time
    detailed_warmup_time = opts.detailed_warmup_time
    detailed_time = opts.detailed_time

    # We probably want to updated the max instructions by subtracting the
    # insts that we are going to execute in the last detailed sim.
    sample_secs = toLatency(warmup_time) + toLatency(detailed_warmup_time) \
                    + toLatency(detailed_time)
    # assume 1 IPC per CPU
    instructions -= int(sample_secs * system.clk_domain.clock[0].frequency *
                        len(system.cpu) * 1)

    # Get the instruction numbers that we want to exit at for our sample
    # points. It needs to be sorted, too.
    sample_insts = sample(xrange(start_insts,
                                 start_insts + instructions),
                          samples)
    sample_insts.sort()



def dumpSamples(outfile="random.samples"):
    """ Dumps the sample points to a file in the gem5 outdir
        so that they can be used accross multiple parallel jobs

        @param outfile name of the sample dump file
    """
    import pickle
    rfilename = "%s/%s" % (m5.options.outdir, outfile)

    with open(rfilename , "w") as rfile:
        pickle.dump(sample_insts, rfile)



def loadSamples(samplefile):
    """ Loads sample points from a file dumped previously
        using dumpSamples().

        @param samplefile name of the sample dump file
    """
    import pickle
    global sample_insts

    if not os.path.isfile(samplefile):
        print >> sys.stderr, "Error could not find file", samplefile
        sys.exit(1)

    with open(samplefile, "r") as rfile:
        sample_insts = pickle.load(rfile)


def makeSampleDir(sampleno):
    """ Make directory for a sample
        Returns the name of the directory
        @param sampleno sample id number """
    insts = sample_insts[sampleno]
    parent = m5.options.outdir
    sampledir = '%(parent)s/'%{'parent':parent} + str(insts)
    if not os.path.isdir(sampledir):
        os.mkdir(sampledir)
    return sampledir


def forwardToSample(system, sample):
    """ Fast forward to a particular sample number.

        @param system the system we are running
        @param sample to forward to
    """
    global _sample
    insts = sample_insts[sample]
    insts_past = fastforward(system, insts - system.totalInsts())
    if insts_past/insts > 0.01:
        print "WARNING: Went past the goal instructions by too much!"
        print "Goal: %d, Actual: %d" % (insts, system.totalInsts())
    _sample = sample



def checkSystem(system):
    """ Do some basic validation of the system such as does it
        have the required objects, uses a KVM cpu etc.

        The system object must have three cpu models named as follows-
              system.cpu : should be a list of KVM cpu type to enable
                           fast-forwarding
        system.atomicCpu : should be a matching sized list of the
                           AtomicSimpleCPU type for warming up caches.
        system.timingCPU : should be a list of instances of the detailed
                           cpu model you wish to measure in your system.

        This also adds two helper functions to the system object
        that are used by the Sampling module - switchCpus() and
        totalInsts()
    """
    for cpulist in ["cpu", "atomicCpu", "timingCpu"]:
        if not (hasattr(system, cpulist) and \
                 isinstance( getattr(system, cpulist), list) ):
            print >> sys.stderr, "System should have three lists of"\
                                 " cpus named- cpu, atomicCpu, timingCpu"
            sys.exit(1)

    if not isinstance(system.cpu[0], BaseKvmCPU):
      print >> sys.stderr, "system.cpu should be of KVM cpu type !"
      sys.exit(1)
    if not isinstance(system.atomicCpu[0], AtomicSimpleCPU):
      print >> sys.stderr, "system.atomicCpu should be of type AtomicSimpleCPU"
      sys.exit(1)

    # system object helper functions
    def switchCpus(self, old, new):
        assert(new[0].switchedOut())
        m5.switchCpus(self, zip(old, new))

    def totalInsts(self):
        return sum([cpu.totalInsts() for cpu in self.cpu]) \
             + sum([cpu.totalInsts() for cpu in self.atomicCpu])

    if not hasattr(system, 'switchCpus'):
        system.switchCpus = classmethod(switchCpus)
    if not hasattr(system, 'totalInsts'):
        system.totalInsts = classmethod(totalInsts)

    _valid_system = True



def Sample(system, opts, runs, warmup=True, startwith=0):
    """ Take measurements at a single sample point.
        Internally, this would involve running mulitple
        simulations starting from the same point with a
        different random seed, in order to cover divergent
        paths. Each simulation runs in its own subdirectory
        simply named as the run number - 0/ , 1/ , 2/ etc.

        Assumes that you have fastforwarded to the sample point.
        On returning the simulator will be using atomicCpu

        @param system the system we are running
        @param opts command line options as an instance of optparse
        @param runs at this sample point
        @param warmup whether to perform functional warmup, one can
               optionally turn this off in cases where additional
               samples are needed
        @param startwith index of the first run directory
               0 by default
    """
    import signal
    import time
    from random import randint

    if not _valid_system:
        checkSystem(system)

    # These are the currently running processes we have forked.
    pids = []

    # Signal handler for when the processes exit. This is how we know when
    # to remove the child from the list of pids.
    def handler(signum, frame):
        assert(signum == signal.SIGCHLD)
        try:
            while 1:
                pid,status = os.wait()
                if status != 0:
                    print "pid", pid, "failed!"
                    sys.exit(status)
                if pid in pids:
                    pids.remove(pid)
        except OSError:
            pass

    # install the signal handler
    signal.signal(signal.SIGCHLD, handler)

    # clone gem5 for every new simulation/measurement
    for r in range(startwith, runs):

        # Max of 4 gem5 instances (-1 for this process). If we have this
        # number of gem5 processes running, we should wait until one finishes
        while len(pids) >= 4 - 1:
            time.sleep(1)

        # Fork gem5 and get a new PID. Save the stats in a folder based on
        # the instruction number and subdir based on run no.
        insts = sample_insts[_sample]
        pid = m5.fork('%(parent)s/'+str(insts)+'/'+str(r))

        if pid == 0: # in child
            from m5.internal.core import seedRandom
            # Make sure each instance of gem5 starts with a different
            # random seed. Can't just use time, since this may occur
            # multiple times in the same second.
            rseed = int(time.time()) * os.getpid() * randint(0,10000)
            seedRandom(rseed)

            # Functional warmup can be kept common among different simulations
            # this spreads the cost over all the runs
            if warmup:
                funcWarmup(system, opts.warmup_time)

            # Detailed warmup has to be done for every new process
            # as gem5 fork drains uarch structures
            detailedWarmup(system, opts.detailed_warmup_time)

            print "Running detailed simulation for sample:", _sample, "run:", r
            # Reset the stats so we only record things in detailed simulation
            m5.stats.reset()
            runSim(opts.detailed_time)

            # Just exit in the child. No need to do anything else.
            sys.exit(0)
        else: # in parent
            # Append the child's PID and fast forward to the next point
            pids.append(pid)

    print "Waiting for children...", pids
    while pids:
        time.sleep(1)
    print "Done with ", runs, " runs at sample ", _sample



def sampleROI(system, opts, instructions, samples, runs):
    """ At a high-level, this function executes the entire ROI and takes
        a number of random sample points. At each point it may run multiple
        simulations.
        This uses gem5 fork support for creating multiple simulations that
        could diverge

        @param system the system we are running
        @param opts command line options as an instance of optparse
        @param instructions total instructions in the ROI (summed over all
               CPUs)
        @param samples number of sample points in the ROI
        @param runs per sample point
    """

    if not _valid_system:
        checkSystem(system)

    start_insts = system.totalInsts()
    generateSamples(system, opts, instructions, samples, start_insts)

    # Here's the magic
    for i, insts in enumerate(sample_insts):
        print "Fast forwarding to sample", i, "stopping at", insts
        forwardToSample(system, i)
        executed_insts = system.totalInsts()
        print "DEBUG: Inst executed so far ", executed_insts

        # Now that we have hit the sample point take multiple observations
        parent = m5.options.outdir
        os.mkdir('%(parent)s/'%{'parent':parent} + str(insts))

        # Take measurements at this sample point
        Sample(system, opts, runs)

        # Switch back to the KVM CPU for next fast-forward
        # system.switchCpus(system.atomicCpu, system.cpu)

