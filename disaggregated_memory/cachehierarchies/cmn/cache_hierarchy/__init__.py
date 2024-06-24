# Copyright (c) 2022 The Regents of the University of California
# All Rights Reserved.
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

from math import log
from typing import List, Union

from m5.objects import TaggedPrefetcher
from m5.util.convert import toMemorySize

from m5.objects import (
    NULL,
    RubySystem,
    RubySequencer,
    RubyPortProxy,
    AddrRange,
    RubyController,
    RubyNetwork,
    ClockDomain,
    Addr,
    RubyCache,
)

from gem5.components.cachehierarchies.ruby.abstract_ruby_cache_hierarchy import (
    AbstractRubyCacheHierarchy,
)
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)

from gem5.utils.requires import requires
from gem5.utils.override import overrides
from gem5.coherence_protocol import CoherenceProtocol
from gem5.components.boards.abstract_board import AbstractBoard

from gem5.components.cachehierarchies.chi.nodes.dma_requestor import (
    DMARequestor,
)
from gem5.components.cachehierarchies.chi.nodes.memory_controller import (
    MemoryController,
)
from gem5.components.cachehierarchies.chi.nodes.abstract_node import (
    AbstractNode,
)

from ..modifiable import Modifiable

from .core_tile import CoreTile
from .network import MeshSystemNetwork


class CoherentMeshNetwork(AbstractRubyCacheHierarchy, Modifiable):
    _modifiables = [
        "Use L1 Prefetcher",
        "L1 Prefetcher",
        "Use L1 Prefetcher",
        "L2 Prefetcher",
    ]

    def __init__(
        self,
        l1i_size: str = "64KiB",
        l1d_size: str = "64KiB",
        l2_size: str = "1MiB",
        slc_size: str = "2MiB",
        slc_size_is_total: bool = False,
        cores_per_tile: int = 2,
        slc_intlv_size: str = "128B",
    ) -> None:
        """ """
        super(AbstractRubyCacheHierarchy, self).__init__()
        super(Modifiable, self).__init__()

        self._l1i_size = l1i_size
        self._l1d_size = l1d_size
        self._l2_size = l2_size
        self._slc_size_is_total = slc_size_is_total
        self._slc_size = toMemorySize(slc_size)
        self._cores_per_tile = cores_per_tile
        self._slc_intlv_size = slc_intlv_size

    @overrides(AbstractCacheHierarchy)
    def incorporate_cache(self, board: AbstractBoard) -> None:
        requires(coherence_protocol_required=CoherenceProtocol.CHI)

        self.ruby_system = RubySystem(
            block_size_bytes=board.get_cache_line_size()
        )

        # Network configurations
        # virtual networks: 0=request, 1=snoop, 2=response, 3=data
        self.ruby_system.number_of_virtual_networks = 4

        # Ruby's global network.
        self.ruby_system.network = MeshSystemNetwork(
            self.ruby_system, self.ruby_system.number_of_virtual_networks
        )

        # Create the coherent side of the memory controllers
        memory_controllers = []
        # mem_range_ports = []
        # mem_range_ports.append(board.get_bootmem_ports())
        # mem_range_ports.append(board.get_local_memory_ports())
        # mem_range_ports.append(board.get_remote_memory_ports())
        for rng_port in board.get_mem_ports():
            for rng, port in rng_port:
                memory_controller = MemoryController(
                    self.ruby_system.network, [rng], port
                )
                memory_controller.ruby_system = self.ruby_system
                memory_controllers.append(memory_controller)
        self.memory_controllers = memory_controllers

        cores = board.get_processor().get_cores()
        assert (len(cores) % self._cores_per_tile) == 0
        self._num_tiles = len(cores) // self._cores_per_tile
        assert self._num_tiles & (self._num_tiles - 1) == 0

        if self._slc_size_is_total:
            self._slc_size = int(self._slc_size / self._num_tiles)

        self._slc_size = f"{self._slc_size}B"
        mem_range = board.get_monolithic_range()
        tile_ranges = self._intlv_memory_for_tiles(
            mem_range.start,
            mem_range.size(),
            int(len(cores) / self._cores_per_tile),
            self._slc_intlv_size,
        )

        core_tiles = []
        system_caches = []
        for i in range(int(len(cores) / self._cores_per_tile)):
            cores_in_this_tile = cores[
                i * self._cores_per_tile : (i + 1) * self._cores_per_tile
            ]
            core_tile = CoreTile(
                cores_in_this_tile,
                board,
                self.ruby_system.network,
                self.ruby_system,
                self._l1i_size,
                self._l1d_size,
                self._l2_size,
            )
            core_tiles.append(core_tile)
            system_cache_slice = SystemLevelCache(
                size=self._slc_size,
                assoc=16,
                network=self.ruby_system.network,
                cache_line_size=board.get_cache_line_size(),
                clk_domain=board.get_clock_domain(),
            )
            system_cache_slice.addr_ranges = tile_ranges[i]
            system_cache_slice.downstream_destinations = (
                self.memory_controllers
            )
            system_cache_slice.ruby_system = self.ruby_system
            system_caches.append(system_cache_slice)
        self.core_tiles = core_tiles
        self.system_caches = system_caches

        for core_tile in core_tiles:
            core_tile.set_downstream_destinations(self.system_caches)

        # Create the DMA Controllers, if required.
        if board.has_dma_ports():
            self.dma_controllers = self._create_dma_controllers(
                board, self.system_caches
            )
            self.ruby_system.num_of_sequencers = len(cores) * 2 + len(
                self.dma_controllers
            )
        else:
            self.ruby_system.num_of_sequencers = len(cores) * 2

        self.ruby_system.network.connect_core_tiles(
            self.core_tiles, self.system_caches
        )
        self.ruby_system.network.connect_memory_controllers(
            self.memory_controllers
        )
        self.ruby_system.network.connect_dma_controllers(
            self.dma_controllers if board.has_dma_ports() else []
        )

        self.ruby_system.network.build_system_network()

        self.ruby_system.network.finalize()

        self.ruby_system.network.setup_buffers()

        # Set up a proxy port for the system_port. Used for load binaries and
        # other functional-only things.
        self.ruby_system.sys_port_proxy = RubyPortProxy()
        board.connect_system_port(self.ruby_system.sys_port_proxy.in_ports)

    def _create_dma_controllers(
        self,
        board: AbstractBoard,
        downstream_destinations: List[RubyController],
    ) -> List[DMARequestor]:
        dma_controllers = []
        for i, port in enumerate(board.get_dma_ports()):
            ctrl = DMARequestor(
                self.ruby_system.network,
                board.get_cache_line_size(),
                board.get_clock_domain(),
            )
            ctrl.cache.size = "1KiB"
            version = len(board.get_processor().get_cores()) + i
            ctrl.sequencer = RubySequencer(version=version, in_ports=port)
            ctrl.sequencer.dcache = NULL

            ctrl.ruby_system = self.ruby_system
            ctrl.sequencer.ruby_system = self.ruby_system

            ctrl.downstream_destinations = downstream_destinations

            dma_controllers.append(ctrl)

        return dma_controllers

    def _intlv_memory_for_tiles(
        self,
        start_addr: Addr,
        mem_size: int,
        num_tiles: int,
        intlv_size: Union[int, str],
    ) -> List[AddrRange]:
        intlv_size = toMemorySize(intlv_size)
        intlv_low_bit = int(log(intlv_size, 2))
        intlv_bits = int(log(num_tiles, 2))
        return [
            AddrRange(
                start=start_addr,
                size=mem_size,
                intlvHighBit=intlv_low_bit + intlv_bits - 1,
                xorHighBit=0,
                intlvBits=intlv_bits,
                intlvMatch=i,
            )
            for i in range(num_tiles)
        ]


class SystemLevelCache(AbstractNode):
    """This cache assumes that it is a shared victim L3.

    This cache also tracks all sharers in caches closer to the CPU.
    """

    def __init__(
        self,
        size: str,
        assoc: int,
        network: RubyNetwork,
        cache_line_size,
        clk_domain: ClockDomain,
    ):
        super().__init__(network, cache_line_size)

        self.cache = RubyCache(
            size=size, assoc=assoc, start_index_bit=self.getBlockSizeBits()
        )

        self.clk_domain = clk_domain
        self.prefetcher = NULL
        self.use_prefetcher = False

        # Only used for L1 controllers
        self.send_evictions = False
        self.sequencer = NULL

        self.is_HN = True
        self.enable_DMT = True
        self.enable_DCT = True

        self.allow_SD = True

        self.alloc_on_seq_acc = False  # Does not apply to L3
        self.alloc_on_seq_line_write = False

        self.alloc_on_readshared = (
            False  # I think this should be True for perf
        )
        self.alloc_on_readunique = False
        self.alloc_on_readonce = False

        # insert on writeback (victim cache)
        self.alloc_on_writeback = True

        # Only cache atomics in SLC
        # self.alloc_on_atomic = True
        self.alloc_on_atomic = False
        # Keep the line if a requestor asks for unique/shared
        ###########################
        self.dealloc_on_unique = False
        self.dealloc_on_shared = False
        ###########################

        # Allow caches closer to core to keep block even if evicted from L3
        self.dealloc_backinv_unique = False
        self.dealloc_backinv_shared = False

        # Some reasonable default TBE params
        self.number_of_TBEs = 64
        self.number_of_repl_TBEs = 64
        self.number_of_snoop_TBEs = 8
        self.number_of_DVM_TBEs = 16
        self.number_of_DVM_snoop_TBEs = 4
        self.unify_repl_TBEs = False

        # Data gathering flags
        # self.is_closest = False
