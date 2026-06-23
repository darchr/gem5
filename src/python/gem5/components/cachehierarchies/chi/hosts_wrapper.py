from typing import (
    List,
    Sequence,
    Tuple,
    Type,
)

from m5.objects import (
    AddrRange,
    MemCtrl,
    MemInterface,
    Port,
)

from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem


class HostConfig:
    """
    Configuration for a single host (NUMA node or CXL memory pool) in the system.
    """

    def __init__(
        self,
        host_id: int,
        num_cores: int,
        num_hns: int,
        mem_range: AddrRange,
        memory_cls: Type[AbstractMemorySystem],
        mem_kwargs: dict,
    ):
        self.host_id = host_id
        self.num_cores = num_cores
        self.num_hns = num_hns
        self.mem_range = mem_range
        self.memory_cls = memory_cls
        self.mem_kwargs = mem_kwargs

        # Instantiate the memory subsystem for this host
        self.memory = self.memory_cls(**self.mem_kwargs)
        # Force the memory to use our specific address range
        self.memory.set_memory_range([self.mem_range])

    @property
    def is_cxl_pool(self):
        return self.num_cores == 0


class HostsWrapper:
    """
    Container for all hosts in the simulation.
    """

    def __init__(self, hosts: List[HostConfig]):
        self.hosts = hosts
        self._composite_memory = MultiHostMemory(self)

    def get_composite_memory(self) -> "MultiHostMemory":
        return self._composite_memory

    @property
    def compute_hosts(self) -> List[HostConfig]:
        return [h for h in self.hosts if not h.is_cxl_pool]

    @property
    def memory_pools(self) -> List[HostConfig]:
        return [h for h in self.hosts if h.is_cxl_pool]

    @property
    def num_hosts(self) -> int:
        return len(self.hosts)

    @property
    def num_compute_hosts(self) -> int:
        return len(self.compute_hosts)


class MultiHostMemory(AbstractMemorySystem):
    """
    A composite memory system that presents the aggregate of all host memory controllers
    to the board, while allowing heterogeneous memory definitions internally.
    """

    def __init__(self, hosts_wrapper: HostsWrapper):
        super().__init__()
        self._hosts_wrapper = hosts_wrapper

        # We must add the child memory objects to the SubSystem so m5 instantiates them properly
        for i, host in enumerate(self._hosts_wrapper.hosts):
            setattr(self, f"host_mem_{host.host_id}", host.memory)

    def incorporate_memory(self, board: AbstractBoard) -> None:
        for host in self._hosts_wrapper.hosts:
            host.memory.incorporate_memory(board)

    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        ports = []
        for host in self._hosts_wrapper.hosts:
            ports.extend(host.memory.get_mem_ports())
        return ports

    def get_memory_controllers(self) -> List[MemCtrl]:
        ctrls = []
        for host in self._hosts_wrapper.hosts:
            ctrls.extend(host.memory.get_memory_controllers())
        return ctrls

    def get_mem_interfaces(self) -> List[MemInterface]:
        interfaces = []
        for host in self._hosts_wrapper.hosts:
            interfaces.extend(host.memory.get_mem_interfaces())
        return interfaces

    def get_size(self) -> int:
        return sum(
            host.memory.get_size() for host in self._hosts_wrapper.hosts
        )

    def set_memory_range(self, ranges: List[AddrRange]) -> None:
        # Ignore calls from the board to set the global memory range.
        # We already explicitly initialized ranges in HostConfig.
        pass

    def get_uninterleaved_range(self) -> List[AddrRange]:
        ranges = []
        for host in self._hosts_wrapper.hosts:
            ranges.extend(host.memory.get_uninterleaved_range())
        return ranges

    def _pre_instantiate(self, root) -> None:
        for host in self._hosts_wrapper.hosts:
            host.memory._pre_instantiate(root)

    def _post_instantiate(self) -> None:
        for host in self._hosts_wrapper.hosts:
            host.memory._post_instantiate()
