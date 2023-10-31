# Copyright (c) 2021-2023 The Regents of the University of California
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
This script is used for running a traffic generator connected to a memory
device. It supports linear and random accesses with a configurable amount
of write traffic.

By default, this scripts runs with one channel (two pseudo channels) of HBM2
and this channel is driven with 32GiB/s of traffic for 1ms.
"""

import argparse

from m5.objects import MemorySize, AddrRange

# from gem5.components.boards.test_board import TestBoard

from test_board_sst import TestBoardForSST

from gem5.components.processors.linear_generator import LinearGenerator
from gem5.components.processors.random_generator import RandomGenerator

from gem5.components.memory.hbm import HighBandwidthMemory
from gem5.components.memory.dram_interfaces.hbm import HBM_2000_4H_1x64

from gem5.simulate.simulator import Simulator

# For hooking up SST with this system.
from m5.objects import OutgoingRequestBridge


def generator_factory(
    generator_class: str, rd_perc: int, mem_size: MemorySize
):
    rd_perc = int(rd_perc)
    if rd_perc > 100 or rd_perc < 0:
        raise ValueError(
            "Read percentage has to be an integer number between 0 and 100."
        )
    if generator_class == "LinearGenerator":
        return LinearGenerator(
            duration="1ms", rate="32GiB/s", max_addr=mem_size, rd_perc=rd_perc
        )
    elif generator_class == "RandomGenerator":
        return RandomGenerator(
            duration="1ms", rate="32GiB/s", max_addr=mem_size, rd_perc=rd_perc
        )
    else:
        raise ValueError(f"Unknown generator class {generator_class}")


parser = argparse.ArgumentParser(
    description="A traffic generator that can be used to test a gem5 "
    "memory component."
)

parser.add_argument(
    "--generator-class",
    type=str,
    help="The class of generator to use.",
    choices=[
        "LinearGenerator",
        "RandomGenerator",
    ],
    default="LinearGenerator",
)

parser.add_argument(
    "--memory-size", type=str, help="Memory size as a string", default="1GiB"
)

parser.add_argument(
    "--read-percentage",
    type=int,
    help="Percentage of read requests in the generated traffic.",
    default=100,
)


args = parser.parse_args()

# Single pair of HBM2 pseudo channels. This can be replaced with any
# single ported memory device
# memory = HighBandwidthMemory(HBM_2000_4H_1x64, 1, 128)
memory_size = args.memory_size
# sst_memory = OutgoingRequestBridge(physical_address_ranges = AddrRange(start = 0x0, size = memory_size))

# print("mem-size: ", str(sst_memory.physical_address_ranges[0])[2:])

generator = generator_factory(
    args.generator_class,
    args.read_percentage,
    int(str(AddrRange(0x0, memory_size))[2:]),
)

# We use the Test Board. This is a special board to run traffic generation
# tasks. Can replace the cache_hierarchy with any hierarchy to simulate the
# cache as well as the memory
board = TestBoardForSST(
    clk_freq="1GHz",  # Ignored for these generators
    generator=generator,  # We pass the traffic generator as the processor.
    # memory=sst_memory,
    remote_memory_size=memory_size,
    memory=None,
    # With no cache hierarchy the test board will directly connect the
    # generator to the memory
    cache_hierarchy=None,
)
board._pre_instantiate()
root = Root(full_system=True, system=board)
# simulator = Simulator(board=board)
# simulator.run()
