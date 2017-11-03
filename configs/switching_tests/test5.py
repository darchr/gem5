
# Tests the simple timing CPU.
# "Will hop swap" CPU 2 for CPU.
# Start running with CPU, unplug CPU, plug in CPU 2, finish simulating.

import m5
from m5.objects import *

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('512MB')]

system.cpu = TimingSimpleCPU(cpu_id = 0)
system.cpu2 = TimingSimpleCPU()

system.membus = SystemXBar()

system.cpu.icache_port = system.membus.slave
system.cpu.dcache_port = system.membus.slave

# create the interrupt controller for the CPU and connect to the membus
system.cpu.createInterruptController()
if m5.defines.buildEnv['TARGET_ISA'] == "x86":
    system.cpu.interrupts[0].pio = system.membus.master
    system.cpu.interrupts[0].int_master = system.membus.slave
    system.cpu.interrupts[0].int_slave = system.membus.master

# Create a DDR3 memory controller and connect it to the membus
system.mem_ctrl = DDR3_1600_8x8()
system.mem_ctrl.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.master

# Connect the system up to the membus
system.system_port = system.membus.slave

# Create a process for a simple "Hello World" application
process = Process()
# Set the command
isa = str(m5.defines.buildEnv['TARGET_ISA']).lower()
process.cmd = ['tests/test-progs/hello/bin/' + isa + '/linux/hello']
# Set the cpu to use the process as its workload and create thread contexts
system.cpu.workload = process
system.cpu.createThreads()

system.cpu.willHotSwap(system.cpu2)

# set up the root SimObject and start the simulation
root = Root(full_system = False, system = system)
# instantiate all of the objects we've created above
m5.instantiate()
print "successfully initialized"

m5.simulate(100000000)
print "successfully simulated with CPU"

m5.drain()

system.cpu.unplug()
print "successfully disconnected the CPU"

system.cpu2.plugIn(system.cpu)
print "successfully reconnected the CPU"

m5.simulate()
print "successfully simulated with CPU2"
