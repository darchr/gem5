from m5.objects import (
    Cache,
    CoherentXBar,
    L2XBar,
    NoncoherentXBar,
)

from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.cachehierarchies.classic.caches.l1dcache import L1DCache
from gem5.components.cachehierarchies.classic.caches.l1icache import L1ICache
from gem5.components.cachehierarchies.classic.caches.l2cache import L2Cache
from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import (
    PrivateL1PrivateL2CacheHierarchy,
)
from gem5.components.processors.base_cpu_core import BaseCPU
from gem5.isas import ISA
from gem5.utils.override import overrides


class BypassedPrivateL1PrivateL2CacheHierarchy(
    PrivateL1PrivateL2CacheHierarchy
):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.pmem_xbar = NoncoherentXBar(
            frontend_latency=1,
            forward_latency=0,
            response_latency=1,
            width=64,
        )

    @overrides(PrivateL1PrivateL2CacheHierarchy)
    def incorporate_cache(self, board: AbstractBoard) -> None:
        board.connect_system_port(self.membus.cpu_side_ports)

        for _, port in board.get_mem_ports():
            self.membus.mem_side_ports = port

        self.l2buses = [
            L2XBar() for i in range(board.get_processor().get_num_cores())
        ]

        # self.membus.mem_side_ports = self.pmem_xbar.cpu_side_ports
        # PMEM is explicitly disconnected from membus to avoid address range overlaps

        for i, cpu in enumerate(board.get_processor().get_cores()):
            l2_node = self.add_root_child(
                f"l2-cache-{i}", L2Cache(size=self._l2_size)
            )
            l1i_node = l2_node.add_child(
                f"l1i-cache-{i}", L1ICache(size=self._l1i_size)
            )
            l1d_node = l2_node.add_child(
                f"l1d-cache-{i}", L1DCache(size=self._l1d_size)
            )

            self.l2buses[i].mem_side_ports = l2_node.cache.cpu_side
            self.membus.cpu_side_ports = l2_node.cache.mem_side

            l1i_node.cache.mem_side = self.l2buses[i].cpu_side_ports
            l1d_node.cache.mem_side = self.l2buses[i].cpu_side_ports

            cpu.connect_icache(l1i_node.cache.cpu_side)

            bypass_xbar = CoherentXBar(
                frontend_latency=1,
                forward_latency=0,
                response_latency=1,
                snoop_response_latency=1,
                width=64,
            )
            setattr(self, f"bypass_xbar_{i}", bypass_xbar)

            cpu.connect_dcache(bypass_xbar.cpu_side_ports)

            bypass_xbar.default = l1d_node.cache.cpu_side
            bypass_xbar.mem_side_ports = self.pmem_xbar.cpu_side_ports

            self._connect_table_walker(i, cpu)

            if board.get_processor().get_isa() == ISA.X86:
                int_req_port = self.membus.mem_side_ports
                int_resp_port = self.membus.cpu_side_ports
                cpu.connect_interrupt(int_req_port, int_resp_port)
            else:
                cpu.connect_interrupt()

        if board.has_coherent_io():
            self._setup_io_cache(board)
