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

"""
This is a all-in-one X86 script to simulate gem5 + SST for disaggregated
memory. It is *STRONGLY ADVISED NOT TO RUN THIS SCRIPT AS A STANDALONE SCRIPT*
and instead use `unified_run.py` to start the simulation.

No, there is no better way to run two separate runs.

This script can be executed both from gem5 and SST.
"""

import argparse
import os
import sys

# all the source files are one directory above.
sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
)

from boards.x86_shared_board import X86SharedMemoryBoard
from boards.x86_main_board import X86ComposableMemoryBoard
from boards.x86_space_control_board import X86SpaceControlBoard

from cachehierarchies.dm_caches import ClassicPrivateL1PrivateL2SharedL3CacheHierarchyWChecks
from cachehierarchies.dm_caches import ClassicPrivateL1PrivateL2SharedL3DMCache
from cachehierarchies.dm_caches import ClassicPrivateL1PrivateL2DMCache
from memories.external_remote_memory import ExternalRemoteMemory

import m5
from m5.objects import (
    AddrRange,
    Root,
)

from gem5.components.memory import (
    DualChannelDDR4_2400,
    DualChannelDDR3_1600,
)

from gem5.isas import ISA
from gem5.resources.resource import *
from gem5.resources.workload import *
from gem5.utils.requires import requires


from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes

# SST passes a couple of arguments for this system to simulate.
parser = argparse.ArgumentParser()

parser.add_argument(
    "--instance",
    type=int,
    required=True,
    help="Instance id is need to correctly read and write to the "
    + "checkpoint in a multi-node simulation.",
)

parser.add_argument(
    "--ff-core-type",
    type=str,
    required=False,
    default="atomic",
    choices=["kvm", "atomic"],
    help="Define the fast-forwarding core.",
)

parser.add_argument(
    "--roi-core-type",
    type=str,
    required=False,
    default="timing",
    choices=["timing", "o3"],
    help="Define the ROI core.",
)

# Parameters related to remote memory
parser.add_argument(
    "--core-count",
    type=str,
    required=False,
    default="8",
    help="Specify the number of cores to use.",
)

parser.add_argument(
    "--core-frequency",
    type=str,
    required=True,
    help="Define the clock frequency of the cores"
)

parser.add_argument(
    "--cache-type",
    type=str,
    required=False,
    default="l1l2l3",
    help="",
)

parser.add_argument(
    "--l1i-size",
    type=str,
    required=False,
    default="32KiB",
    help="",
)
parser.add_argument(
    "--l1d-size",
    type=str,
    required=False,
    default="32KiB",
    help="",
)
parser.add_argument(
    "--l1i-assoc",
    type=str,
    required=False,
    default="0",
    help="",
)
parser.add_argument(
    "--l1d-assoc",
    type=str,
    required=False,
    default="0",
    help="",
)
parser.add_argument(
    "--l2-size",
    type=str,
    required=False,
    default="512KiB",
    help="",
)
parser.add_argument(
    "--l2-assoc",
    type=str,
    required=False,
    default="0",
    help="",
)
parser.add_argument(
    "--l3-size",
    type=str,
    required=False,
    default="2MiB",
    help="",
)
parser.add_argument(
    "--l3-assoc",
    type=str,
    required=False,
    default="16",
    help="",
)
parser.add_argument(
    "--local-memory-type",
    type=str,
    required=False,
    choices=["", "ddr3", "ddr4", "hbm", "ddr5"],
    default="ddr4",
    help="",
)
parser.add_argument(
    "--local-memory-size",
    type=str,
    required=False,
    default="2GiB",
    help="",
)
parser.add_argument(
    "--remote-memory-shared",
    type=str,
    required=False,
    choices=["true", "false"],
    default="false",
    help="",
)
parser.add_argument(
    "--remote-memory-start",
    type=str,
    required=True,
    help="Expected the value in hex",
)
parser.add_argument(
    "--remote-memory-end",
    type=str,
    required=True,
    help="",
)
parser.add_argument(
    "--is-composable",
    type=str,
    required=True,
    choices=["true", "false"],
    help=""
)
parser.add_argument(
    "--cmd",
    type=str,
    required=True,
    help="",
)
parser.add_argument(
    "--disk-path",
    type=str,
    required=False,
    default=
        "/home/kaustavg/projects/gem5-resources/src/benchmarks/x86/shared-simple-graphs/x86-disk-image-24-04/x86-ubuntu",
    help="",
)
parser.add_argument(
    "--kernel-path",
    type=str,
    required=False,
    default="/home/kaustavg/kernel/x86/linux-6.9.9/vmlinux",
    help="",
)
parser.add_argument(
    "--bootloader-path",
    type=str,
    required=False,
    default="",
    help="",
)

# Parameters related to checkpoints.
parser.add_argument(
    "--ckpt-file",
    type=str,
    default="",
    required=False,
    help="optionally put a path to restore a checkpoint",
)

parser.add_argument(
    "--systemd",
    type=str,
    required=True,
    choices=["true", "false"],
    help="Optionally the user can simulate the system with systemd"
)

# For space control
parser.add_argument(
    "--permission",
    type=str,
    required=True,
    choices=["none", "flat-tables", "deact", "mondrian", "space-control"],
    help="An option to simulate permission table"
)


args = parser.parse_args()

# Make sure that the default values are input correctly. if any of the input
# arguments are "", then a default value has to be reassigned!
# TODO
if args.core_count == "":
    args.core_count = "8"

if args.cache_type == "":
    args.cache_type = "l1l2l3"

if args.l1i_size == "":
    args.l1i_size = "32KiB"

if args.l1d_size == "":
    args.l1d_size = "32KiB"

if args.l2_size == "":
    args.l2_size = "512KiB"

if args.l3_size == "":
    args.l3_size = "2MiB"

if args.l3_assoc == None or args.l3_assoc == "":
    args.l3_assoc = 16

if args.local_memory_size == "":
    args.local_memory_size = "2GiB"


use_sst = {"true": True, "false": False}[args.is_composable]
ff_core = {"kvm": CPUTypes.KVM,
           "atomic": CPUTypes.ATOMIC}[args.ff_core_type]
roi_core = {"timing": CPUTypes.TIMING,
           "o3": CPUTypes.O3}[args.roi_core_type]
core_count = int(args.core_count)
core_freq = args.core_frequency
systemd = {"true": True, "false": False}[args.systemd]

# Figure out the core to use.
core_to_use = ff_core
if use_sst == True:
    core_to_use = roi_core
    
cache_type = {  "l1l2l3": ClassicPrivateL1PrivateL2SharedL3DMCache(
                        l1i_size=args.l1i_size,
                        l1d_size=args.l1d_size,
                        l2_size=args.l2_size,
                        l3_size=args.l3_size,
                        l3_assoc=args.l3_assoc),
                "l1l2": ClassicPrivateL1PrivateL2DMCache(
                        l1i_size=args.l1i_size,
                        l1d_size=args.l1d_size,
                        l2_size=args.l2_size,)}[args.cache_type]
"""
if args.permission == "none":
    print("permissions -- none")
elif args.permission == "flat-tables":
    # Cache type will be overridden by the type of permission table
    warn("The cache is overridden with flat tables cache")
    cache_type = ClassicPrivateL1PrivateL2SharedL3CacheHierarchyWChecks(
        l1d_size="32KiB",
        l1i_size="32KiB",
        l2_size="1MiB",
        l3_size="16MiB",
    )
    cache_type.permission_table.model_name = "flat-table"

    # flat tables consume a lot of storage. this needs to be modeled correctly.
    # The host is needed to be specified to figure our where is the repeated entry
    cache_type.get_permission_table().host_id = args.instance

    # configure the permission table for flat tables control
    cache_type.get_permission_table().enable_permission_check = True

    # We'll get to this later.
    cache_type.get_permission_table().use_dedicated_caching = False
    # make sure that the permission parameters are setup correctly.
    cache_type.get_permission_table().permission_base_addr = 0x8C0000000 # 35 GiB
    cache_type.get_permission_table().remote_memory_start = 0x500000000
    cache_type.get_permission_table().local_memory_start =  0x100000000
    cache_type.get_permission_table().local_memory_end =  0x500000000

    # Number of entries is used to override the class contructor.
    # cache_hierarchy.get_permission_table().number_of_entries = (0x800000000 / (2 ** 12))

    cache_type.get_permission_table().binary_search = True
    # using parameters from the driver. After the cacheline version is finished,
    # this latency is drastically reduced!
    cache_type.get_permission_table().permission_entry_size = 64

    cache_type.get_permission_table().total_memory_size = 0x400000000
elif args.permission == "space-control":
    # Cache type will be overridden by the type of permission table
    warn("The cache is overridden with flat tables cache")
    cache_type = ClassicPrivateL1PrivateL2SharedL3CacheHierarchyWChecks(
        l1d_size="32KiB",
        l1i_size="32KiB",
        l2_size="1MiB",
        l3_size="16MiB",
    )
    cache_type.permission_table.model_name = "space-control"

    # flat tables consume a lot of storage. this needs to be modeled correctly.
    # The host is needed to be specified to figure our where is the repeated entry
    cache_type.get_permission_table().host_id = args.instance

    # configure the permission table for flat tables control
    cache_type.get_permission_table().enable_permission_check = True

    # We'll get to this later.
    cache_type.get_permission_table().use_dedicated_caching = False
    # make sure that the permission parameters are setup correctly.
    cache_type.get_permission_table().permission_base_addr = 0x8C0000000 # 35 GiB
    cache_type.get_permission_table().remote_memory_start = 0x500000000
    cache_type.get_permission_table().local_memory_start =  0x100000000
    cache_type.get_permission_table().local_memory_end =  0x500000000

    # Number of entries is used to override the class contructor.
    # cache_hierarchy.get_permission_table().number_of_entries = (0x800000000 / (2 ** 12))

    cache_type.get_permission_table().binary_search = True
    # using parameters from the driver. After the cacheline version is finished,
    # this latency is drastically reduced!
    cache_type.get_permission_table().permission_entry_size = 64

    cache_type.get_permission_table().total_memory_size = 0x400000000
    cache_type.get_permission_table().mshr_count = 1024
    cache_type.get_permission_table().number_of_entries = int(
                                                0x800000000 / 0x1000)
    cache_type.get_permission_table().segment_size = 64
elif args.permission == "mondrain":
    # Cache type will be overridden by the type of permission table
    warn("The cache is overridden with flat tables cache")
    cache_type = ClassicPrivateL1PrivateL2SharedL3CacheHierarchyWChecks(
        l1d_size="32KiB",
        l1i_size="32KiB",
        l2_size="1MiB",
        l3_size="16MiB",
    )
    cache_type.permission_table.model_name = "mondrain"

    # flat tables consume a lot of storage. this needs to be modeled correctly.
    # The host is needed to be specified to figure our where is the repeated entry
    cache_type.get_permission_table().host_id = args.instance

    # configure the permission table for flat tables control
    cache_type.get_permission_table().enable_permission_check = True

    # We'll get to this later.
    cache_type.get_permission_table().use_dedicated_caching = False
    # make sure that the permission parameters are setup correctly.
    cache_type.get_permission_table().permission_base_addr = 0x8C0000000 # 35 GiB
    cache_type.get_permission_table().remote_memory_start = 0x500000000
    cache_type.get_permission_table().local_memory_start =  0x100000000
    cache_type.get_permission_table().local_memory_end =  0x500000000

    # Number of entries is used to override the class contructor.
    # cache_hierarchy.get_permission_table().number_of_entries = (0x800000000 / (2 ** 12))

    cache_type.get_permission_table().binary_search = True
    # using parameters from the driver. After the cacheline version is finished,
    # this latency is drastically reduced!
    cache_type.get_permission_table().permission_entry_size = 64

    cache_type.get_permission_table().total_memory_size = 0x400000000
    cache_type.get_permission_table().mshr_count = 1024
    cache_type.get_permission_table().number_of_entries = 1
    cache_type.get_permission_table().segment_size = 64
"""

local_mem = {"ddr3": DualChannelDDR3_1600(size=args.local_memory_size),
             "": DualChannelDDR4_2400(size=args.local_memory_size),
             "ddr4": DualChannelDDR4_2400(size=args.local_memory_size),
             "hbm": None,
             "ddr5": None}[args.local_memory_type]
# Memory: Dual Channel DDR4 2400 DRAM device.
# The space control board uses a hack to override the 3 GiB memory limitation
warn("local memory is overridden. hardcoded for experiments")
local_mem = DualChannelDDR4_2400(size="16GiB")


remote_mem_start = int(args.remote_memory_start, 16)
remote_mem_end = int(args.remote_memory_end, 16)
remote_memory_range = AddrRange(remote_mem_start, remote_mem_end)

print(remote_mem_start, args.remote_memory_start, remote_memory_range.start)

shared_memory = {"true": True, "false": False}[args.remote_memory_shared]

# Check if the cmd is from SST. We ignore it.
if args.cmd != "":
    print(args.cmd)
    cmd = args.cmd.split("__fmt__")
else:
    cmd = ""

# Make sure that the resoure paths are not overwritten
if args.disk_path == "":
    args.disk_path = \
        "/home/kaustavg/projects/gem5-resources/src/benchmarks/x86/shared-simple-graphs/x86-disk-image-24-04/x86-ubuntu"

if args.kernel_path == "":
    args.kernel_path = "/home/kaustavg/kernel/x86/linux-6.9.9/vmlinux"


# This runs a check to ensure the gem5 binary is compiled for ARM.
requires(isa_required=ISA.X86)

processor = SimpleProcessor(cpu_type=core_to_use, isa=ISA.X86,
                            num_cores=int(args.core_count))

cache_hierarchy = cache_type

local_memory = local_mem
remote_memory = ExternalRemoteMemory(
    addr_range=remote_memory_range, use_sst_sim=use_sst, host_id=args.instance
)

board = None
# Here we setup the board which allows us to do Full-System ARM simulations.
if shared_memory == False:
    board = X86ComposableMemoryBoard(
        clk_freq=core_freq,
        processor=processor,
        cache_hierarchy=cache_hierarchy,
        local_memory=local_memory,
        remote_memory=remote_memory,
    )
else:
    # inherits from the ComposableMemory board
    board = X86SpaceControlBoard(
        clk_freq=core_freq,
        processor=processor,
        cache_hierarchy=cache_hierarchy,
        local_memory=local_memory,
        remote_memory=remote_memory,
        remote_memory_address_range=remote_memory_range
    )
    # setup the permissions on the board instead of the caches
    # Cache type will be overridden by the type of permission table
    warn("The cache is overridden with flat tables cache") 
    board.get_permission_table().model_name = "space-control"

    # flat tables consume a lot of storage. this needs to be modeled correctly.
    # The host is needed to be specified to figure our where is the repeated entry
    board.get_permission_table().host_id = args.instance

    # configure the permission table for flat tables control
    board.get_permission_table().enable_permission_check = True

    # We'll get to this later.
    board.get_permission_table().use_dedicated_caching = False
    # make sure that the permission parameters are setup correctly.
    board.get_permission_table().permission_base_addr = 0x8C0000000 # 35 GiB
    board.get_permission_table().remote_memory_start = 0x500000000
    board.get_permission_table().local_memory_start =  0x100000000
    board.get_permission_table().local_memory_end =  0x500000000

    # Number of entries is used to override the class contructor.
    # cache_hierarchy.get_permission_table().number_of_entries = (0x800000000 / (2 ** 12))

    board.get_permission_table().binary_search = True
    # using parameters from the driver. After the cacheline version is finished,
    # this latency is drastically reduced!
    board.get_permission_table().permission_entry_size = 64

    board.get_permission_table().total_memory_size = 0x400000000
    board.get_permission_table().mshr_count = 1024
    board.get_permission_table().number_of_entries = int(
                                                0x800000000 / 0x1000)
    board.get_permission_table().segment_size = 64


workload = CustomWorkload(
    function="set_kernel_disk_workload",
    parameters={
        "kernel": CustomResource(args.kernel_path),
        "disk_image": DiskImageResource(args.disk_path, root_partition="2"),
        "readfile_contents": " ".join(cmd),
    },
)

ckpt_to_read_write = ""
if use_sst == False:
    ckpt_to_read_write = m5.options.outdir + "/checkpoint"
    # inform the user where the checkpoint will be saved
    print("Checkpoint will be saved in " + ckpt_to_read_write)
else:
    # Now, when this script is called from SST, we need to restore the
    # checkpoint and proceed towards finishing the simulation.
    ckpt_to_read_write = m5.options.outdir + "/checkpoint"
    print(ckpt_to_read_write)
    # raise NotImplementedError
    # assert args.ckpt_file != ""
    # ckpt_to_read_write = args.ckpt_file

# This disk image needs to have NUMA tools installed.
board.set_workload(workload)

# This script will boot two NUMA nodes in a full system simulation where the
# gem5 node will be sending instructions to the SST node. the simulation will
# after displaying numastat information on the terminal, which can be viewed
# from board.terminal.
board._pre_instantiate()
root = Root(full_system=True, board=board)
board._post_instantiate()


# define on_exit_event
def handle_exit():
    yield True  # Stop the simulation. We're done.


# Here are the different scenarios:
# no checkpoint, run everything in gem5
if use_sst == False:
    root.sim_quantum = int(1e9)
    m5.instantiate()

    # probably this script is being called only in gem5. Since we are not using
    # the simulator module, we might have to add more m5.simulate()
    m5.simulate()
    m5.simulate()
    m5.simulate()
    if ckpt_to_read_write != "":
        m5.checkpoint(os.path.join(os.getcwd(), ckpt_to_read_write))
else:
    # This is called in SST. SST will take care of running this script.
    # Instantiate the system regardless of the simulator.
    m5.instantiate(ckpt_to_read_write)

    # we can still use gem5. So making another if-else
    if use_sst == False:
        m5.simulate()
    # otherwise just let SST do the simulation.
