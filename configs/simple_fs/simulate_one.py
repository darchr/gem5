import os
import sys
import time

from random import uniform

import m5
import m5.ticks
from m5.util.convert import toLatency
from m5.ticks import fromSeconds
from m5.objects import *

basedir = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                       '../../../')

sys.path.append(os.path.join(basedir, 'gem5/configs/common/'))
import SimpleOpts

from system import MySystem

sys.path.append(os.path.join(basedir, 'workloads'))
from utils import *

SimpleOpts.set_usage("usage: %prog [options] workload checkpoint_num")

SimpleOpts.add_option("--warmup_time", default="10ms",
                      help="Simulation time to warmup")
SimpleOpts.add_option("--detailed_warmup_time", default="10us",
                      help="Simulation time to warmup")
SimpleOpts.add_option("--detailed_time", default="1ms",
                      help="Simulation time to perform detailed simulation")

SimpleOpts.add_option("--test_output", default=False, action="store_true",
                      help="Testing. Ignore the weird output directory.")


if __name__ == "__m5_main__":
    (opts, args) = SimpleOpts.parse_args()

    # Since we are NOT using kvm
    opts.no_host_parallel = True

    if len(args) != 2:
        SimpleOpts.print_help()
        fatal("Simulate script requires exactly two arguments")

    if args[0].find('.') == -1:
        fatal("Workload must be in the form suite.app")

    if int(args[1]) < 0:
        fatal("Checkpoint num must be a positive integer")

    suite, app = args[0].split('.')
    cpt_num = int(args[1])

    cpt_dir = os.path.join(basedir, 'checkpoints', suite, app,
                           'cpt.'+str(cpt_num))


    if m5.options.outdir == 'm5out' or opts.test_output:
        print "Warning: Output directory not changed! Maybe overwriting"
    else:
        dirlist = m5.options.outdir.split('/')
        if dirlist[-2] != app or dirlist[-3] != suite:
            fatal("Bad outdir specified. Be sure to use .../suite/app/<num>")

    # create the system we are going to simulate
    system = MySystem(opts, no_kvm=True)

    # For workitems to work correctly
    # This will cause the simulator to exit simulation when the first work
    # item is reached and when the first work item is finished.
    system.work_begin_exit_count = 1
    system.work_end_exit_count = 1

    # set up the root SimObject and start the simulation
    root = Root(full_system = True, system = system)

    # Disable the gdb ports. Required for high core counts.
    m5.disableAllListeners()

    print "Starting from checkpoint in", cpt_dir

    # instantiate all of the objects created above and load the checkpoint
    m5.instantiate(ckpt_dir = cpt_dir)

    # Use the atomic CPU for warmup
    #system.switchCpus(system.cpu, system.atomicCpu)

    global_start = time.time()
    start_ticks = m5.curTick()

    print "Warming up the caches"

    exit_event = m5.simulate(fromSeconds(toLatency(opts.warmup_time)))
    if exit_event.getCause() != "simulate() limit reached":
        print "Exited during warmup because", exit_event.getCause()
        exit(1)

    warmup_timing = time.time() - global_start
    warmup_ticks = m5.curTick() - start_ticks

    # Switch to the detailed CPU for timing
    system.switchCpus(system.cpu, system.timingCpu)

    print "Warming up the OOO CPU microarchitecture"

    detailed_warmup_time = fromSeconds(toLatency(opts.detailed_warmup_time))
    exit_event = m5.simulate(detailed_warmup_time)
    if exit_event.getCause() != "simulate() limit reached":
        print "Exited during simulation because", exit_event.getCause()
        exit(1)

    detailed_warmup_timing = time.time() - global_start - warmup_timing
    detailed_warmup_ticks = m5.curTick() - start_ticks - warmup_ticks

    # Reset the stats so we only record things in the detailed simulation
    m5.stats.reset()

    print "Running the detailed simulation"

    exit_event = m5.simulate(fromSeconds(toLatency(opts.detailed_time)))
    if exit_event.getCause() != "simulate() limit reached":
        print "Exited during simulation because", exit_event.getCause()
        exit(1)

    detailed_timing = time.time() - global_start
    detailed_timing -= warmup_timing - detailed_warmup_timing
    detailed_ticks = m5.curTick() - start_ticks
    detailed_ticks-= warmup_ticks - detailed_warmup_ticks

    print
    print "Performance statistics"

    total_time = time.time() - global_start
    print "Total wallclock time: %.2fs, %.2f min" % \
                (total_time, (total_time)/60)

    print "Warmup ran a total of %.2fms simulator and %.2fs wallclock time" \
          % (warmup_ticks/1e9, warmup_timing)

    print "Detailed warmup ran a total of %.2fms simulator" \
           % (detailed_warmup_ticks/1e9),
    print "and %.2fs wallclock time" \
           % (detailed_warmup_timing)

    print "Detail ran a total of %.2fms simulator and %.2fs wallclock time" \
          % (detailed_ticks/1e9, detailed_timing)

    import resource
    print
    print "Max memory usage",
    print resource.getrusage(resource.RUSAGE_SELF).ru_maxrss

    sys.exit(0)
