from ossaudiodev import SNDCTL_MIDI_MPUMODE
import m5
from gem5.isas import ISA
from gem5.simulate.exit_event import ExitEvent
from gem5.utils.requires import requires
from gem5.resources.resource import Resource, CustomResource
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.simulate.simulator import Simulator
from gem5.components.memory import DualChannelDDR4_2400
from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import(
    PrivateL1PrivateL2CacheHierarchy
)

requires(isa_required=ISA.ARM)


cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
    l1d_size = "32kB",
    l1i_size="32kB",
    l2_size="256kB",
)

memory = DualChannelDDR4_2400(size = "3GB")

processor = SimpleProcessor(
    cpu_type= CPUTypes.ATOMIC,
    isa = ISA.ARM,
    num_cores= 1
)

processor.cores[0].core.addSimPointProbe(1000000)

board = SimpleBoard(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

board.set_se_binary_workload(
    binary = CustomResource("Enter_File_Path_Here"),
    #arguments=[]
)

simulator = Simulator(
    full_system=False, 
    board=board,
    )


simulator.run()

print(
    "Exiting @ tick {} because {}.".format(
        simulator.get_current_tick(),
        simulator.get_last_exit_event_cause(),
    )
)
