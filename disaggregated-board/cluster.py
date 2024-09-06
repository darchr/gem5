from typing import (
    Sequence,
    Tuple,
)

from m5.objects import (
    AddrRange,
    NoncoherentXBar,
    Port,
    SrcClockDomain,
    SubSystem,
    System,
    VoltageDomain,
)

from gem5.components.memory.dram_interfaces.ddr4 import DDR4_2400_8x8
from gem5.components.memory.memory import ChanneledMemory


class RemoteMemory(ChanneledMemory):
    """
    This is a normal memory system except that it has a xbar in front of the
    memory.

    Instead of returning the ports for each channeled memory, this object has
    a xbar. This allows *multiple requestors* (e.g., "hosts") to be connected
    to this remote memory. We use a non coherent xbar for now (no hardware
    coherence).
    """

    def __init__(self, size: str, start: str) -> None:
        super().__init__(DDR4_2400_8x8, 1, 64, size=size)
        self.set_memory_range([AddrRange(start=start, size=size)])
        self.xbar = NoncoherentXBar(
            width=64,
            frontend_latency=1,
            forward_latency=1,
            response_latency=1,
        )
        for _, port in super().get_mem_ports():
            self.xbar.mem_side_ports = port

    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return [(self.get_uninterleaved_range()[0], self.xbar.cpu_side_ports)]


class Cluster(SubSystem):
    """
    A `Cluster` can act like an `AbstractBoard` but has multiple sub-boards.

    These "sub-boards" are like CXL hosts. Each one is independent and runs
    independent software/OS/etc.

    This is abusing duck typing. It looks like an AbstractBoard, but isn't

    LIMITATIONS: Currently does not support boards with KVM cores.
    """

    def __init__(self, boards, remote_memory):
        super().__init__()
        self.boards = boards
        self._checkpoint = False

        self.mem_system = System()
        self.mem_system.clk_domain = SrcClockDomain()
        self.mem_system.clk_domain.clock = "1GHz"
        self.mem_system.clk_domain.voltage_domain = VoltageDomain()
        self.mem_system.mem_mode = "timing"  # Use timing accesses
        self.mem_system.memory = remote_memory

        for board in self.boards:
            board.add_remote_memory(
                self.mem_system.memory, self.mem_system
            )

    def __getattr__(self, attr):
        # This can be removed after Bobby moves the kvm stuff to
        # _pre_instantiate.
        if attr == "processor":

            class Empty:
                # More duck typing for the kvm hack
                pass

            processor = Empty()
            core = Empty()
            core.is_kvm_core = lambda: False
            processor.get_cores = lambda: [core]
            return processor
        return super().__getattr__(attr)

    def get_processor(self):
        # This is a hack for the default exit events for generators.
        return None

    def is_fullsystem(self):
        return True

    def _pre_instantiate(self):
        for board in self.boards:
            board._pre_instantiate()

    def _post_instantiate(self):
        for board in self.boards:
            board._post_instantiate()
