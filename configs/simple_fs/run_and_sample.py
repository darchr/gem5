import os
import sys
import time

import m5
from m5.objects import *
from m5.ticks import fromSeconds
from m5.util.convert import toLatency

sys.path.append('configs/common/') # For the next line...
import SimpleOpts

from system import MySystem

SimpleOpts.add_option("--script", default='',
                      help="Script to execute in the simulated system")

SimpleOpts.add_option("--runs_per_observation", default=1, type='int',
                      help="Number of runs per observation point")

SimpleOpts.set_usage("usage: %prog [options] roi_instructions samples=1")

class UnexpectedExit(Exception):
    pass

def fastforward(system, instructions):
    """ Fast forward the simulation by instructions.
        This only works with system.cpu, not other CPUs.
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
    ticks = fromSeconds(toLatency(sim_time))
    abs_ticks = m5.curTick() + ticks

    exit_event = m5.simulate(ticks)
    while exit_event.getCause() != "simulate() limit reached":
        # It's possible (likely) there are a few max insts exit events still
        # scheduled. We should ignore these.
        print "Exited:", exit_event.getCause()
        if not exit_event.getCause().startswith("Max Insts"):
            raise UnexpectedExit(exit_event.getCause())

        exit_event = m5.simulate(abs_ticks - m5.curTick())

def warmupAndRun(system, warmup_time, detailed_warmup_time, sim_time):
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

def simulateROI(system, instructions, samples, measurements):
    from random import randint, sample, seed
    import signal
    import time

    seed()

    executed_insts = system.totalInsts()

    # We probably want to updated the max instructions by subtracting the
    # insts that we are going to execute in the last detailed sim.
    sample_secs = toLatency("10ms") + toLatency("10us") + toLatency("100us")
    # assume 1 IPC per CPU
    instructions -= int(sample_secs * system.clk_domain.clock[0].frequency *
                        len(system.cpu) * 1)

    sample_insts = sample(xrange(executed_insts,
                                 executed_insts + instructions),
                          samples)
    sample_insts.sort()

    pids = []

    def handler(signum, frame):
        assert(signum == signal.SIGCHLD)
        try:
            while 1:
                pid,status = os.wait()
                if status != 0:
                    print "pid", pid, "failed!"
                    sys.exit(status)
                pids.remove(pid)
        except OSError:
            pass

    signal.signal(signal.SIGCHLD, handler)

    for i, insts in enumerate(sample_insts):
        print "Fast forwarding to sample", i, "stopping at", insts
        insts_past = fastforward(system, insts - system.totalInsts())
        if insts_past/insts > 0.01:
            print "WARNING: Went past the goal instructions by too much!"
            print "Goal: %d, Actual: %d" % (insts, system.totalInsts())

        for j in range(measurements):
            # Max of 4 gem5 instances
            while len(pids) >= 4:
                time.sleep(1)

            pid = m5.fork('%(parent)s/'+str(insts)+'.'+str(j))
            if pid == 0: # in child
                from m5.internal.core import seedRandom
                # Make sure each instance of gem5 starts with a different
                # random seed. Can't just use time, since this may occur
                # multiple times in the same second. 100 choose arbitrarily
                rseed = int(time.time()) * os.getpid()
                seedRandom(rseed)
                print "Running detailed simulation for sample", i, j,
                print "seed", rseed
                warmupAndRun(system, "10ms", "10us", "500us")
                print "Done with detailed simulation for sample", i, j
                sys.exit(0)
            else:
                pids.append(pid)

    print "Waiting for children...", pids
    while pids:
        time.sleep(1)

if __name__ == "__m5_main__":
    (opts, args) = SimpleOpts.parse_args()

    # create the system we are going to simulate
    system = MySystem(opts)

    if not (len(args) == 1 or len(args) == 2):
        SimpleOpts.print_help()
        fatal("Simulate script requires one or two arguments")

    roiInstructions = int(args[0])

    if len(args) == 2:
        samples = int(args[1])
    else:
        samples = 1

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
            simulateROI(system, roiInstructions, samples,
                        opts.runs_per_observation)
        elif exit_event.getCause() == "work items exit count reached":
            end_tick = m5.curTick()

        print "Continuing"
        exit_event = m5.simulate()

    print
    print "Performance statistics"

    print "Ran a total of", m5.curTick()/1e12, "simulated seconds"
    print "Total wallclock time: %.2fs, %.2f min" % \
                (time.time()-globalStart, (time.time()-globalStart)/60)


