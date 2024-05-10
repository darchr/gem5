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

# This SST configuration file can be used with the Composable script in gem5.
# For multi-node simulation, make sure to set the instance id correctly.

import sst
from sst import UnitAlgebra
import argparse

parser = argparse.ArgumentParser()

parser.add_argument(
    "--outdir",
    type=str,
    required=True,
    help="Output directory",
)
parser.add_argument(
    "--system-nodes",
    type=int,
    required=True,
    help="Number of nodes connected to the disaggregated memory system.",
)
parser.add_argument(
    "--memory-alloc-policy",
    type=str,
    required=True,
    help="The policy to allocate memory. This can be either local, interleaved or remote.",
)
parser.add_argument(
    "--sst-memory-size",
    type=str,
    required=True,
    help="Remote memory size",
)
parser.add_argument(
    "--remote-memory-addr-range",
    type=str,
    required=True,
    help="Remote memory range",
)

args = parser.parse_args()

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

def get_address_range(node, local_mem_size, remote_mem_size, blank_mem_size):
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
    return [blank_mem_size + (node + 1) * local_mem_size + \
                    (node) * remote_mem_size,
            blank_mem_size + (node + 1) * local_mem_size + \
                    (node) * remote_mem_size + remote_mem_size
    ]

# =========================================================================== #
gem5_run_script = "../../disaggregated_memory/configs/arm-main.py"

# The disaggregated_memory latency should be set at SST's side as a link
# latency.
# XXX
disaggregated_memory_latency = "750ns"

cache_link_latency = "1ps"

cpu_clock_rate = "4GHz"

# The following parameters have to be manually set by the user
# output directory
# XXX
stat_output_directory = args.outdir+"/m5out_"

# It is expected that if this script is executed from SST, the memory is
# composable.

memory_alloc_policy = args.memory_alloc_policy

# Define the CPU type
cpu_type = "o3"



# =========================================================================== #

# Define the number of gem5 nodes in the system. anything more than 1 needs
# mpirun to run the sst binary.
system_nodes = args.system_nodes

# Define the total number of SST Memory nodes
memory_nodes = 1

# This example uses fixed number of node size -> 2 GiB
# The directory controller decides where the addresses are mapped to.
node_memory_slice = "2GiB"
node_memory_slice_in_hex = 0x80000000

# We are use 32 GiB of remote memory per node.
remote_memory_slice = "2GiB"
remote_memory_slice_in_hex = 0x80000000

# The first 2 GB is ignored for I/O devices.
blank_memory_space = "2GiB"
blank_memory_space_in_hex = 0x80000000

sst_memory_size = args.sst_memory_size
addr_range_end = UnitAlgebra(sst_memory_size).getRoundedValue()
print(sst_memory_size, addr_range_end)
remote_memory_range = list(map(int, args.remote_memory_addr_range.split(",")))

# There is one cache bus connecting all gem5 ports to the remote memory.
mem_bus = sst.Component("membus", "memHierarchy.Bus") 
mem_bus.addParams( { "bus_frequency" : cpu_clock_rate } )

# Set memctrl params
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.setRank(0, 0)

# `addr_range_end` should be changed accordingly to memory_size_sst
memctrl.addParams({
    "debug" : "0",
    "clock" : "1.2GHz",
    "request_width" : "64",
    "addr_range_end" : addr_range_end,
})
# We need a DDR4-like memory device.
memory = memctrl.setSubComponent( "backend", "memHierarchy.timingDRAM")
memory.addParams({
    "id" : 0,
    "addrMapper" : "memHierarchy.simpleAddrMapper",
    "addrMapper.interleave_size" : "64B",
    "addrMapper.row_size" : "1KiB",
    "clock" : "1.2GHz",
    "mem_size" : sst_memory_size,
    "channels" : 4,
    "channel.numRanks" : 2,
    "channel.rank.numBanks" : 16,
    "channel.rank.bank.TRP" : 14,
    "printconfig" : 1,
})

# Add all the Gem5 nodes to this list.
gem5_nodes = []
memory_ports = []

# Create each of these nodes and conect it to a SST memory cache
for node in range(system_nodes): 
    cmd = [
        f"-re",
        f"--outdir={stat_output_directory + str(node)}",
        f"{gem5_run_script}",
        f"--cpu-type {cpu_type}",
        f"--instance {node}",
        f"--local-memory-size {node_memory_slice}",
        f"--is-composable True",
        f"--remote-memory-addr-range {remote_memory_range[node*2]},{remote_memory_range[node*2+1]}",
        f"--memory-alloc-policy {args.memory_alloc_policy}",
        f"--take-ckpt False",  # This setup is not expected to take checkpoints
        f"--ckpt-file ../../ckpt_instance_{node}/ckpt_{node}",
    ]
    ports = {
        "remote_memory_port" : "board.remote_memory.outgoing_request_bridge"
    }
    port_list = []
    for port in ports:
        port_list.append(port)
    cpu_params = {
       "frequency" : cpu_clock_rate,
       "cmd" : " ".join(cmd),
       "debug_flags" : "Checkpoint,MemoryAccess",
       "ports" : " ".join(port_list)
    }
    # Each of the Gem5 node has to be separately simulated.
    gem5_nodes.append(
        sst.Component("gem5_node_{}".format(node), "gem5.gem5Component")
    )
    gem5_nodes[node].addParams(cpu_params)
    gem5_nodes[node].setRank(node, 0)

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
    # TODO: Figure out if we need to add the link latency here?
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
sst.setStatisticOutput("sst.statOutputTXT",
        {"filepath" : f"arm-main-board.txt"})
sst.enableAllStatisticsForAllComponents()
