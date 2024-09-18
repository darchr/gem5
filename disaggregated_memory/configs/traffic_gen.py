# Copyright (c) 2023-24 The Regents of the University of California
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
# this software without specific SSTInterfaceprior written permission.
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

import m5
from m5.objects import *
from os import path
import argparse

def generate_traffic(tgen, start_addr, end_addr, instance):
    yield tgen.createLinear(
    # yield tgen.createRandom(
        100000000,
        start_addr, # + instance * 8,
        end_addr,
        64,
        1000,
        1000,
        100,
        0
    )
    yield tgen.createExit(0)

# ---------------------------------------------------------------

parser = argparse.ArgumentParser()
parser.add_argument(
    "--cpu-clock-rate",
    type=str,
    help="CPU clock rate, e.g. 3GHz",
    default = "1GHz"
)
parser.add_argument(
    "--memory-size",
    type=str,
    help="Memory size, e.g. 4GiB",
    default = "1GiB"
)
parser.add_argument(
    "--memory-addr-range",
    type=str,
    required=True
)
parser.add_argument(
    "--instance",
    type=int,
    required=True
)

args = parser.parse_args()

cpu_clock_rate = args.cpu_clock_rate
memory_size = args.memory_size
instance = args.instance

remote_memory_range = list(map(int, args.memory_addr_range.split(",")))
remote_memory_range = AddrRange(remote_memory_range[0], remote_memory_range[1])

# ---------------------------------------------------------------

system = System()
system.membus = NoncoherentXBar(
    frontend_latency=1,
    forward_latency=0,
    response_latency=0,
    header_latency=0,
    width=256,
)
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = cpu_clock_rate
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_ranges = [remote_memory_range]

system.mem_mode = "timing"

system.tgen = PyTrafficGen()
system.monitor = CommMonitor()

system.tgen.port = system.monitor.cpu_side_port
system.monitor.mem_side_port = system.membus.cpu_side_ports
# system.tgen.port = system.membus.cpu_side_ports
system.system_port = system.membus.cpu_side_ports

system.memory_outgoing_bridge = ExternalMemory(
    physical_address_ranges=system.mem_ranges[0]
)
system.memory_outgoing_bridge.range = system.mem_ranges[0]

print(system.memory_outgoing_bridge.physical_address_ranges[0].start)
system.memory_outgoing_bridge.port = system.membus.mem_side_ports

root = Root(full_system=False, system=system)

m5.instantiate()
print(system.mem_ranges[0].start, system.mem_ranges[0].end)
system.tgen.start(
        generate_traffic(system.tgen,
                        system.mem_ranges[0].start,
                        system.mem_ranges[0].end,
                        instance)
)

