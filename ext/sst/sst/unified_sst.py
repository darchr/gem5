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
# For multi-node simulation, make sure to set the instance id correctly. This
# script is ISA agnostic and expects the user to compile gem5 ALL to avoid ISA
# confusion.

import os
import sst
import json
from sst import UnitAlgebra
import argparse

parser = argparse.ArgumentParser()

parser.add_argument(
    "--output-directory",
    type=str,
    required=True,
    help="Output directory where gem5 has stored all the stats and " +
        "checkpoints",
)
# parser.add_argument(
#     "--gem5-config",
#     type=str,
#     required=True,
#     help="Point SST to where the gem5 side script is located. SST expects " +
#         "users to deal with ISA related changes of gem5.",
# )
# parser.add_argument(
#     "--sst-memory-size",
#     type=str,
#     required=True,
#     help="The unified runscript calculates the total memory required for " +
#     "starting SST.",
# )
parser.add_argument(
    "--jobs-path",
    type=str,
    required=True,
    help="The JSON of gem5 jobs will be sent over to SST for simplicity.",
)
parser.add_argument(
    "--clock",
    type=str,
    required=True,
    help="Specify the core frequency for all the processes",
)

parser.add_argument(
    "--checkpoints",
    type=str,
    required=True,
    choices=["True", "False"],
    help="Remote memory range",
)

args = parser.parse_args()

# convert True/False strings into boolean.
checkpoints =  {"True": True, "False": False}[args.checkpoints]

# if checkpoints == False:
#     raise NotImplementedError


def connect_components(link_name: str,
                       low_port_name: str, low_port_idx: int,
                       high_port_name: str, high_port_idx: int,
                       remote_memory_latency: str = None,
                       port = False, direct_link = False,
                       latency = False) -> None:
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
        assert (latency == True)
        assert (remote_memory_latency != None)
        link.connect(
            (low_port_name, low_port, cache_link_latency),
            (high_port_name, high_port, remote_memory_latency)
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

def util_int_to_size(size_in_bytes: int) -> str:
    return str(size_in_bytes/1024/1024/1024) + "GiB"
# =========================================================================== #

cache_link_latency = "1ps"

cpu_clock_rate = args.clock
print(cpu_clock_rate)

# The disaggregated_memory latency should be set in the JSON.
f = open(args.jobs_path)
jobs = json.loads(f.read())
f.close()

# Note that the length of the dictionary is 1 node more than gem5s as there is
# metadata attached!

# =========================================================================== #

# Define the number of gem5 nodes in the system. The metadata is either inside
# the node configuration or is sent as command line.
system_nodes = len(jobs)

# Define the total number of SST Memory nodes. This is hardcoded and fixed. To
# change this, the entire structure of the boards have to be changed :(
memory_nodes = 1

# The blank memory range is the starting address of node 0
blank_memory_space = jobs["0"]["remote-memory"]["start"]
total_sst_memory = int(blank_memory_space, 16)

# We only support shared or not shared regions. not a mix of both
for job in jobs:
    total_sst_memory = total_sst_memory + int(
                                jobs[job]["remote-memory"]["end"], 16) - \
                                int(jobs[job]["remote-memory"]["start"], 16)
    # If this range is shared, then we just break from the loop, and assume all
    # the subsequent ranges are shared in the same memory range.
    if jobs[job]["remote-memory"]["shared"] == "true":
        break

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
    "addr_range_start": 0x0,
    "addr_range_end" : UnitAlgebra(
        util_int_to_size(total_sst_memory)).getRoundedValue(),
})

# We need a DDR4-like memory device. This is hardcoded.
memory = memctrl.setSubComponent( "backend", "memHierarchy.timingDRAM")
memory.addParams({
    "id" : 0,
    "addrMapper" : "memHierarchy.simpleAddrMapper",
    "addrMapper.interleave_size" : "64B",
    "addrMapper.row_size" : "1KiB",
    "clock" : "1.2GHz",
    "mem_size" : util_int_to_size(total_sst_memory),
    "channels" : 4,
    "channel.numRanks" : 2,
    "channel.rank.numBanks" : 16,
    "channel.rank.bank.TRP" : 14,
    "printconfig" : 1,
})

# Add all the Gem5 nodes to this list.
gem5_nodes = []
memory_ports = []

output_directory = args.output_directory

# Create each of these nodes and conect it to a SST memory cache
for node in range(system_nodes):
    # The key of this system
    job = str(node)

    # The binary for SST is located in ext/sst. This needs to be accounted for
    # when starting the SST-side script
    p_config = os.path.join(os.getcwd(), "../../")
    # depending upon shared memory, the config script will change (the
    # board changes behind the scene). Will be read directly
    # Next up is the ISA. Are we running this process from the gem5
    # directory?
    if os.path.exists(
            os.path.join(p_config, "disaggregated_memory")) == False:
        # Force the user to restart the process!
        print("1", p_config, output_directory)
        print("fatal: Please run this script from the top gem5 directory.")
        exit(-1)

    if jobs[job]["cpu"]["isa"].lower() == "arm":
        p_config = os.path.join(os.getcwd(),
                        "../../disaggregated_memory/configs/arm_unified.py")
    elif jobs[job]["cpu"]["isa"].lower() == "riscv":
        p_config = os.path.join(os.getcwd(),
                        "../../disaggregated_memory/configs/riscv_unified.py")
    elif jobs[job]["cpu"]["isa"].lower() == "x86":
        p_config = os.path.join(os.getcwd(),
                        "../../disaggregated_memory/configs/x86_unified.py")
    else:
        print("fatal: Unsupported ISA!")
        exit(-1)    

    # Update this to use python fstring.
    cmd = [
        # "-re",
        "--outdir=" + os.path.join(output_directory,
                        jobs[job]["metadata"]["experiment"]) + "_" + str(node),
        p_config]
    
    # Check if a TICK or MAXINST exists in the config file.
    max_ticks = 0
    max_insts = 0
    try:
        if jobs[job]["metadata"]["maxtics"] != "":
            max_ticks = int(jobs[job]["metadata"]["maxtics"])
    except KeyError:
        # Maxtics doesn't exist!
        max_ticks = 0
    except ValueError:
        # maxtics exists but is an empty string
        max_ticks = 0
    try:
        if jobs[job]["metadata"]["maxinsts"] != "":
            max_insts = int(jobs[job]["metadata"]["maxinsts"])
    except KeyError:
        # Maxtics doesn't exist!
        max_insts = 0
    except ValueError:
        # maxtics exists but is an empty string
        max_insts = 0
    
    # Make sure that both of these are not set
    if max_insts == max_ticks:
        # see if these are 0
        if max_insts != 0:
            print("fatal: cannot simulate with both max insts and max tics!")
            exit(-1)
        
    
    rest_of_cmd = [
        "--instance=" + job,
        "--ff-core-type=" + jobs[job]["cpu"]["ff-core"],
        "--roi-core-type=" + jobs[job]["cpu"]["roi-core"],
        "--core-count=" + jobs[job]["cpu"]["count"],
        # The clock is fixed to the same frequency.
        "--core-frequency=" + cpu_clock_rate,
        "--cache-type=" + jobs[job]["cache"]["type"],
        "--l1i-size=" + jobs[job]["cache"]["l1i-size"],
        "--l1d-size=" + jobs[job]["cache"]["l1d-size"],
        "--l2-size=" + jobs[job]["cache"]["l2-size"],
        "--l3-size=" + jobs[job]["cache"]["l3-size"],
        "--l3-assoc=" + jobs[job]["cache"]["l3-assoc"],
        "--local-memory-type=" + jobs[job]["local-memory"]
                            ["type"],
        "--local-memory-size=" + jobs[job]["local-memory"]
                            ["size"],
        "--remote-memory-shared=" + jobs[job]["remote-memory"]
                                                ["shared"].lower(),
        "--remote-memory-start=" + str(jobs[job]["remote-memory"]["start"]),
        "--remote-memory-end=" + str(jobs[job]["remote-memory"]["end"]),
        # When running gem5 processes, we have to specify
        # --is-composable as False as we don't use SST here.
        "--is-composable=true",
        # Does not really matter for the following arguments.
        "--cmd=\"\"", # + "__fmt__".join(jobs[job]["workitem"]["cmd"]),
        "--disk-path=" + jobs[job]["workitem"]["disk"],
        "--kernel-path=" + jobs[job]["workitem"]["kernel"],
        "--bootloader-path=" + jobs[job]["workitem"]["bootloader"]
        
    ]
    
    # Make sure to put in the max tick/inst case if there is. This is needed
    # for reallife workloads.
    if max_ticks != 0:
        cmd = cmd + ["--abs-max-tick=" + str(max_ticks)] + rest_of_cmd
    elif max_insts != 0:
        cmd = cmd + ["--maxinsts=" + str(max_insts)] + rest_of_cmd
    else:
        cmd = cmd + rest_of_cmd
    
    ports = {
        "remote_memory_port" : "board.remote_memory.outgoing_request_bridge"
    }
    port_list = []
    for port in ports:
        port_list.append(port)
    cpu_params = {
       "frequency" : cpu_clock_rate,
       "cmd" : " ".join(cmd),
       "debug_flags" : "",
       "ports" : " ".join(port_list)
    }
    # Each of the Gem5 node has to be separately simulated.
    gem5_nodes.append(
        sst.Component("gem5_node_{}".format(node), "gem5.gem5Component")
    )
    gem5_nodes[node].addParams(cpu_params)
    
    # To boost parallel efficiency, we can put the memory-only node to an MPI
    # rank simulating a gem5 node.
    try:
        if (jobs[str(node)]["metadata"]["parallel-eff"] == "false"):
            gem5_nodes[node].setRank(node, 0)
        else:
            gem5_nodes[node].setRank(node + 1, 0)
    except KeyError:
        # In case the joblist has this field missing, the simulation defaults
        # to allocating a new rank to the memory node.
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
    # TODO: Figure out if we need to add the link latency here?
    print(jobs[job]["remote-memory"]["latency"])
    connect_components(f"node_{node}_mem_port_2_mem_bus",
                memory_ports[node], 0,
                mem_bus, node,
                remote_memory_latency=jobs[job]["remote-memory"]["latency"],
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
# sst.setStatisticLoadLevel(10)
sst.setStatisticOutput("sst.statOutputTXT",
        {"filepath" : f"arm-main-board.txt"})
sst.enableAllStatisticsForAllComponents()
