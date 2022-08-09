from gem5.simulate.exit_event import ExitEvent
from gem5.simulate.simulator import Simulator
from gem5.utils.requires import requires
from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.memory import DualChannelDDR4_2400
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.resources.resource import Resource
from pathlib import Path
from gem5.components.cachehierarchies.classic.\
    private_l1_private_l2_cache_hierarchy import(
    PrivateL1PrivateL2CacheHierarchy
)

from gem5.components.processors.simpoint import SimPoint
from gem5.simulate.exit_event_generators import (
    simpoint_save_checkpoint_generator,
)

requires(isa_required=ISA.X86)

cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
    l1d_size = "32kB",
    l1i_size="32kB",
    l2_size="256kB",
)

memory = DualChannelDDR4_2400(size = "3GB")

processor = SimpleProcessor(
    cpu_type=CPUTypes.ATOMIC,
    isa=ISA.X86,
    num_cores=1,
)

board = SimpleBoard(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

command= "/home/gem5/NPB3.3-OMP/bin/bt.A.x;"\
    + "sleep 5;" \
    + "m5 exit;"

simpoint = SimPoint(
    simpoint_list = [3,5,15],
    weight_list = [0.1,0.5,0.4],
    simpoint_interval = 1000000,
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
