from typing import (
    List,
    Sequence,
    Tuple,
)

from m5.objects import (
    AddrRange,
    Port,
    SysBridge,
    System,
)
from m5.util.fdthelper import (
    FdtNode,
    FdtProperty,
    FdtPropertyStrings,
    FdtPropertyWords,
)

from gem5.components.boards.arm_board import ArmBoard
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.processors.abstract_processor import AbstractProcessor


class RemoteArmBoard(ArmBoard):
    """This class acts just like the arm board but it allows you to connect
    a remote memory system to the board.

    The added function in `add_remote_memory` which takes a memory system.
    """

    def __init__(
        self,
        clk_freq: str,
        processor: AbstractProcessor,
        memory: AbstractMemorySystem,
        cache_hierarchy: AbstractCacheHierarchy,
        **kwargs,
    ) -> None:
        super().__init__(
            clk_freq=clk_freq,
            processor=processor,
            memory=memory,
            cache_hierarchy=cache_hierarchy,
            **kwargs,
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
        self.system_bridges = [
            SysBridge(
                source=self,
                target=remote_system,
                target_port=remote_port,
            )
            for _, remote_port in remote_memory.get_mem_ports()
        ]
        self._remote_memory_ports = [
            (rng, bridge.source_port)
            for (rng, rm_port), bridge in zip(
                remote_memory.get_mem_ports(), self.system_bridges
            )
        ]

    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        print("Getting the memory ports")
        return self.get_memory().get_mem_ports() + self._remote_memory_ports

    def generateDeviceTree(self, state):
        # Generate a device tree root node for the system by creating the root
        # node and adding the generated subnodes of all children.
        # When a child needs to add multiple nodes, this is done by also
        # creating a node called '/' which will then be merged with the
        # root instead of appended.

        def generateMemNode(mem_range):
            node = FdtNode(f"memory@{int(mem_range.start):x}")
            node.append(FdtPropertyStrings("device_type", ["memory"]))
            node.append(
                FdtPropertyWords(
                    "reg",
                    state.addrCells(mem_range.start)
                    + state.sizeCells(mem_range.size()),
                )
            )
            return node

        root = FdtNode("/")
        root.append(state.addrCellsProperty())
        root.append(state.sizeCellsProperty())

        # Add memory nodes
        for mem_range in self.mem_ranges:
            root.append(generateMemNode(mem_range))

        for node in self.recurseDeviceTree(state):
            # Merge root nodes instead of adding them (for children
            # that need to add multiple root level nodes)
            if node.get_name() == root.get_name():
                root.merge(node)
            else:
                root.append(node)

        for rng, _ in self._remote_memory_ports:
            node = FdtNode(f"pmem@{int(rng.start):x}")
            node.append(FdtPropertyStrings("compatible", ["pmem-region"]))
            node.append(
                FdtPropertyWords(
                    "reg",
                    state.addrCells(rng.start) + state.sizeCells(rng.size()),
                )
            )
            node.append(FdtProperty("volatile"))
            root.append(node)

        return root
