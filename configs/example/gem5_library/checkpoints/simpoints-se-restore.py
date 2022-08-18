import imp
from gem5.simulate.exit_event import ExitEvent
from gem5.simulate.simulator import Simulator
from gem5.utils.requires import requires
from gem5.components.cachehierarchies.classic.\
    private_l1_private_l2_cache_hierarchy import(
    PrivateL1PrivateL2CacheHierarchy
)
from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.memory import DualChannelDDR4_2400
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.resources.resource import Resource

from gem5.components.processors.simpoint import SimPoint
from pathlib import Path
from m5.stats import reset, dump

requires(isa_required=ISA.X86)

# The cache hierarchy can be different from the cache hierarchy used in taking
# the checkpoints
cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
    l1d_size = "32kB",
    l1i_size="32kB",
    l2_size="256kB",
)

# The memory structure can be different from the memory structure used in
# taking the checkpoints, but the size of the memory must be maintained
memory = DualChannelDDR4_2400(size = "2GB")

processor = SimpleProcessor(
    cpu_type=CPUTypes.TIMING,
    isa=ISA.X86,
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
    arguments = ['print this', 15000]
)

dir = Path("se_checkpoint_folder/cpt.10789598601").as_posix()

def max_inst():
    warmed_up = False
    while True:
        if warmed_up:
            print("end of SimPoint interval")
            yield True
        else:
            print("end of warmup, starting to simulate SimPoint")
            warmed_up = True
            # schedule a MAX_INSTS exit event during the simulation
            simulator.schedule_max_insts(simpoint.get_simpoint_interval())
            dump()
            reset()
            yield False

simulator = Simulator(
    board=board,
    checkpoint_path=dir,
    on_exit_event={
        ExitEvent.MAX_INSTS: max_inst()
    }
)

# schedule a MAX_INSTS exit event before the simulation begins
# the schedule_max_insts function only schedule event when the instruction length is greater than 0
simulator.schedule_max_insts(simpoint.get_warmup_list()[3], True)
simulator.run()
