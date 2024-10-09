from typing import (
    Sequence,
    Tuple,
)

from m5.objects import (
    AddrRange,
    SimpleMemDelay,
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
        self._num_boards = 0
        self.set_memory_range([AddrRange(start=start, size=size)])
        self.xbar = NoncoherentXBar(
            width=64,
            frontend_latency=1,
            forward_latency=1,
            response_latency=1,
        )
        for _, port in super().get_mem_ports():
            self.xbar.mem_side_ports = port

    def set_num_boards(self, num_boards: int) -> None:
        self._num_boards = num_boards

        # Create a memory delay for each board.
        # The reason we do this here is so that these objects are in the
        # remote memory system's sub-system and use that thread's eventq.
        self.mem_delays = [
            SimpleMemDelay(read_req="100ns", read_resp="0ns",
                           write_req="100ns", write_resp="0ns",
                           mem_side_port=self.xbar.cpu_side_ports)
            for _ in range(self._num_boards)
        ]

    def get_mem_system_for_board(self, board_index: int):
        """This returns a MemorySystem-like object that has the
        `get_mem_ports` function. This is used by the `RemoteMemoryX86Board`
        It needs to be wierd like this since each board is going to connect
        to a different port.
        """
        class MemorySystem:
            def __init__(self, rng, port):
                self.rng = rng
                self.port = port
            def get_mem_ports(self):
                return [(self.rng, self.port)]
        return MemorySystem(self.get_uninterleaved_range()[0],
                            self.mem_delays[board_index].cpu_side_port)

    # def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
    #     return [(self.get_uninterleaved_range()[0], self.xbar.cpu_side_ports)]


class Cluster(SubSystem):
    """
    A `Cluster` can act like an `AbstractBoard` but has multiple sub-boards.

    These "sub-boards" are like CXL hosts. Each one is independent and runs
    independent software/OS/etc.

    This is abusing duck typing. It looks like an AbstractBoard, but isn't

    LIMITATIONS: Currently does not support boards with KVM cores.
    """

    def __init__(self, boards, remote_memory, parallel=False):
        super().__init__()
        self.boards = boards
        self._checkpoint = False
        self._parallel = parallel

        self.mem_system = System()
        self.mem_system.clk_domain = SrcClockDomain()
        self.mem_system.clk_domain.clock = "1GHz"
        self.mem_system.clk_domain.voltage_domain = VoltageDomain()
        self.mem_system.mem_mode = "timing"  # Use timing accesses
        self.mem_system.memory = remote_memory
        self.mem_system.memory.set_num_boards(len(boards))

        for i,board in enumerate(self.boards):
            board.add_remote_memory(
                self.mem_system.memory.get_mem_system_for_board(i),
                self.mem_system
            )

    def get_processor(self):
        # This is a hack for the default exit events for generators.
        return None

    def is_fullsystem(self):
        return True

    def _pre_instantiate(self, full_system = None):
        """ This needs to look like an AbstractBoard so that the cluster can
        be used as a board in the simulation. The main cluster is what is
        responsible for creating the root.
        """
        if self._parallel:
            for obj in self.descendants():
                obj.eventq_index = 0
            for i,board in enumerate(self.boards):
                for obj in board.descendants():
                    obj.eventq_index = i + 1

        from m5.objects import Root
        root = Root(full_system=True, board=self)

        if self._parallel:
            root.sim_quantum = 100000

        # Note: The remote boards' _pre_instantiate require a root object
        # unlike the AbstractBoard
        for board in self.boards:
            board._pre_instantiate(root)

        return root

    def _post_instantiate(self):
        for board in self.boards:
            board._post_instantiate()
