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

    # Keep running until we are done.
    print "Running the simulation"
    exit_event = m5.simulate()
    while exit_event.getCause() != "m5_exit instruction encountered":
        if exit_event.getCause() == "user interrupt received":
            print "User interrupt. Exiting"
            break
        print "Exited because", exit_event.getCause()

        if exit_event.getCause() == "work started count reach":
            start_tick = m5.curTick()
            start_insts = system.totalInsts()
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
    print "Simulated time in ROI: %.2fs" % ((end_tick-start_tick)/1e12)
    print "Instructions executed in ROI: %d" % ((end_insts-start_insts))

    import resource
    print
    print "Max memory usage",
    print resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
