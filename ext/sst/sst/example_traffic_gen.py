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
import argparse

from sst import UnitAlgebra

# Setup an argpase to automate all the experiments


# SST passes a couple of arguments for this system to simulate.
parser = argparse.ArgumentParser()

parser.add_argument("--link-latency", type=str, default="1ps")
parser.add_argument("--nodes", type=int, default=1)
args = parser.parse_args()

# The disaggregated_memory latency should be set at SST's side as a link
# latency.
# XXX
disaggregated_memory_latency = args.link_latency
cache_link_latency = "1ns"

bbl = "riscv-boot-exit-nodisk"
cpu_clock_rate = "3.1GHz"
def connect_components(link_name: str,
                       low_port_name: str, low_port_idx: int,
                       high_port_name: str, high_port_idx: int,
                       port = False, direct_link = False, latency = False):
    link = sst.Link(link_name)
    low_port = "low_network_" + str(low_port_idx)
    if port == True:
        low_port = "port"
    high_port = "high_network_" + str(high_port_idx)
    if direct_link == True:
        high_port = "direct_link"
    if latency == False:
        link.connect(
            (low_port_name, low_port, cache_link_latency),
            (high_port_name, high_port, cache_link_latency)
        )
    else:
        # TODO: Figure out if the added latency is correct!
        link.connect(
            (low_port_name, low_port, cache_link_latency),
            (high_port_name, high_port, disaggregated_memory_latency)
        )

def get_address_range(node, remote_mem_size):
    """
    This function returns a list of start and end address corresponding to a
    given node in SST

    @params
    :node: Node index (aka the instance/system node id)
    :local_mem_size: Local memory size as integer
    :remote_mem_size: Remote memory size as interger
    :blank_mem_size: The I/O hole as interger

    @returns [start_addr, end_addr] for the remote memory
    """
    return [(node) * remote_mem_size, (node + 1) * remote_mem_size]
# =========================================================================== #

# Define the number of gem5 nodes in the system.
system_nodes = args.nodes

# Define the total number of SST Memory nodes
memory_nodes = 1

# This example uses fixed number of node size -> 2 GiB
# TODO: Fix this in the later version of the script.
# The directory controller decides where the addresses are mapped to.
node_memory_slice = "2GiB"
remote_memory_slice = "2GiB"

# SST memory node size. Each system gets a 2 GiB slice of fixed memory.
sst_memory_size = str(system_nodes * 2) + "GiB"
addr_range_end = UnitAlgebra(sst_memory_size).getRoundedValue()
print(sst_memory_size)

addr_range_end = UnitAlgebra(sst_memory_size).getRoundedValue()
print(sst_memory_size, addr_range_end)

# There is one cache bus connecting all gem5 ports to the remote memory.
mem_bus = sst.Component("membus", "memHierarchy.Bus") 
mem_bus.addParams( { "bus_frequency" : cpu_clock_rate } )

# Set memctrl params
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.setRank(0, 0)

# `addr_range_end` should be changed accordingly to memory_size_sst
memctrl.addParams({
    "debug" : "0",
    "clock" : "1200MHz",
    "request_width" : "64",
    "addr_range_end" : addr_range_end,
})

# We need a DDR4-like memory device.
memory = memctrl.setSubComponent( "backend", "memHierarchy.timingDRAM")
memory.addParams({
        "id" : 0,
        "addrMapper" : "memHierarchy.simpleAddrMapper", # roundRobinAddrMapper",
        "addrMapper.interleave_size" : "64B",
        "addrMapper.row_size" : "1KiB",
        "clock" : "1200MHz",
        "mem_size" : sst_memory_size,
        "channels" : 4,
        "channel.numRanks" : 2,
        "channel.rank.numBanks" : 16,
        "channel.transaction_Q_size" : 128,
        "channel.rank.bank.CL" : 14,
        # "channel.rank.bank.CL_WR" : 12,
        "channel.rank.bank.RCD" : 14,
        "channel.rank.bank.TRAS" : 32,
        "channel.rank.bank.TRP" : 14,
        # "channel.rank.bank.dataCycles" : 2,
        "channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
        "channel.rank.bank.pagePolicy.close" : "false",
        "channel.rank.bank.transactionQ" : "memHierarchy.reorderTransactionQ",
        "channel.rank.bank.pagePolicy.close" : 0,
        "printconfig" : 1,
        "channel.printconfig" : 0,
        "channel.rank.printconfig" : 0,
        "channel.rank.bank.printconfig" : 0,
})

gem5_nodes = []
memory_ports = []

# Create each of these nodes and conect it to a SST memory cache
for node in range(system_nodes):
    # Each of the nodes needs to have the initial parameters. We might need to
    # to supply the instance count to the Gem5 side. This will enable range
    # adjustments to be made to the DTB File.

    node_range = get_address_range(node, 0x80000000)
    # node_range = [0x0, 0x80000000]
    cmd = [
        f"--outdir=traffic/linear/{system_nodes}/{disaggregated_memory_latency}/traffic_gen_{node}",
        "../../disaggregated_memory/configs/traffic_gen.py",
        f"--cpu-clock-rate {cpu_clock_rate}",
        f"--memory-addr-range {node_range[0]},{node_range[1]}",
        f"--instance={node}"
        # "--memory-size 2GiB"
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
                       port = True, latency = True)
    
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
