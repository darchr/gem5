# -*- coding: utf-8 -*-
# Copyright (c) 2015 Jason Power
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

""" This file creates a barebones system and executes 'hello', a simple Hello
World application.
See Part 1, Chapter 2: Creating a simple configuration script in the
learning_gem5 book for more information about this script.

IMPORTANT: If you modify this file, it's likely that the Learning gem5 book
           also needs to be updated. For now, email Jason <power.jg@gmail.com>

"""

from __future__ import print_function
from __future__ import absolute_import


# import the m5 (gem5) library created when gem5 is built
import m5
# import all of the SimObjects
from m5.objects import *
from gem5.isas import ISA
from gem5.runtime import get_runtime_isa

SMT_test = True
# create the system we are going to simulate
system = System(multi_thread = SMT_test)

# Set the clock fequency of the system (and all of its children)
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

# Set up the system
system.mem_mode = 'timing'               # Use timing accesses
system.mem_ranges = [AddrRange('512MB')] # Create an address range

# Create a simple CPU
if SMT_test:
    system.cpu = TimingSimpleCPU(numThreads = 2)
else:
    system.cpu = TimingSimpleCPU()

# Create a memory bus, a system crossbar, in this case
system.membus = SystemXBar()

# Hook the CPU ports up to the membus
system.cpu.icache_port = system.membus.slave
system.cpu.dcache_port = system.membus.slave

# create the interrupt controller for the CPU and connect to the membus
system.cpu.createInterruptController()

# For x86 only, make sure the interrupts are connected to the memory
# Note: these are directly connected to the memory bus and are not cached

if get_runtime_isa() == ISA.X86:
    for i in range(system.cpu.numThreads):
        system.cpu.interrupts[i].pio = system.membus.mem_side_ports
        system.cpu.interrupts[i].int_requestor = system.membus.cpu_side_ports
        system.cpu.interrupts[i].int_responder = system.membus.mem_side_ports

# Create a DDR3 memory controller and connect it to the membus
system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports

# Connect the system up to the membus
system.system_port = system.membus.slave

# get ISA for the binary to run.
isa = get_runtime_isa()

# Default to running 'hello', use the compiled ISA to find the binary
# grab the specific path to the binary
thispath = os.path.dirname(os.path.realpath(__file__))
#binary1 = os.path.join(thispath, 'tests/test-progs/hello/bin/x86/linux/hello')
#binary2 = os.path.join(thispath, 'tests/test-progs/hello/bin/x86/linux/hello')

binary1 = os.path.join(thispath, './',
                      'tests/test-progs/hello/bin/', isa.name.lower(), 'linux/hello')
binary2 = os.path.join(thispath, './',
                      'tests/test-progs/hello/bin/', isa.name.lower(), 'linux/hello')

#binary3 = os.path.join(thispath,'../../../',
#                      'tests/test-progs/threads/bin/x86/linux/threads')

system.workload = SEWorkload.init_compatible(binary1)
system.workload2 = SEWorkload.init_compatible(binary2)

#system.workload = SEWorkload.init_compatible(binary1)

# Create a process for a simple "Hello World" application
process1 = Process(pid=1000)
process2 = Process(pid=1001)

# Set the command
# cmd is a list which begins with the executable (like argv)
process1.cmd = [binary1]
process2.cmd = [binary2]
#process1.cmd = [binary3]

# Set the cpu to use the process as its workload and create thread contexts
if SMT_test:
    system.cpu.workload = [process1, process2]
    #system.cpu.workload = process1
else:
    system.cpu.workload = process1

system.cpu.createThreads()

# set up the root SimObject and start the simulation
root = Root(full_system = False, system = system)
# instantiate all of the objects we've created above
m5.instantiate()

print("Beginning simulation!")
exit_event = m5.simulate()
print('Exiting @ tick %i because %s' % (m5.curTick(), exit_event.getCause()))
