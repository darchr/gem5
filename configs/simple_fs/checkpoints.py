import os
import sys
import time

from random import uniform

import m5
import m5.ticks
from m5.util.convert import toLatency
from m5.ticks import fromSeconds
from m5.objects import *

sys.path.append('configs/common/') # For the next line...
import SimpleOpts

from system import MySystem

sys.path.append('/p/multifacet/users/powerjg/supernuma/workloads')
from utils import *

SimpleOpts.add_option("--max_cpt_interval", default=1.0, type="float",
                      help="Max time between checkpoints in seconds")
SimpleOpts.add_option("--min_cpt_interval", default=0.0, type="float",
                      help="Min time between checkpoints in seconds")

SimpleOpts.set_usage("usage: %prog [options] workload")

if __name__ == "__m5_main__":
    (opts, args) = SimpleOpts.parse_args()

    if len(args) != 1:
        SimpleOpts.print_help()
        fatal("Checkpointing script requires exactly one argument")

    if args[0].find('.') == -1:
        fatal("Workload must be in the form suite.app")

    suite, app = args[0].split('.')

    dirlist = m5.options.outdir.split('/')
    if dirlist[-1] != app or dirlist[-2] != suite:
        if m5.options.outdir == 'm5out':
            m5.util.warn("Output directory not changed! Maybe overwriting")
        else:
            fatal("Bad outdir specified. Be sure to use ...../suite/app")

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
    system.readfile = getRunscript(suite, app)

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

    checkpoint_times = []

    exit_event = m5.simulate()
    if exit_event.getCause() != "work started count reach":
        print "'%s'" % (exit_event.getCause())
        print "Unexpected exit:", exit_event.getCause()
    else:
        print "Starting to execute the work item"
        start_tick = m5.curTick()
        checkpoint_num = 0
        while exit_event.getCause() != "work items exit count reached":
            interval = uniform(opts.min_cpt_interval, opts.max_cpt_interval)
            print "Running for", interval, "seconds: ",
            print fromSeconds(interval), "ticks."
            exit_event = m5.simulate(fromSeconds(interval))

            if exit_event.getCause() != "simulate() limit reached":
                print "Unexpected exit:", exit_event.getCause()
                break

            start = time.time()
            print "Checkpointing. Current tick", m5.curTick()
            cptdir = os.path.join(m5.options.outdir,
                                  "cpt."+str(checkpoint_num))
            m5.checkpoint(cptdir)
            checkpoint_times.append(time.time()-start)
            checkpoint_num += 1

        print "Finished the work item"
        end_tick = m5.curTick()

    # Keep running until we are done.
    print "Running the rest of the simulation"
    exit_event = m5.simulate()
    while exit_event.getCause() != "m5_exit instruction encountered":
        if exit_event.getCause() == "user interrupt received":
            print "User interrupt. Exiting"
            break
        print "Exited because", exit_event.getCause()

        print "Continuing"
        exit_event = m5.simulate()

    print
    print "Performance statistics"

    print "Ran a total of", m5.curTick()/1e12, "simulated seconds"
    print "Total wallclock time: %.2fs, %.2f min" % \
                (time.time()-globalStart, (time.time()-globalStart)/60)
    if checkpoint_times:
        cpt_avg = sum(checkpoint_times)/len(checkpoint_times)
        print "Checkpointing took %.fs on average" % cpt_avg
    print "Simulated time in RIO: %.2fs" % ((end_tick-start_tick)/1e12)

    import resource
    print
    print "Max memory usage",
    print resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
