import m5
import argparse
from m5.objects import *

from gem5.components.boards.test_board import TestBoard
from gem5.components.memory.hbm import HighBandwidthMemory
from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.memory.dram_interfaces.hbm import HBM_2000_4H_1x64

from gem5.components.cachehierarchies.ruby.mesi_two_level_cache_hierarchy import (
    MESITwoLevelCacheHierarchy,
)

from gem5.components.cachehierarchies.classic.private_l1_shared_l2_cache_hierarchy import(
    PrivateL1SharedL2CacheHierarchy,
)

from gem5.simulate.simulator import Simulator

from dr_trace_player import DRTracePlayerGenerator

parser = argparse.ArgumentParser(
    description="A script to run google traces."
)

benchmark_choices = ["charlie", "delta", "merced", "whiskey"]

parser.add_argument(
    "--path",
    type=str,
    required=True,
    help="Main directory containing the traces.",
)

parser.add_argument(
    "--workload",
    type=str,
    required=True,
    help="Input the benchmark program to execute.",
    choices=benchmark_choices,
)

parser.add_argument(
    "--players",
    type=int,
    required=True,
    help="Input the number of players to use.",
)

parser.add_argument(
    "--ruby",
    type=int,
    required=True,
    help="Use with ruby or classic caches",
)


args = parser.parse_args()

generator = DRTracePlayerGenerator(
    "{}/{}/".format(args.path, args.workload),
    num_cores=8,
    max_ipc=8,
    max_outstanding_reqs=16,
)

if args.ruby == 1:
    cache_hierarchy = MESITwoLevelCacheHierarchy(
        l1d_size="512kB",
        l1d_assoc=8,
        l1i_size="32kB",
        l1i_assoc=2,
        l2_size="1MB",
        l2_assoc=16,
        num_l2_banks=8,
    )
elif args.ruby == 0:
    cache_hierarchy = PrivateL1SharedL2CacheHierarchy(
        l1d_size="512kB",
        l1d_assoc=8,
        l1i_size="32kB",
        l1i_assoc=2,
        l2_size="1MB",
        l2_assoc=16,
    )
else:
    print("WRONG RUBY OPTION")
    exit()

memory = SingleChannelDDR4_2400(size="3GB")

board = TestBoard(
    clk_freq="5GHz",  # Ignored for these generators
    generator=generator,  # We pass the traffic generator as the processor.
    memory=memory,
    # With no cache hierarchy the test board will directly connect the
    # generator to the memory
    cache_hierarchy=cache_hierarchy,
)

simulator = Simulator(board=board)
simulator.run(100000000000)
