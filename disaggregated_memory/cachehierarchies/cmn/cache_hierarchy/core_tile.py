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


from typing import List, Tuple

from m5.objects import ClockDomain, SubSystem, NULL
from m5.objects import BOPPrefetcher, StridePrefetcher, TaggedPrefetcher
from m5.objects import RubyCache, RubyNetwork, RubySequencer, RubyController

from gem5.isas import ISA
from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.processors.abstract_core import AbstractCore
from gem5.components.cachehierarchies.chi.nodes.abstract_node import (
    AbstractNode,
)

from .network import CMNSwitch, IntLink, ExtLink

from ..modifiable import Modifiable


class CoreTile(SubSystem, Modifiable):
    _modifiables = [
        "Use L1 Prefetcher",
        "L1 Prefetcher",
        "Use L1 Prefetcher",
        "L2 Prefetcher",
    ]
    _core_number = 0

    @classmethod
    def _get_core_number(cls):
        cls._core_number += 1  # Use count for this particular type
        return cls._core_number - 1

    def __init__(
        self,
        cores: List[AbstractCore],
        board: AbstractBoard,
        network,
        ruby_system,
        l1i_size,
        l1d_size,
        l2_size,
    ):
        super(SubSystem, self).__init__()
        super(Modifiable, self).__init__()

        self._l1i_size = l1i_size
        self._l1d_size = l1d_size
        self._l2_size = l2_size

        self._ruby_system = ruby_system

        self._network = network

        # Create one core cluster with a split I/D and L2 cache for each core
        self.core_clusters = [
            self._create_core_clusters(core, board) for core in cores
        ]

    def setup_network(self, network, system_router) -> Tuple[List, List, List]:
        """Returns a list of routers, ext links, and int links"""

        routers = []
        ext_links = []
        int_links = []

        for cluster in self.core_clusters:
            cluster.router = CMNSwitch(network)
            cluster.router.ext_routing_latency = 2
            cluster.router.int_routing_latency = 5
            cluster.icache_link = ExtLink(
                cluster.icache, cluster.router, bandwidth_factor=64
            )
            cluster.dcache_link = ExtLink(
                cluster.dcache, cluster.router, bandwidth_factor=64
            )
            cluster.l2_link = ExtLink(
                cluster.l2cache, cluster.router, bandwidth_factor=64
            )
            routers.append(cluster.router)

            cluster.l2_system_link = IntLink(
                cluster.router, system_router, bandwidth_factor=64
            )
            cluster.system_l2_link = IntLink(
                system_router, cluster.router, bandwidth_factor=64
            )
            ext_links.extend(
                [
                    cluster.icache_link,
                    cluster.dcache_link,
                    cluster.l2_link,
                ]
            )
            int_links.extend(
                [
                    cluster.l2_system_link,
                    cluster.system_l2_link,
                ]
            )

        return (routers, ext_links, int_links)

    def set_downstream_destinations(
        self, destinations: List[RubyController]
    ) -> None:
        for cluster in self.core_clusters:
            cluster.l2cache.downstream_destinations = destinations

    def _create_core_clusters(
        self, core: AbstractCore, board: AbstractBoard
    ) -> SubSystem:
        """Given the core and the core number this function creates a cluster
        for the core with a split I/D cache and L2 cache
        """
        core_num = self._get_core_number()

        cluster = SubSystem()
        cluster.dcache = PrivateL1Cache(
            size=self._l1d_size,
            assoc=4,
            network=self._network,
            core=core,
            cache_line_size=board.get_cache_line_size(),
            clk_domain=board.get_clock_domain(),
        )
        # cluster.dcache.sc_lock_enable = True
        cluster.icache = PrivateL1Cache(
            size=self._l1i_size,
            assoc=4,
            network=self._network,
            core=core,
            cache_line_size=board.get_cache_line_size(),
            clk_domain=board.get_clock_domain(),
        )

        cluster.icache.sequencer = RubySequencer(
            version=core_num, dcache=NULL, clk_domain=cluster.icache.clk_domain
        )
        cluster.dcache.sequencer = RubySequencer(
            version=core_num,
            dcache=cluster.dcache.cache,
            clk_domain=cluster.dcache.clk_domain,
        )

        if board.has_io_bus():
            cluster.dcache.sequencer.connectIOPorts(board.get_io_bus())

        cluster.dcache.ruby_system = self._ruby_system
        cluster.icache.ruby_system = self._ruby_system

        core.connect_icache(cluster.icache.sequencer.in_ports)
        core.connect_dcache(cluster.dcache.sequencer.in_ports)

        core.connect_walker_ports(
            cluster.dcache.sequencer.in_ports,
            cluster.icache.sequencer.in_ports,
        )

        # Connect the interrupt ports
        if board.get_processor().get_isa() == ISA.X86:
            int_req_port = cluster.dcache.sequencer.interrupt_out_port
            int_resp_port = cluster.dcache.sequencer.in_ports
            core.connect_interrupt(int_req_port, int_resp_port)
        else:
            core.connect_interrupt()

        # Create the L2 cache
        cluster.l2cache = PrivateL2Cache(
            size="512KiB",
            assoc=8,
            network=self._network,
            cache_line_size=board.get_cache_line_size(),
            clk_domain=board.get_clock_domain(),
        )

        cluster.l2cache.ruby_system = self._ruby_system

        cluster.dcache.downstream_destinations = [cluster.l2cache]
        cluster.icache.downstream_destinations = [cluster.l2cache]

        return cluster

    def set_l2_size(self, l2_size: str):
        for cluster in self.core_clusters:
            cluster.l2cache.cache.size = l2_size


class PrivateL1Cache(AbstractNode):
    def __init__(
        self,
        size: str,
        assoc: int,
        network: RubyNetwork,
        core: AbstractCore,
        cache_line_size,
        clk_domain: ClockDomain,
    ):
        super().__init__(network, cache_line_size)

        self.cache = RubyCache(
            size=size, assoc=assoc, start_index_bit=self.getBlockSizeBits()
        )

        self.clk_domain = clk_domain
        self.send_evictions = core.requires_send_evicts()
        # self.prefetcher = BOPPrefetcher()
        self.prefetcher = StridePrefetcher(degree=16, latency = 1)
        self.use_prefetcher = True
        # self.use_prefetcher = False

        # Only applies to home nodes
        self.is_HN = False
        self.enable_DMT = False
        self.enable_DCT = False

        self.allow_SD = True
        # Prevent forwarding since I have no idea whether it should be T/F
        self.fwd_unique_on_readshared = False

        self.alloc_on_seq_acc = True
        # Guess: Probably useful for DMA
        self.alloc_on_seq_line_write = False

        self.alloc_on_readshared = True
        self.alloc_on_readunique = True
        self.alloc_on_readonce = True
        self.alloc_on_writeback = False  # Should never happen in an L1
        self.alloc_on_atomic = True
        # self.alloc_on_atomic = False

        ###########################
        # Don't apply to L1
        self.dealloc_on_unique = False
        self.dealloc_on_shared = False
        self.dealloc_backinv_unique = False
        self.dealloc_backinv_shared = False
        ###########################

        # Some reasonable default TBE params
        self.number_of_TBEs = 20
        self.number_of_repl_TBEs = 20
        self.number_of_snoop_TBEs = 4
        self.number_of_DVM_TBEs = 16
        self.number_of_DVM_snoop_TBEs = 4
        self.unify_repl_TBEs = False

        # Data gathering flags
        # self.is_closest = True


class PrivateL2Cache(AbstractNode):
    """This cache assumes the CPU-side L1 cache is inclusive (no clean WBs)
    and that the L3 is tracking all tags in the L1/L2.

    This cache also assumes the L3 is a victim cache, so it needs to writeback
    clean and dirty data.
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
        # self.prefetcher = BOPPrefetcher(
        #     round_max=50, rr_size=128, offset_list_size=92
        # )
        self.prefetcher = TaggedPrefetcher(degree=16, latency = 1, queue_size = 16)
        self.use_prefetcher = True
        # self.use_prefetcher = False

        # Only used for L1 controllers
        self.send_evictions = False
        self.sequencer = NULL

        # Only applies to home nodes
        self.is_HN = False
        self.enable_DMT = False
        self.enable_DCT = False

        # Allow owned state
        self.allow_SD = True

        # Prevent forwarding since I have no idea whether it should be T/F
        self.fwd_unique_on_readshared = False

        ###########################
        # Don't apply to L2
        self.alloc_on_seq_acc = False
        self.alloc_on_seq_line_write = False
        ###########################

        ###########################
        # Keeping L2 inclusive of L1
        self.alloc_on_readshared = True
        self.alloc_on_readunique = True
        self.alloc_on_readonce = True
        self.alloc_on_atomic = True
        # self.alloc_on_atomic = False

        self.dealloc_on_unique = False
        self.dealloc_on_shared = False
        self.dealloc_backinv_unique = True
        self.dealloc_backinv_shared = True
        self.alloc_on_writeback = False  # Shouldn't matter since inclusive
        ###########################

        # Some reasonable default TBE params
        self.number_of_TBEs = 46
        self.number_of_repl_TBEs = 46
        self.number_of_snoop_TBEs = 4
        self.number_of_DVM_TBEs = 16
        self.number_of_DVM_snoop_TBEs = 4
        self.unify_repl_TBEs = False

        # Data gathering flags
        # self.is_closest = False
