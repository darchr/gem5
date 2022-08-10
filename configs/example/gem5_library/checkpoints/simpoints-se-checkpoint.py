from gem5.simulate.exit_event import ExitEvent
from gem5.simulate.simulator import Simulator
from gem5.utils.requires import requires
from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.memory.single_channel import SingleChannelDDR3_1600
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.resources.resource import Resource
from pathlib import Path
from gem5.components.cachehierarchies.classic.no_cache import NoCache
from gem5.components.processors.simpoint import SimPoint
from gem5.simulate.exit_event_generators import (
    simpoint_save_checkpoint_generator,
)

requires(isa_required=ISA.X86)

# When taking a checkpoint, the cache state is not saved, so the cache
# hierarchy can be changed completely when restoring from a checkpoint.
# By using NoCache() to take checkpoints, it can slightly improve the
# performance when running in atomic mode, and it will not put any restrictions
# on what people can do with the checkpoints.
cache_hierarchy = NoCache()

# Using simple memory to take checkpoints might slightly imporve the
# performance in atomic mode. The memory structure can be changed when
# restoring from a checkpoint, but the size of the memory must be maintained.
memory = SingleChannelDDR3_1600(size = "2GB")

processor = SimpleProcessor(
    cpu_type=CPUTypes.ATOMIC,
    isa=ISA.X86,
    # SimPoints only works with one core
    num_cores=1,
)

board = SimpleBoard(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

simpoint = SimPoint(
    simpoint_list = [2, 3, 5, 15],
    weight_list = [0.1, 0.2, 0.4, 0.3],
    simpoint_interval = 1000000,
    warmup_interval = 1000000
    # simpoint_file_path=Path("path/to/simpoints"),
    # weight_file_path=Path("path/to/weights"),
)

board.set_se_binary_workload(
    binary = Resource('x86-print-this'),
    arguments = ['print this', 15000],
    simpoint = simpoint
)

dir = Path("se_checkpoint_folder/")
dir.mkdir(exist_ok=True)

simulator = Simulator(
    board=board,
    on_exit_event={
        # using the SimPoints event generator in the standard library to take
        # checkpoints
        ExitEvent.SIMPOINT_BEGIN: simpoint_save_checkpoint_generator(dir)
    }
)

simulator.run()
