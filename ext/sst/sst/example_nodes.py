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
cpu_clock_rate = "3GHz"
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

# DEfine the number of gem5 nodes in the system.
system_nodes = 2

# Define the total number of SST Memory nodes
memory_nodes = 1

# This example uses fixed number of node size -> 2 GiB
# TODO: Fix this in the later version of the script.
# The directory controller decides where the addresses are mapped to.
node_memory_size = "2GiB"
remote_memory_slice = "2GiB"

# SST memory node size. Each system gets a 2 GiB slice of fixed memory.
sst_memory_size = str((memory_nodes * int(node_memory_size[0])) + 2) + "GiB"
print(sst_memory_size)

# Add all the Gem5 nodes to this list.
gem5_nodes = []
cache_buses = []
directory_caches = []
comp_dirctrls = []
memory_ports = []

# Create each of these nodes and conect it to a SST memory cache
for node in range(system_nodes):
    # Each of the nodes needs to have the initial parameters. We might need to
    # to supply the instance count to the Gem5 side. This will enable range
    # adjustments to be made to the DTB File.
    cmd = [
        "--outdir=m5out_{}".format(node),
        "../../configs/example/sst/riscv_fs_node.py",
        "--cpu-clock-rate {}".format(cpu_clock_rate),
        "--memory-size {}".format(node_memory_size),
        # "--local-memory-size {}".format(node_memory_size),
        # "--remote-memory-size {}".format(remote_memory_slice),
        "--instance {}".format(node)
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
       "debug_flags" : "Plic,Clint",
       "ports" : " ".join(port_list)
    }
    # Each of the Gem5 node has to be separately simulated. TODO: Figure out
    # this part on the mpirun side.
    gem5_nodes.append(
        sst.Component("gem5_node_{}".format(node), "gem5.gem5Component")
    )
    gem5_nodes[node].addParams(cpu_params)
    # We need a separate cache bus for each of the nodes
    cache_buses.append(
        sst.Component("cache_bus_for_node_{}".format(node), "memHierarchy.Bus")
    )
    cache_buses[node].addParams({"bus_frequency" : cpu_clock_rate})
    # TODO: This needs to be updated 
    memory_ports.append(
        gem5_nodes[node].setSubComponent(
            "remote_memory_port", "gem5.gem5Bridge", 0
        )
    )
    memory_ports[node].addParams({
        "response_receiver_name" : "system.memory_outgoing_bridge"
    })
    directory_caches.append(create_cache("dir_cache_{}".format(node)))
    directory_caches[node].addParams({"network_address" : "2" })
    # Connect the basic components.
    connect_components("node_{}_mem_port_2_bus".format(node),
                       memory_ports[node], 0,
                       cache_buses[node], 0,
                       port = True)
    connect_components("node_{}_bus_2_dir_cache".format(node),
                       cache_buses[node], 0,
                       directory_caches[node], 0)
    # Create directory controllers that dictates the memory ranges for each of
    # the remote meory nodes.
    comp_dirctrls.append(sst.Component(
        "dirctrl_for_node_{}".format(node),
        "memHierarchy.DirectoryController")
    )
    addr_range_start = 0x80000000 + node * 0x80000000
    addr_range_end = 0x80000000 + (node + 1) * 0x80000000
    comp_dirctrls[node].addParams({
        "coherence_protocol" : "MESI",
        "network_address" : "1",
        "entry_cache_size" : "16384",
        "network_bw" : "25GB/s",
        "addr_range_start" : addr_range_start,  # 2 * (1024 ** 3),     # starts at 0x80000000
        "addr_range_end" : addr_range_end       # 2 * (1024 ** 3) + 2048 * (1024 ** 2) # ends at 0x100000000 (4GiB)
    })
# All system nodes are setup. Now create a SST memory. Keep it simplemem for
# avoiding extra simulation time. There is only one memory node in SST's side.
# This will be updated in the future to use number of sst_memory_nodes
memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
    "request_width" : 64,
    "coherence_protocol" : "MESI",
    "access_time" : "33 ns",
    "backend.mem_size" : sst_memory_size,
    "clock" : "2.4GHz",
    "debug" : "0",
    "range_start" : 2 * (1024 ** 3), # it's behind a directory controller and it starts at 0x80000000
    })
comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.setSubComponent("topology","merlin.singlerouter")
comp_chiprtr.addParams({
      "xbar_bw" : "128GB/s",
      "link_bw" : "128GB/s",
      "input_buf_size" : "1KB",
      "num_ports" : str(system_nodes * 2),
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
mem_bus = sst.Component("membus", "memHierarchy.Bus")
# Finally connect all the nodes together in the net
for node in range(system_nodes):
    sst.Link("link_cache_net_node_{}".format(node)).connect(
        (directory_caches[node], "directory", "10ns"),
        (comp_chiprtr, "port" + str(node * 2 + 1), "2ns"))
    sst.Link("link_dir_net_nodes_{}".format(node)).connect(
        (comp_chiprtr, "port" + str(node * 2), "2ns"),
        (comp_dirctrls[node], "network", "2ns"))
    sst.Link("link_dir_mem_link_node_{}".format(node)).connect(
        (comp_dirctrls[node], "memory", "10ns"),
        (memory, "direct_link", "10ns"))
# enable Statistics
stat_params = { "rate" : "0ns" }
sst.setStatisticLoadLevel(10)
sst.setStatisticOutput("sst.statOutputTXT", {"filepath" : "./sst-router-example.txt"})
sst.enableAllStatisticsForAllComponents()
