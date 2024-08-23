"""
I thought this would be useful, but I found a better way. The code may still
be useful in the future, though.
"""

import itertools
from typing import (
    List,
    Sequence,
    Tuple,
)

from m5.objects import (
    AddrRange,
    MemCtrl,
    Port,
)

from gem5.components.memory.abstract_memory_system import AbstractMemorySystem


class MultiPortMemory(AbstractMemorySystem):
    """This allows you to combine multiple memory systems into one.

    The ports from each memory system will be combined into one list of ports.
    """

    def __init__(self, memories: List[AbstractMemorySystem]) -> None:
        super().__init__()
        self.memories = memories

    def incorporate_memory(self, board: AbstractBoard) -> None:
        self._initialized = True
        for memory in self.memories:
            memory.incorporate_memory(board)

    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        all_ports = list(
            itertools.chain.from_iterable(
                memory.get_mem_ports() for memory in self.memories
            )
        )
        return all_ports

    def get_memory_controllers(self) -> List[MemCtrl]:
        all_ctrls = list(
            itertools.chain.from_iterable(
                memory.get_memory_controllers() for memory in self.memories
            )
        )
        return all_ctrls

    def get_size(self) -> int:
        return sum(memory.get_size() for memory in self.memories)

    def set_memory_range(self, ranges: List[AddrRange]) -> None:
        if len(ranges) != len(self.memories):
            raise Exception(
                "The number of ranges must match the number of memories."
            )
        for memory, range in zip(self.memories, ranges):
            memory.set_memory_range([range])

    def get_uninterleaved_range(self) -> List[AddrRange]:
        all_ranges = list(
            itertools.chain.from_iterable(
                memory.get_uninterleaved_range() for memory in self.memories
            )
        )
        return all_ranges
