# Copyright (c) 2023 The Regents of the University of California
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

# This SST configuration file tests a merlin router.
import sst
import sys
import os

from sst import UnitAlgebra

cache_link_latency = "1ns"

bbl = "riscv-boot-exit-nodisk"
cpu_clock_rate = "1GHz"
# gem5 will send requests to physical addresses of range [0x80000000, inf) to memory
# currently, we do not subtract 0x80000000 from the request's address to get the "real" address
# so, the mem_size would always be 2GiB larger than the desired memory size
# memory_size_gem5 = "2GiB"
# memory_size_sst = "4GiB"
# addr_range_end = UnitAlgebra(memory_size_sst).getRoundedValue()

l1_params = {
    "access_latency_cycles" : "1",
    "cache_frequency" : cpu_clock_rate,
    "replacement_policy" : "lru",
    "coherence_protocol" : "MESI",
    "associativity" : "4",
    "cache_line_size" : "64",
    "cache_size" : "32 MiB",
    "L1" : "1",
}

dirNicParams = {
      "network_bw" : "25GB/s",
      "group" : 1,
}

def create_cache(name, params = None):
    cache = sst.Component(name, "memHierarchy.Cache")
    if params is None:
        cache.addParams(l1_params)
    else:
        cache.addParams(params)
    return cache

def connect_components(link_name: str,
                       low_port_name: str, low_port_idx: int,
                       high_port_name: str, high_port_idx: int,
                       port = False, direct_link = False):
    link = sst.Link(link_name)
    low_port = "low_network_" + str(low_port_idx)
    if port == True:
        low_port = "port"
    high_port = "high_network_" + str(high_port_idx)
    if direct_link == True:
        high_port = "direct_link"
    link.connect(
        (low_port_name, low_port, cache_link_latency),
        (high_port_name, high_port, cache_link_latency)
    )

# =========================================================================== #

# Define the number of gem5 nodes in the system.
system_nodes = 1

# Define the total number of SST Memory nodes
memory_nodes = 1

# This example uses fixed number of node size -> 2 GiB
# TODO: Fix this in the later version of the script.
# The directory controller decides where the addresses are mapped to.
node_memory_slice = "2GiB"
remote_memory_slice = "2GiB"

# SST memory node size. Each system gets a 2 GiB slice of fixed memory.
sst_memory_size = str((memory_nodes * int(node_memory_slice[0])) + (system_nodes) * 2 + 2) + "GiB"
addr_range_end = UnitAlgebra(sst_memory_size).getRoundedValue()
print(sst_memory_size)

# There is one cache bus connecting all gem5 ports to the remote memory.
mem_bus = sst.Component("membus", "memHierarchy.Bus")
mem_bus.addParams( { "bus_frequency" : cpu_clock_rate } )

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.setRank(0, 0)
# `addr_range_end` should be changed accordingly to memory_size_sst
memctrl.addParams({
    "debug" : "1",
    "clock" : "1GHz",
    "request_width" : "64",
        "verbose" : 2,
    "debug_level" : 10,
    "backing" : "none",
    "addr_range_end" : addr_range_end,
})
# memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
# memory.addParams({
#     "access_time" : "50ns",
#     "mem_size" : sst_memory_size
# })

memory = memctrl.setSubComponent( "backend", "memHierarchy.timingDRAM")
memory.addParams({
    "id" : 0,
    "addrMapper" : "memHierarchy.sandyBridgeAddrMapper",
    "addrMapper.interleave_size" : "64B",
    "addrMapper.row_size" : "1KiB",
    "clock" : "2.4GHz",
    "mem_size" : sst_memory_size,
    "channels" : 1,
    "channel.numRanks" : 2,
    "channel.rank.numBanks" : 16,
    "channel.transaction_Q_size" : 64,
    "channel.rank.bank.CL" : 14,
    "channel.rank.bank.CL_WR" : 12,
    "channel.rank.bank.RCD" : 14,
    "channel.rank.bank.TRP" : 14,
    "channel.rank.bank.dataCycles" : 2,
    "channel.rank.bank.pagePolicy" : "memHierarchy.timeoutPagePolicy",
    "channel.rank.bank.transactionQ" : "memHierarchy.reorderTransactionQ",
    "channel.rank.bank.pagePolicy.timeoutCycles" : 50,
    "printconfig" : 0,
    "channel.printconfig" : 0,
    "channel.rank.printconfig" : 0,
    "channel.rank.bank.printconfig" : 0,
})


# Add all the Gem5 nodes to this list.
gem5_nodes = []
memory_ports = []

# Create each of these nodes and conect it to a SST memory cache
for node in range(system_nodes):
    # Each of the nodes needs to have the initial parameters. We might need to
    # to supply the instance count to the Gem5 side. This will enable range
    # adjustments to be made to the DTB File.
    cmd = [
        f"--outdir=traffic_gen_{node}",
        "../../configs/example/sst/traffic_gen.py",
        f"--cpu-clock-rate {cpu_clock_rate}",
        "--memory-size 1GiB"
    ]
    ports = {
        "remote_memory_port" : "system.memory_outgoing_bridge"
    }
    port_list = []
    for port in ports:
        port_list.append(port)
    cpu_params = {
       "frequency" : cpu_clock_rate,
       "cmd" : " ".join(cmd),
       "debug_flags" : "", # TrafficGen",
       "ports" : " ".join(port_list)
    }
    # Each of the Gem5 node has to be separately simulated. TODO: Figure out
    # this part on the mpirun side.
    gem5_nodes.append(
        sst.Component("gem5_node_{}".format(node), "gem5.gem5Component")
    )
    gem5_nodes[node].addParams(cpu_params)
    gem5_nodes[node].setRank(node + 1, 0)

    memory_ports.append(
        gem5_nodes[node].setSubComponent(
            "remote_memory_port", "gem5.gem5Bridge", 0
        )
    )
    memory_ports[node].addParams({
        "response_receiver_name" : ports["remote_memory_port"]
    })
    
    # we dont need directory controllers in this example case. The start and
    # end ranges does not really matter as the OS is doing this management in
    # in this case.
    connect_components(f"node_{node}_mem_port_2_mem_bus",
                       memory_ports[node], 0,
                       mem_bus, node,
                       port = True)
    
# All system nodes are setup. Now create a SST memory. Keep it simplemem for
# avoiding extra simulation time. There is only one memory node in SST's side.
# This will be updated in the future to use number of sst_memory_nodes

connect_components("membus_2_memory",
                   mem_bus, 0,
                   memctrl, 0,
                   direct_link = True)

# enable Statistics
stat_params = { "rate" : "0ns" }
sst.setStatisticLoadLevel(10)
sst.setStatisticOutput("sst.statOutputTXT", {"filepath" : "./sst-traffic-example.txt"})
sst.enableAllStatisticsForAllComponents()
