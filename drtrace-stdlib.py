from gem5.components.boards.test_board import TestBoard
from gem5.components.memory.hbm import HighBandwidthMemory
from gem5.components.memory.dram_interfaces.hbm import HBM_2000_4H_1x64

from gem5.components.cachehierarchies.chi.private_l1_cache_hierarchy import (
    PrivateL1CacheHierarchy,
)

from gem5.simulate.simulator import Simulator

from dr_trace_player import DRTracePlayerGenerator


generator = DRTracePlayerGenerator(
    "/projects/google-traces/delta/",
    num_cores=16,
    max_ipc=1,
    max_outstanding_reqs=16,
)

memory = HighBandwidthMemory(HBM_2000_4H_1x64, 1, 128)

board = TestBoard(
    clk_freq="1GHz",  # Ignored for these generators
    generator=generator,  # We pass the traffic generator as the processor.
    memory=memory,
    # With no cache hierarchy the test board will directly connect the
    # generator to the memory
    cache_hierarchy=PrivateL1CacheHierarchy("32KiB", 8),
)

simulator = Simulator(board=board)
simulator.run()
