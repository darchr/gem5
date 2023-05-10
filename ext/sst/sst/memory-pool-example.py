# Copyright (c) 2021 The Regents of the University of California
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

import sst
import sys
import os

from sst import UnitAlgebra

default_link_latency = "10ns"

bbl = "riscv-boot-exit-nodisk"
cpu_clock_rate = "4GHz"
# gem5 will send requests to physical addresses of range [0x80000000, inf) to memory
# currently, we do not subtract 0x80000000 from the request's address to get the "real" address
# so, the mem_size would always be 2GiB larger than the desired memory size
memory_size_sst = "16GiB"
addr_range_end = UnitAlgebra(memory_size_sst).getRoundedValue()

cpu_params = {
    "frequency": cpu_clock_rate, # TODO: this is not a param; the CPU clock rate is set in gem5, not here
    "cmd": "../../disaggregated_memory_setup/numa_sst_remote_config.py --command={} --cpu-type={}".format("numastat", "timing"),
    "debug_flags": ""
}

gem5_node = sst.Component("gem5_node", "gem5.gem5Component")
gem5_node.addParams(cpu_params)

main_bus = sst.Component("main_bus", "memHierarchy.Bus")
main_bus.addParams( { "bus_frequency" : cpu_clock_rate } )

cache_port = gem5_node.setSubComponent("cache_port", "gem5.gem5Bridge", 0) # SST -> gem5
cache_port.addParams({ "response_receiver_name": "board.remote_memory_outgoing_bridge"})

# Memory
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : "0",
    "clock" : "1.6GHz", # DDR4
    "request_width" : "64",
    "backend.mem_size": memory_size_sst,
    "addr_range_start": 0x100000000, # TODO: a constant for now, should be a parameter reflected in the gem5 config
    "addr_range_end" : 0x100000000 + UnitAlgebra(memory_size_sst).getRoundedValue(), # should be changed accordingly to memory_size_sst
})
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "15ns",
    "mem_size" : memory_size_sst
})

# Connections

node_bus_link = sst.Link("node_bus_link")
node_bus_link.connect(
    (cache_port, "port", default_link_latency),
    (main_bus, "high_network_0", default_link_latency)
)

bus_mem_link = sst.Link("bus_mem_link")
bus_mem_link.connect(
    (main_bus, "low_network_0", default_link_latency),
    (memctrl, "direct_link", default_link_latency)
)

# enable Statistics
stat_params = { "rate" : "0ns" }
sst.setStatisticLoadLevel(5)
sst.setStatisticOutput("sst.statOutputTXT", {"filepath" : "./sst-stats.txt"})
sst.enableAllStatisticsForComponentName("memory", stat_params)
