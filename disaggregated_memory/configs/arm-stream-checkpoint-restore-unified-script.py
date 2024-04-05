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

"""
This script shows an example of running a full system ARM Ubuntu boot
simulation using the gem5 library. This simulation boots Ubuntu 20.04 using
1 TIMING CPU cores and executes `STREAM`. The simulation ends when the
startup is completed successfully.
"""

import os
import sys
import argparse

sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
)

import m5

from m5.objects.RealView import VExpress_GEM5_V1

from boards.arm_gem5_board import ArmGem5DMBoard
from boards.arm_sst_board import ArmSstDMBoard
from cachehierarchies.dm_caches import ClassicPrivateL1PrivateL2SharedL3DMCache
from cachehierarchies.dm_caches_sst import ClassicPrivateL1PrivateL2SharedL3SstDMCache
from memories.remote_memory import RemoteChanneledMemory
from memories.external_remote_memory import ExternalRemoteMemory
from gem5.utils.requires import requires
from gem5.components.memory.dram_interfaces.ddr4 import DDR4_2400_8x8
from gem5.components.memory import DualChannelDDR4_2400
from gem5.components.memory.multi_channel import *
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.simulate.simulator import Simulator
from gem5.resources.workload import *
from gem5.resources.resource import *
from m5.objects import AddrRange, Root
from m5.objects import ArmDefaultRelease
from gem5.simulate.exit_event import ExitEvent

def parse_options():

    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--chpt-dir", type=str, default="", required=False,
        help="Optionally, put a path to a checkpoint for restoring"
    )
    parser.add_argument(
        "--take-chpt", type=str,
        help="Set it to true if taking a checkpoint"
    )
    parser.add_argument(
        "--is-composable", type=str,
        help="Set it to true if this setup will be " +
        "used in SST experiments, false otherwise"
    )
    parser.add_argument(
        "--remote-memory-addr-range", type=str, default="",
        help="Remote memory range when using SST",
    )
    parser.add_argument(
        "--remote-memory-latency", type=int,
        help="Remote memory latency in Ticks (has to be converted prior)",
    )
    parser.add_argument(
        "--instance", type=int,
        help="Instance id is need to correctly read and write to the checkpoint." +
        " This is only used when restoring a checkpoint."
    )
    return parser.parse_args()

def create_cmd_list(is_composable):
    ### We create a set of commands to boot with KVM,
    ### exit and take checkpoint,
    ### then restoring with a Timing CPU and
    ## running the STREAM benchmark and exit.
    cmd =  ["mount -t sysfs - /sys;",
            "mount -t proc - /proc;",
            "numastat;",
            "m5 --addr=0x10010000 exit;",
            "ls;",]
    if is_composable == "False":
        cmd = cmd + [
            "numactl --membind=0 -- " +
            "/home/ubuntu/simple-vectorizable-benchmarks/stream/" +
            "stream.hw.m5 1000000;",]
    else:
        cmd = cmd + [
            "numactl --membind=1 -- " +
            "/home/ubuntu/simple-vectorizable-benchmarks/stream/" +
            "stream.hw.m5 1000000;",]
    cmd = cmd + ["numastat;",
                 "m5 --addr=0x10010000 exit;",]
    return cmd

def handle_exit():
    yield True  # Stop the simulation. We're done.

if __name__ == "__m5_main__":
    args = parse_options()

    requires(isa_required=ISA.ARM)

    local_memory = DualChannelDDR4_2400(size="2GiB")

    # Always use a Timing CPU, unless you are taking
    # a checkpoint which you should use KVM to fast-forward
    cpu_type = CPUTypes.TIMING
    if args.take_chpt == "True":
        cpu_type=CPUTypes.KVM
    processor = SimpleProcessor(cpu_type=cpu_type, isa=ISA.ARM, num_cores=4)

    # Create the memory system and the board
    if args.is_composable == "False":
        print("************************************ 1")
        cache_hierarchy = ClassicPrivateL1PrivateL2SharedL3DMCache(
        l1d_size="32KiB", l1i_size="32KiB", l2_size="256KiB", l3_size="4MiB"
        )
        remote_memory = RemoteChanneledMemory(
                DDR4_2400_8x8,
                2,
                64,
                size="32GB",
                remote_offset_latency = args.remote_memory_latency
        )
        board = ArmGem5DMBoard(
        clk_freq="3GHz",
        processor=processor,
        local_memory=local_memory,
        remote_memory=remote_memory,
        cache_hierarchy=cache_hierarchy,
        platform=VExpress_GEM5_V1(),
        release = ArmDefaultRelease.for_kvm()
        )
    else:
        print("************************************ 3")
        cache_hierarchy = ClassicPrivateL1PrivateL2SharedL3SstDMCache(
        l1d_size="32KiB", l1i_size="32KiB", l2_size="256KiB", l3_size="4MiB"
        )
        remote_memory_range = list(map(int, args.remote_memory_addr_range.split(",")))
        remote_memory_range = AddrRange(remote_memory_range[0], remote_memory_range[1])
        remote_memory = ExternalRemoteMemory(
            addr_range = remote_memory_range, link_latency = args.remote_memory_latency
        )
        board = ArmSstDMBoard(
        clk_freq="3GHz",
        processor=processor,
        local_memory=local_memory,
        remote_memory=remote_memory,
        cache_hierarchy=cache_hierarchy,
        platform=VExpress_GEM5_V1(),
        release = ArmDefaultRelease.for_kvm()
        )

    # Make the command lists to be executed.
    cmd = create_cmd_list(args.is_composable)

    print(cpu_type)
    print(cmd)
    print("************************************ 2")

    # Set the kernel, bootloader, disk image and the commands to be executed
    board.set_kernel_disk_workload(
        kernel=CustomResource("/home/kaustavg/vmlinux-5.4.49-NUMA.arm64"),
        bootloader=CustomResource(
                "/home/kaustavg/kernel/arm/bootloader/arm64-bootloader"
        ),
        disk_image=DiskImageResource(
            "/projects/gem5/hn/DISK_IMAGES/arm64-hpc-2204-numa-kvm.img-20240304",
            root_partition="1",
        ),
        readfile_contents=" ".join(cmd),
    )

    if args.take_chpt == "True": # taking a checkpoint
        simulator = Simulator(board=board,
                          on_exit_event={ExitEvent.EXIT: handle_exit()})
        simulator.run()
        print("Finished simulation, now writing the checkpoint")
        simulator.save_checkpoint(m5.options.outdir+"/checkpoint")
        print("Finished writing the checkpoint")
    else: # restoring a checkpoint
        assert(args.chpt_dir != "")
        if args.is_composable == "False": # remote memory is in gem5 node
            simulator = Simulator(board=board,
                    checkpoint_path=os.path.join(os.getcwd(), args.chpt_dir),
                    on_exit_event={ExitEvent.EXIT: handle_exit()})
            print("Done with restoring the checkpoint. Now running the simulation.")
            simulator.run()
        else: # remote memory is in SST node
            board._pre_instantiate()
            root = Root(full_system=True, board=board)
            board._post_instantiate()
            m5.instantiate(args.chpt_dir)