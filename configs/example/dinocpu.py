# Copyright (c) 2018 The Regents of the University of California
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

""" This file show a simple example of how to use the gem5 verilog interface.
It uses the dinoCPU and loads a simple RISC-V application into memory. This
would be a replacement for the verilator-based emulator that can be generate
d from the dinoCPU verilog.
"""

from __future__ import print_function

# import the m5 (gem5) library created when gem5 is built
import m5
# import all of the SimObjects
from m5.objects import *

# Add the common scripts to our path
m5.util.addToPath('../')

# import the SimpleOpts module
from common import SimpleOpts

# Set the usage message to display
SimpleOpts.set_usage("usage: %prog [options] <binary to load>")

# Finalize the arguments and grab the opts so we can pass it on to our objects
(opts, args) = SimpleOpts.parse_args()

if len(args) == 1:
    binary = args[0]
else:
    SimpleOpts.print_help()
    m5.fatal("Expected a binary to execute as positional argument")

##############################################################################
# CREATE THE SYSTEM
##############################################################################

# create the system we are going to simulate
system = DinoCPUSystem()

# Set the clock frequency of the system (and all of its children)
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

# Set up the system
system.mem_mode = 'atomic'               # Use timing accesses
system.mem_ranges = [AddrRange('4GB')] # Create an address range

# Create a DDR3 memory controller
system.mem_ctrls = [SimpleMemory(latency = '0ns', bandwidth = '0GB/s')]
system.mem_ctrls.range = system.mem_ranges[0]

# Set up the binary to load
system.kernel = binary
system.system_port = system.mem_ctrl.port

# Create the dinocpu verilator wrapper
system.dinocpu = VerilatorDinoCPU()

# Create the mem black box verilator wrapper
system.verilator_mem = DinoCPUCombMemBlackBox()


system.verilator_mem.inst_port = system.mem_ctrl.port
system.verilator_mem.data_port = system.mem_ctrl.port

# set up the root SimObject and start the simulation
root = Root(full_system = True, system = system)

# instantiate all of the objects we've created above
m5.instantiate()

print("Beginning simulation!")
exit_event = m5.simulate(200000)
print('Exiting @ tick %i because %s' % (m5.curTick(), exit_event.getCause()))
