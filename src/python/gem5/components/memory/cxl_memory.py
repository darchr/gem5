from typing import (
    List,
    Sequence,
    Tuple,
)

from m5.objects import (
    AddrRange,
    MemCtrl,
    Port,
    CXLHostPort,
)

from .abstract_memory_system import AbstractMemorySystem
from ...utils.override import overrides

class CXLMemory(AbstractMemorySystem):
    """ A class to implement the CXL Memory system

    """

    def __init__(self)-> None:
        super().__init__()
        self.cxl_host_port = CXLHostPort(request_queue_size=-1, request_issue_width=-1)

    @overrides(AbstractMemorySystem)
    def incorporate_memory(self, board: AbstractBoard) -> None:
        """This function completes all of the necessary steps to add this
        memory system to the board."""
        raise NotImplementedError

    @overrides(AbstractMemorySystem)
    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return [(self.cxl_host_port.mem_ranges, self.cxl_host_port.getPort())]

    @overrides(AbstractMemorySystem)
    def get_memory_controllers(self) -> List[MemCtrl]:
        return [self.cxl_host_port]

    @overrides(AbstractMemorySystem)
    def get_mem_interfaces(self) -> List[MemInterface]:
        """Get all memory interfaces in this memory system.
        Useful when creating physical memory objects."""
        raise Exception("No memory in the CXLHostPort")

    @overrides(AbstractMemorySystem)
    def get_size(self) -> int:
        """Returns the total size of the memory system."""
        raise NotImplementedError

    @overrides(AbstractMemorySystem)
    def set_memory_range(self, ranges: List[AddrRange]) -> None:
        """Set the total range for this memory system.

        May pass multiple non-overlapping ranges. The total size of the ranges
        should match the size of the memory.

        If this memory system is incompatible with the ranges, an exception
        will be raised.
        """

        self.cxl_host_port.mem_ranges = ranges

    @overrides(AbstractMemorySystem)
    def get_uninterleaved_range(self) -> List[AddrRange]:
        """Returns the range of the memory system without interleaving.
        This is useful when other components in the system want to interleave
        the memory range different to how the memory has interleaved them.
        """
        return self.cxl_host_port.mem_ranges