from typing import (
    List,
    Optional,
    Sequence,
    Tuple,
)

from m5.objects import (
    AddrRange,
    CXL_CXLHost_Controller,
    CXLHostPort,
    MemCtrl,
    MemInterface,
    MessageBuffer,
    Port,
    RubySystem,
)
from m5.util.convert import toMemorySize

from .abstract_memory_system import AbstractMemorySystem
from ..boards.abstract_board import AbstractBoard
from ...utils.override import overrides


class CXLMemory(AbstractMemorySystem):
    """A class to implement the CXL Memory system"""

    def __init__(self, size: Optional[str] = "4GiB") -> None:
        super().__init__()
        self._size = toMemorySize(size)
        self.cxl_host_port = CXLHostPort(
            request_latency=1,
            response_latency=1,
            request_queue_size=-1,
            request_issue_width=-1,
        )
        self.cxl_host_port.controller = CXL_CXLHost_Controller(
            hostPort=self.cxl_host_port,
            mandatoryQueue=MessageBuffer(),
            version=1,
        )

    @overrides(AbstractMemorySystem)
    def incorporate_memory(self, board: AbstractBoard) -> None:
        Warning("CXLMemory does not have backing store memory.")
        pass

    @overrides(AbstractMemorySystem)
    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return [
            (self.cxl_host_port.mem_ranges, self.cxl_host_port.host_side_port)
        ]

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
        return self._size

    @overrides(AbstractMemorySystem)
    def set_memory_range(self, ranges: List[AddrRange]) -> None:
        # We check this because of the assumption in get_size
        # that there is only one range. This is temporary
        if len(ranges) != 1:
            raise RuntimeError(
                "CXLMemory only supports a single range. "
                f"Got {len(ranges)} ranges."
            )
        self.cxl_host_port.mem_ranges = ranges

    @overrides(AbstractMemorySystem)
    def get_uninterleaved_range(self) -> List[AddrRange]:
        """Returns the range of the memory system without interleaving.
        This is useful when other components in the system want to interleave
        the memory range different to how the memory has interleaved them.
        """
        return self.cxl_host_port.mem_ranges

    def set_ruby_system(self, ruby_system: RubySystem) -> None:
        """Set the Ruby system for this memory system."""
        self.cxl_host_port.ruby_system = ruby_system
        self.cxl_host_port.controller.ruby_system = ruby_system
