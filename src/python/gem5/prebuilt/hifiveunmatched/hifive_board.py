from gem5.components.memory.dram_interfaces.ddr4 import DDR4_2400_8x8
from gem5.components.memory.memory import ChanneledMemory
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.utils.requires import requires
from gem5.isas import ISA
from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.processors.cpu_types import CPUTypes, CustomCPUTypes
from hifive_cache import HiFiveCacheHierarchy
from hifive_proc import U74Processor
from m5.objects import AddrRange

class HiFiveUnmatchedBoard(SimpleBoard):
    def __init__(self) -> None:
        requires(isa_required=ISA.RISCV)

        cache_hierarchy = HiFiveCacheHierarchy(
            l1d_size="32kB", l1i_size="32kB", l2_size="2MB"
        )

        memory = ChanneledMemory(DDR4_2400_8x8, 1, 64, "16GB")
        memory.set_memory_range(
            [AddrRange(start=0x80000000, size=memory.get_size())])

        processor = U74Processor()

        super().__init__(
            clk_freq="1.2GHz", # real system is 1.0 to 1.5 GHz
            processor=processor,
            memory=memory,
            cache_hierarchy=cache_hierarchy,
        )
