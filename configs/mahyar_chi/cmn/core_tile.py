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


from typing import (
    List,
    Tuple,
)

from m5.objects import (
    NULL,
    ClockDomain,
    RubyCache,
    RubyController,
    RubyNetwork,
    RubySequencer,
    StridePrefetcher,
    SubSystem,
    TaggedPrefetcher,
)

from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.cachehierarchies.chi.nodes.abstract_node import (
    AbstractNode,
)
from gem5.components.processors.abstract_core import AbstractCore
from gem5.isas import ISA

from .network import (
    CMNSwitch,
    ExtLink,
    IntLink,
)


class CoreTile(SubSystem):
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
        add_dseq_to_core: bool,
    ):
        super(SubSystem, self).__init__()

        self._l1i_size = l1i_size
        self._l1d_size = l1d_size
        self._l2_size = l2_size

        self._ruby_system = ruby_system

        self._network = network

        # NOTE: To return when shared caches need to set their upstream_sequencers.
        self._sequencers = []

        # Create one core cluster with a split I/D and L2 cache for each core
        self.core_clusters = [
            self._create_core_clusters(core, board, add_dseq_to_core)
            for core in cores
        ]

    def get_sequencers(self) -> List[RubySequencer]:
        return self._sequencers

    def setup_network(self, network, system_router) -> Tuple[List, List, List]:
        """Returns a list of routers, ext links, and int links"""

        routers = []
        ext_links = []
        int_links = []

        for cluster in self.core_clusters:
            cluster.router = CMNSwitch(network)
            cluster.router.ext_routing_latency = 2
            cluster.router.int_routing_latency = 5
            cluster.l2_link = ExtLink(
                cluster.l2cache, cluster.router, bandwidth_factor=64
            )
            cluster.icache_link = ExtLink(
                cluster.icache, cluster.router, bandwidth_factor=64
            )
            cluster.dcache_link = ExtLink(
                cluster.dcache, cluster.router, bandwidth_factor=64
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
        self,
        core: AbstractCore,
        board: AbstractBoard,
        add_dseq_to_core: bool,
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
        cluster.icache.profile_usefulness = False

        cluster.icache.sequencer = RubySequencer(
            version=core_num,
            dcache=NULL,
            icache=cluster.icache.cache,
            clk_domain=cluster.icache.clk_domain,
            ruby_system=self._ruby_system,
        )
        cluster.dcache.sequencer = RubySequencer(
            version=core_num,
            dcache=cluster.dcache.cache,
            icache=NULL,
            deadlock_threshold=1_000_000,
            clk_domain=cluster.dcache.clk_domain,
            ruby_system=self._ruby_system,
        )

        cluster.dcache.upstream_sequencers = [cluster.dcache.sequencer]
        if add_dseq_to_core:
            core.set_data_sequencer(cluster.dcache.sequencer)

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
            size=self._l2_size,
            assoc=8,
            network=self._network,
            cache_line_size=board.get_cache_line_size(),
            clk_domain=board.get_clock_domain(),
        )
        cluster.l2cache.upstream_sequencers = [cluster.dcache.sequencer]

        cluster.l2cache.ruby_system = self._ruby_system

        cluster.dcache.downstream_destinations = [cluster.l2cache]
        cluster.icache.downstream_destinations = [cluster.l2cache]

        self._sequencers.append(cluster.dcache.sequencer)
        return cluster


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
        self.prefetcher = StridePrefetcher(
            degree=16, latency=1, block_size=cache_line_size
        )
        self.use_prefetcher = True

        # Only applies to home nodes
        self.is_HN = False
        self.enable_DMT = False
        self.enable_DCT = False

        self.allow_SD = True
        # This is kind of like enabling/disabling F state?...
        self.fwd_unique_on_readshared = False

        self.alloc_on_seq_acc = True
        # If we're writing the whole cache line, why allocate?
        # Most prevalent use case is for initializing data?...
        # Vector Instructions?
        self.alloc_on_seq_line_write = True

        self.alloc_on_readshared = True
        self.alloc_on_readunique = True
        self.alloc_on_readonce = True
        self.alloc_on_writeback = False  # Should never happen in an L1
        self.alloc_on_atomic = False

        ###########################
        # Don't apply to L1
        self.dealloc_on_unique = False
        self.dealloc_on_shared = False
        # NOTE: The CHI Config in gem5 sets these to True, I don't know why
        # it matters though. I don't think they matter though since L1 does not
        # have an upstream cache, maybe sequencer? Ideally, if it doesn't
        # matter, I would set it to False. I guess it does matter?...
        self.dealloc_backinv_unique = True
        self.dealloc_backinv_shared = True
        ###########################

        # Some reasonable default TBE params
        self.number_of_TBEs = 20
        self.number_of_repl_TBEs = 20
        self.number_of_snoop_TBEs = 4
        self.number_of_DVM_TBEs = 16
        self.number_of_DVM_snoop_TBEs = 4
        self.unify_repl_TBEs = False


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
        self.prefetcher = TaggedPrefetcher(
            degree=16, latency=1, queue_size=16, block_size=cache_line_size
        )
        self.use_prefetcher = True

        # Only used for L1 controllers
        self.send_evictions = False
        self.sequencer = NULL

        # Only applies to home nodes
        self.is_HN = False
        self.enable_DMT = False
        self.enable_DCT = False

        # Allow owned state
        self.allow_SD = True

        # This is kind of like enabling F state?...
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
        self.alloc_on_atomic = False

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
