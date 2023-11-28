# Copyright (c) 2023 The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.components.cachehierarchies.classic.abstract_classic_cache_hierarchy import (
    AbstractClassicCacheHierarchy,
)
from gem5.components.cachehierarchies.abstract_two_level_cache_hierarchy import (
    AbstractTwoLevelCacheHierarchy,
)

# from gem5.components.cachehierarchies.classic.caches.l1dcache import L1DCache
# from gem5.components.cachehierarchies.classic.caches.l1icache import L1ICache
# from gem5.components.cachehierarchies.classic.caches.l2cache import L2Cache
from gem5.components.cachehierarchies.classic.caches.mmu_cache import MMUCache
from gem5.components.boards.abstract_board import AbstractBoard
from gem5.isas import ISA
from m5.objects import (
    Cache,
    L2XBar,
    BaseXBar,
    SystemXBar,
    BadAddr,
    Port,
    Clusivity,
    BasePrefetcher,
    StridePrefetcher,
)

from gem5.utils.override import *
from typing import Type

# from m5.objects import Clusivity, BasePrefetcher, StridePrefetcher

from typing import Type


class NormalCache(Cache):
    """
    A simple Cache with default values.
    """

    def __init__(
        self,
        size: str,
        assoc: int = 16,
        tag_latency: int = 10,
        data_latency: int = 10,
        response_latency: int = 1,
        mshrs: int = 20,
        tgts_per_mshr: int = 12,
        writeback_clean: bool = False,
        clusivity: Clusivity = "mostly_incl",
        PrefetcherCls: Type[BasePrefetcher] = StridePrefetcher,
        **kwargs
    ):
        super().__init__()
        self.size = size
        self.assoc = assoc
        self.tag_latency = tag_latency
        self.data_latency = data_latency
        self.response_latency = response_latency
        self.mshrs = mshrs
        self.tgts_per_mshr = tgts_per_mshr
        self.writeback_clean = writeback_clean
        self.clusivity = clusivity
        self.prefetcher = PrefetcherCls()


class RocketChipCacheHierarchy(
    AbstractClassicCacheHierarchy, AbstractTwoLevelCacheHierarchy
):
    """

    A cache setup where each core has a private L1 Data and Instruction Cache,
    and a shared L2 cache.
    The HiFive board has a partially inclusive cache hierarchy, hence this hierarchy is chosen.
    The details of the cache hierarchy are in Table 7, page 36 of the datasheet.

    - L1 Instruction Cache:
        - 32 KiB 4-way set associative
    - L1 Data Cache
        - 32 KiB 8-way set associative
    - L2 Cache
        - 2 MiB 16-way set associative

    """

    def __init__(
        self,
        l2_size: str,
    ) -> None:
        """
        :param l2_size: The size of the L2 Cache (e.g., "256kB").
        :type l2_size: str
        """
        AbstractClassicCacheHierarchy.__init__(self=self)
        AbstractTwoLevelCacheHierarchy.__init__(
            self,
            l1i_size="32KiB",
            l1i_assoc=4,
            l1d_size="16KiB",
            l1d_assoc=4,
            l2_size=l2_size,
            l2_assoc=8,
        )

        self.membus = SystemXBar(width=64)
        self.membus.badaddr_responder = BadAddr()
        self.membus.default = self.membus.badaddr_responder.pio
        # need to initilize the caches here to have them show up
        # while enumerating params
        # they will be overwritten in incorporate_cache
        self.l2cache = NormalCache(
            size=self._l2_size,
            assoc=self._l2_assoc,
            tag_latency=20,
            data_latency=20,
            response_latency=1,
            mshrs=4,
            warmup_percentage=0,
            demand_mshr_reserve=10,
            tgts_per_mshr=1,
            write_buffers=16,
            prefetch_on_access=False,
            prefetch_on_pf_hit=False,
            move_contractions=False,
            sequential_access=False,
            writeback_clean=True,
            clusivity="mostly_incl",
        )
        self.l1dcaches = [
            NormalCache(
                size=self._l1d_size,
                assoc=self._l1d_assoc,
                data_latency=1,
                tag_latency=1,
                response_latency=1,
                mshrs=2,
                warmup_percentage=0,
                demand_mshr_reserve=1,
                tgts_per_mshr=14,
                write_buffers=1,
                prefetch_on_access=False,
                prefetch_on_pf_hit=False,
                move_contractions=False,
                sequential_access=False,
                writeback_clean=False,
                clusivity="mostly_incl",
            )
            for i in range(1)
        ]
        self.l2bus = L2XBar()

        self.l1icaches = [
            NormalCache(
                size=self._l1i_size,
                assoc=self._l1i_assoc,
                tag_latency=1,
                data_latency=1,
                response_latency=1,
                mshrs=10,
                warmup_percentage=0,
                demand_mshr_reserve=3,
                tgts_per_mshr=11,
                write_buffers=16,
                prefetch_on_access=False,
                prefetch_on_pf_hit=False,
                move_contractions=False,
                sequential_access=False,
                writeback_clean=True,
                clusivity="mostly_excl",
            )
            for i in range(1)
        ]

        # ITLB Page walk caches
        self.iptw_caches = [MMUCache(size="2KiB") for _ in range(1)]
        # DTLB Page walk caches
        self.dptw_caches = [MMUCache(size="2KiB") for _ in range(1)]

    @overrides(AbstractClassicCacheHierarchy)
    def get_mem_side_port(self) -> Port:
        return self.membus.mem_side_ports

    @overrides(AbstractClassicCacheHierarchy)
    def get_cpu_side_port(self) -> Port:
        return self.membus.cpu_side_ports

    @overrides(AbstractCacheHierarchy)
    def incorporate_cache(self, board: AbstractBoard) -> None:
        # self.l1icaches = [
        #     L1ICache(size=self._l1i_size, assoc=self._l1i_assoc)
        #     for i in range(board.get_processor().get_num_cores())
        # ]
        # self.l1dcaches = [
        #     L1DCache(
        #         size=self._l1d_size, assoc=self._l1d_assoc
        #     )
        #     for i in range(board.get_processor().get_num_cores())
        # ]
        # self.l2bus = L2XBar()

        # # ITLB Page walk caches
        # self.iptw_caches = [
        #     MMUCache(size="2KiB")
        #     for _ in range(board.get_processor().get_num_cores())
        # ]
        # # DTLB Page walk caches
        # self.dptw_caches = [
        #     MMUCache(size="2KiB")
        #     for _ in range(board.get_processor().get_num_cores())
        # ]

        # Set up the system port for functional access from the simulator.
        board.connect_system_port(self.membus.cpu_side_ports)

        for cntr in board.get_memory().get_memory_controllers():
            cntr.port = self.membus.mem_side_ports

        if board.has_coherent_io():
            self._setup_io_cache(board)

        for i, cpu in enumerate(board.get_processor().get_cores()):
            cpu.connect_icache(self.l1icaches[i].cpu_side)
            cpu.connect_dcache(self.l1dcaches[i].cpu_side)

            self.l1icaches[i].mem_side = self.l2bus.cpu_side_ports
            self.l1dcaches[i].mem_side = self.l2bus.cpu_side_ports
            self.iptw_caches[i].mem_side = self.l2bus.cpu_side_ports
            self.dptw_caches[i].mem_side = self.l2bus.cpu_side_ports

            cpu.connect_walker_ports(
                self.iptw_caches[i].cpu_side, self.dptw_caches[i].cpu_side
            )

            if board.get_processor().get_isa() == ISA.X86:
                int_req_port = self.membus.mem_side_ports
                int_resp_port = self.membus.cpu_side_ports
                cpu.connect_interrupt(int_req_port, int_resp_port)
            else:
                cpu.connect_interrupt()

        self.l2bus.mem_side_ports = self.l2cache.cpu_side
        self.membus.cpu_side_ports = self.l2cache.mem_side

    def _setup_io_cache(self, board: AbstractBoard) -> None:
        """Create a cache for coherent I/O connections"""
        self.iocache = Cache(
            assoc=8,
            tag_latency=50,
            data_latency=50,
            response_latency=50,
            mshrs=20,
            size="1kB",
            tgts_per_mshr=12,
            addr_ranges=board.mem_ranges,
        )
        self.iocache.mem_side = self.membus.cpu_side_ports
        self.iocache.cpu_side = board.get_mem_side_coherent_io_port()
