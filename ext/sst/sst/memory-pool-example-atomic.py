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

sst.setProgramOption("partitioner", "gem5NodesPartitioner.gem5NodesPartitioner")

default_link_latency = "10ns"

cpu_clock_rate = "4GHz"

num_nodes = 1
memory_node = 0

# -----------------------------------------------------------------------------------------------------
# Setting up the Memory system
# -----------------------------------------------------------------------------------------------------
memory_size_sst = "16GiB"

main_bus = sst.Component(f"main_bus_{memory_node}", "memHierarchy.Bus")
main_bus.addParams( { "bus_frequency" : cpu_clock_rate } )

memctrl = sst.Component(f"memory_{memory_node}", "memHierarchy.MemController")
memory_offset = 1 << 34
memctrl.addParams({
    "debug" : "0",
    "clock" : "1.6GHz", # DDR4
    "request_width" : "64",
    "addr_range_start": memory_offset, # TODO: a constant for now, should be a parameter reflected in the gem5 config
    "addr_range_end" : memory_offset + UnitAlgebra(memory_size_sst).getRoundedValue(), # should be changed accordingly to memory_size_sst
})
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "15ns",
    "mem_size" : memory_size_sst
})

bus_mem_link = sst.Link("bus_mem_link")
bus_mem_link.connect(
    (main_bus, "low_network_0", default_link_latency),
    (memctrl, "direct_link", default_link_latency)
)

# -----------------------------------------------------------------------------------------------------
# Setting up the gem5 nodes
# -----------------------------------------------------------------------------------------------------

gem5_nodes = []
links = []
cache_ports = []
memory_partition_size = UnitAlgebra("512MiB").getRoundedValue()
for node_idx in range(num_nodes):
    addr_range_start = memory_offset + memory_partition_size * node_idx
    addr_range_end = memory_offset + memory_partition_size * (node_idx + 1)
    cpu_params = {
        "frequency": cpu_clock_rate, # TODO: this is not a param; the CPU clock rate is set in gem5, not here
        "cmd": "--outdir=m5out_{};--listener-mode=off;../../disaggregated_memory_setup_arm/numa_sst_remote_config.py;--command={};--cpu-type={};--remote-memory-range={},{}".format(node_idx, "/home/ubuntu/simple-vectorizable-microbenchmarks/stream/stream.hw 15000000", "atomic", addr_range_start, addr_range_end),
        "debug_flags": ""
    }

    gem5_node = sst.Component(f"gem5_node_{node_idx}", "gem5.gem5Component")
    gem5_node.addParams(cpu_params)

    cache_port = gem5_node.setSubComponent("cache_port", "gem5.gem5Bridge", 0) # SST -> gem5
    cache_port.addParams({ "response_receiver_name": "board.remote_memory_outgoing_bridge"})

    # Connections
    node_bus_link = sst.Link(f"node_bus_link_{node_idx}")
    node_bus_link.connect(
        (cache_port, "port", default_link_latency),
        (main_bus, f"high_network_{node_idx}", default_link_latency)
    )

    cache_ports.append(cache_port)
    gem5_nodes.append(gem5_node)
    links.append(node_bus_link)

# enable Statistics
stat_params = { "rate" : "0ns" }
sst.setStatisticLoadLevel(5)
sst.setStatisticOutput("sst.statOutputTXT", {"filepath" : "./sst-stats.txt"})
sst.enableAllStatisticsForComponentName(f"memory_{memory_node}", stat_params)
