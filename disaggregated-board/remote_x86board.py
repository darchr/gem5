from typing import (
    List,
    Sequence,
    Tuple,
)

from m5.objects import (
    AddrRange,
    Port,
    SimpleMemDelay,
    SysBridge,
    System,
    X86E820Entry,
)

from gem5.components.boards.x86_board import X86Board
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.processors.abstract_processor import AbstractProcessor


class RemoteMemoryX86Board(X86Board):
    """This class acts just like the x86 board but it allows you to connect
    a remote memory system to the board.

    The added function in `add_remote_memory` which takes a memory system.
    """

    def __init__(
        self,
        clk_freq: str,
        processor: AbstractProcessor,
        memory: AbstractMemorySystem,
        cache_hierarchy: AbstractCacheHierarchy,
    ) -> None:
        super().__init__(
            clk_freq=clk_freq,
            processor=processor,
            memory=memory,
            cache_hierarchy=cache_hierarchy,
        )
        self._remote_memory_ports = []

    def add_remote_memory(
        self, remote_memory: AbstractMemorySystem, remote_system: System
    ) -> None:
        """Allow this board to access a remote memory system.

        :param: remote_memory the remote memory system. Note: This can be
                *exactly the same* between multiple `RemoteMemoryX86Board`s
        :param: remote_system the system that owns the remote_memory. This is
                is used to set up the RequestorIDs

        A couple of implementation details:
        - The remote memory will be connected to the cache hierarchy just like
          a regular port in the memory system. This means that the xbar or
          directory that is connected to the remote memory port will be
          responsible for the coherence for the remote addresses.
        - Instead of directly connecting things we use a `SysBridge` so that
          the RequestorIDs are correct at the remote memory system.
        """
        # These mem_delays are needed to ensure that the remote memory system
        # and this memory system can safely communicate when running in
        # parallel mode. The CXL-host side adds latency to the responses and
        # the remote memory system adds latency to the requests.
        self.mem_delays = [
            SimpleMemDelay(
                read_req="0ns",
                read_resp="100ns",
                write_req="0ns",
                write_resp="100ns",
                mem_side_port=remote_port,
            )
            for _, remote_port in remote_memory.get_mem_ports()
        ]
        self.system_bridges = [
            SysBridge(
                source=self,
                target=remote_system,
                target_port=mem_delay.cpu_side_port,
            )
            for mem_delay in self.mem_delays
        ]
        self._remote_memory_ports = [
            (rng, bridge.source_port)
            for (rng, rm_port), bridge in zip(
                remote_memory.get_mem_ports(), self.system_bridges
            )
        ]
        self.external_memory_ranges = [
            rng for rng, _ in remote_memory.get_mem_ports()
        ]

    def _pre_instantiate(self, root):
        """Must override AbstractBoard._pre_instantiate since
        root is created by the cluster "board"
        """
        self._connect_things()
        self.get_processor()._pre_instantiate(root)
        self.get_cache_hierarchy()._pre_instantiate(root)
        self.get_memory()._pre_instantiate(root)

    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return self.get_memory().get_mem_ports() + self._remote_memory_ports
