# import the m5 (gem5) library created when gem5 is built
import m5
# import all of the SimObjects
from m5.objects import *

m5.util.addToPath('../')
from system import *

import time
import signal
import sys
import os

# create the system we are going to simulate
system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('512MB')]

system.nocaches = Config(system, NoCaches, '2GHz')
system.twolevel = Config(system, TwoLevel, freq = '2GHz', switched_out = True)
system.twolevelfast = Config(system, TwoLevel, freq = '0.5GHz',
                             switched_out = True)

# get ISA for the binary to run.
isa = str(m5.defines.buildEnv['TARGET_ISA']).lower()

# Run 'hello' and use the compiled ISA to find the binary
binary = 'tests/test-progs/hello/bin/' + isa + '/linux/hello'

# Create a process for a simple "Hello World" application
process = Process()
# Set the command
# cmd is a list which begins with the executable (like argv)
process.cmd = [binary]

system.nocaches.setWorkload(process)
system.twolevel.setWorkload(process)
system.twolevelfast.setWorkload(process)

# Think about whether this makes sense!
system.nocaches.setFutureConfig(system.twolevelfast)
system.twolevel.setFutureConfig(system.twolevelfast)

m5.disableAllListeners()

# set up the root SimObject and start the simulation
root = Root(full_system = False, system = system)
# instantiate all of the objects we've created above
m5.instantiate()

print "Beginning simulation!"
exit_event = m5.simulate(int(326250500/2))
print 'Exiting @ tick %i because %s' % (m5.curTick(), exit_event.getCause())

# Need to ensure that there is no data in the caches of the configuration we're
# switching from!
m5.memWriteback(system.nocaches)
m5.memInvalidate(system.nocaches)

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
            pids.remove(pid)
    except OSError:
        pass


# install the signal handler
signal.signal(signal.SIGCHLD, handler)

for config in [system.twolevel, system.twolevelfast]:

    pid = m5.fork()
    if pid == 0:
        # in child
        print "Switching to twolevel!"
        m5.switchCpus(system, [(system.nocaches.cpu, config.cpu)])
        print "Switched!"

        print "Beginning simulation again***********************************"
        exit_event = m5.simulate()
        print 'Exiting because %s' % (exit_event.getCause())
        print "Time: %i" % (m5.curTick())
        sys.exit(0)
    else:
        pids.append(pid)
        while pids:
            time.sleep(1)
