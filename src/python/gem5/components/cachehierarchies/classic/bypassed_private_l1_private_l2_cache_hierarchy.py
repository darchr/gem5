from m5.objects import (
    AddrRange,
    Bridge,
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
    def __init__(self, *args, pmem_address_range=None, **kwargs):
        super().__init__(*args, **kwargs)

        # When pmem_address_range is given we replace the "default-responder"
        # bypass hack with non-coherent Bridges hanging off each per-core xbar
        # (the xbar between the core and its L1). incorporate_cache() adds, per
        # core, an l1_bridge (xbar -> L1, NON-device range) and a
        # device_in_bridge (xbar -> membus, device range). Here we add the one
        # shared device_bridge (membus -> device). Net effect: device-range
        # accesses skip the L1/L2 and reach the device as a plain memory on the
        # normal memory bus -- like a baseline memory controller -- so its
        # mapping survives mid-workload checkpoint/restore. The Bridges keep the
        # device out of the coherence domain (no snoops), and host-DRAM
        # coherence on the membus is untouched (only the device range is routed
        # through the device path).
        #
        # When it is None we keep the original behavior: the cache is the xbar
        # default responder and device-range traffic exits via a dedicated
        # non-coherent xbar (pmem_xbar) that other scripts still wire to.
        self._pmem_address_range = pmem_address_range
        if pmem_address_range is None:
            self.pmem_xbar = NoncoherentXBar(
                frontend_latency=1,
                forward_latency=0,
                response_latency=1,
                width=64,
            )
        else:
            # membus <-> device (non-coherent; device-facing side wired to
            # board.pmem.port in the run script).
            self.device_bridge = Bridge(
                ranges=[pmem_address_range], delay="0ns"
            )
            self.membus.mem_side_ports = self.device_bridge.cpu_side_port

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

            if self._pmem_address_range is None:
                # Original behavior: the cache is the xbar's default responder
                # and device-range traffic goes out the side pmem_xbar.
                bypass_xbar.default = l1d_node.cache.cpu_side
                bypass_xbar.mem_side_ports = self.pmem_xbar.cpu_side_ports
            else:
                # No "default" bypass hack. Two non-coherent Bridges hang off
                # this per-core xbar and TILE the address space by range, so the
                # xbar routes purely by address:
                #   - device range   -> device_in_bridge -> membus -> device
                #     (uncached: skips this core's L1/L2)
                #   - everything else -> l1_bridge -> L1 (normal cached path)
                #
                # The L1 sits BEHIND l1_bridge instead of being wired straight
                # to the xbar. A directly-connected cache claims *all* addresses
                # it can reach downstream -- and since the device is on the
                # membus, that includes the device range -- which would overlap
                # device_in_bridge (xbar range overlap = error) and would cache
                # the device. l1_bridge advertises only the NON-device range, so
                # the cache no longer claims the device range here and no
                # `default` catch-all is needed. Snoops are unaffected: they
                # reach the L1 from its mem_side (L2/membus), not this cpu_side.
                dev = self._pmem_address_range
                dev_start = int(dev.start)
                dev_end = dev_start + int(dev.size())

                l1_bridge = Bridge(
                    ranges=[
                        AddrRange(start=0, end=dev_start),
                        AddrRange(start=dev_end, end=(1 << 48)),
                    ],
                    delay="0ns",
                )
                setattr(self, f"l1_bridge_{i}", l1_bridge)
                bypass_xbar.mem_side_ports = l1_bridge.cpu_side_port
                l1_bridge.mem_side_port = l1d_node.cache.cpu_side

                device_in_bridge = Bridge(ranges=[dev], delay="0ns")
                setattr(self, f"device_in_bridge_{i}", device_in_bridge)
                bypass_xbar.mem_side_ports = device_in_bridge.cpu_side_port
                device_in_bridge.mem_side_port = self.membus.cpu_side_ports

            self._connect_table_walker(i, cpu)

            if board.get_processor().get_isa() == ISA.X86:
                int_req_port = self.membus.mem_side_ports
                int_resp_port = self.membus.cpu_side_ports
                cpu.connect_interrupt(int_req_port, int_resp_port)
            else:
                cpu.connect_interrupt()

        if board.has_coherent_io():
            self._setup_io_cache(board)
