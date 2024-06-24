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

from itertools import chain
from typing import List, Tuple

from m5.objects import ClockDomain, SubSystem, NULL
from m5.objects import RubyCache, RubyNetwork, RubySequencer

from gem5.isas import ISA
from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.processors.abstract_core import AbstractCore
from gem5.components.cachehierarchies.chi.nodes.abstract_node import (
    AbstractNode,
)

from .network import SagaSwitch, SagaIntLink, SagaExtLink


class CoreComplex(SubSystem):
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
        directories: List,
        ruby_system,
    ):
        """A single AMD Epyc-like core complex.

        Each core complex has up to 8 cores each with a private L1 I/D and
        private L2 as well as a shared victim L3.

        The L1 I/D are both 32 KiB, L2 is 512 KiB (per core), and L3 is 8 MiB
        per CCX. These are number from Zen3.
        """
        super().__init__()

        self._ruby_system = ruby_system

        self._network = network

        # Note: we could reduce this size to smaller when there are fewer cores
        self.l3cache = SharedL3Cache(
            size="8MiB",
            assoc=16,
            network=network,
            cache_line_size=board.get_cache_line_size(),
            clk_domain=board.get_clock_domain(),
        )
        self.l3cache.ruby_system = self._ruby_system

        # Create one core cluster with a split I/D and L2 cache for each core
        self.core_clusters = [
            self._create_core_cluster(core, board)
            for i, core in enumerate(cores)
        ]

        self.l3cache.downstream_destinations = directories

    def get_all_controllers(self):
        """Returns a flat list of all controllers.

        The order is such that you'll get a dcache, then an icache for each
        core, then the L2 caches for each core, and finally the L3 cache
        """
        return (
            list(
                chain.from_iterable(  # Grab the controllers from each cluster
                    [
                        (cluster.dcache, cluster.icache)
                        for cluster in self.core_clusters
                    ]
                )
            )
            + [cluster.l2cache for cluster in self.core_clusters]
            + [self.l3cache]
        )

    def setup_network(self, dir_router, network) -> Tuple[List, List, List]:
        """Returns a list of routers, ext links, and int links"""
        self.l3_router = SagaSwitch(network)

        routers = [self.l3_router]
        ext_links = []
        int_links = []

        for cluster in self.core_clusters:
            cluster.l2_router = SagaSwitch(network)
            routers.append(cluster.l2_router)

            cluster.icache_link = SagaExtLink(
                cluster.icache, cluster.l2_router
            )
            cluster.dcache_link = SagaExtLink(
                cluster.dcache, cluster.l2_router
            )
            cluster.l2_link = SagaExtLink(cluster.l2cache, cluster.l2_router)
            cluster.l2_l3_link = SagaIntLink(cluster.l2_router, self.l3_router)
            cluster.l3_l2_link = SagaIntLink(self.l3_router, cluster.l2_router)
            ext_links.extend(
                [
                    cluster.icache_link,
                    cluster.dcache_link,
                    cluster.l2_link,
                ]
            )
            int_links.extend([cluster.l2_l3_link, cluster.l3_l2_link])

        self.l3_link = SagaExtLink(
            self.l3cache, self.l3_router, bandwidth_factor=64
        )
        ext_links.append(self.l3_link)

        self.ccx_link_out = SagaIntLink(
            self.l3_router, dir_router, bandwidth_factor=64
        )
        self.ccx_link_in = SagaIntLink(
            dir_router, self.l3_router, bandwidth_factor=64
        )
        int_links.extend([self.ccx_link_out, self.ccx_link_in])

        return (routers, ext_links, int_links)

    def _create_core_cluster(
        self, core: AbstractCore, board: AbstractBoard
    ) -> SubSystem:
        """Given the core and the core number this function creates a cluster
        for the core with a split I/D cache and L2 cache
        """
        core_num = self._get_core_number()

        cluster = SubSystem()
        cluster.dcache = PrivateL1Cache(
            size="32KiB",
            assoc=8,
            network=self._network,
            core=core,
            cache_line_size=board.get_cache_line_size(),
            clk_domain=board.get_clock_domain(),
        )
        cluster.icache = PrivateL1Cache(
            size="32KiB",
            assoc=8,
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

        cluster.l2cache.downstream_destinations = [self.l3cache]

        return cluster


class PrivateL1Cache(AbstractNode):
    """This cache requires that the L2 (memory side cache) is inclusive
    of the L1.
    """

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
        self.use_prefetcher = False
        self.prefetcher = NULL # Maryam

        # Only applies to home nodes
        self.is_HN = False
        self.enable_DMT = False
        self.enable_DCT = False

        # MOESI states for a 1 level cache
        self.allow_SD = True

        ###########################
        # I don't understand
        self.alloc_on_seq_acc = True
        # What is sequencer line write RequestType::StoreLine
        self.alloc_on_seq_line_write = False
        ###########################

        self.alloc_on_readshared = True
        self.alloc_on_readunique = True
        self.alloc_on_readonce = True
        self.alloc_on_writeback = False  # Should never happen in an L1
        self.alloc_on_atomic = False # Maryam

        ###########################
        # I don't understand
        # I think this should does not apply to L1
        self.dealloc_on_unique = False
        self.dealloc_on_shared = False
        ###########################

        # Don't keep the line if someone else invalidates
        # I think this does not apply to L1
        self.dealloc_backinv_unique = True
        self.dealloc_backinv_shared = True

        # Some reasonable default TBE params
        self.number_of_TBEs = 16
        self.number_of_repl_TBEs = 16
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
        self.use_prefetcher = False  # >>> Should be true
        self.prefetcher = NULL # Maryam

        # Only used for L1 controllers
        self.send_evictions = False
        self.sequencer = NULL

        # Only applies to home nodes
        self.is_HN = False
        self.enable_DMT = False
        self.enable_DCT = False

        # Allow owned state
        self.allow_SD = True

        # Allow "forwarding" state
        ###########################
        # I don't understand this, is this equivalent to F in MOESIF?
        # self.fwd_unique_on_readshared = True
        ###########################

        # Allocate on miss
        ###########################
        # I don't think these matter to an L2 cache
        # Setting them to false
        self.alloc_on_seq_acc = False
        self.alloc_on_seq_line_write = False
        ###########################
        self.alloc_on_readshared = True
        self.alloc_on_readunique = True
        self.alloc_on_readonce = True

        # Maintain inclusion
        # Can we set this to False if it doesn't matter?
        self.alloc_on_writeback = True  # Shouldn't matter since inclusive
        self.alloc_on_atomic = False # Maryam

        self.dealloc_on_unique = False
        self.dealloc_on_shared = False

        # Maintain inclusion. Invalidate the L1 if this line is evicted
        self.dealloc_backinv_unique = True
        self.dealloc_backinv_shared = True

        # Some reasonable default TBE params
        self.number_of_TBEs = 16
        self.number_of_repl_TBEs = 16
        self.number_of_snoop_TBEs = 4
        self.number_of_DVM_TBEs = 16
        self.number_of_DVM_snoop_TBEs = 4
        self.unify_repl_TBEs = False


class SharedL3Cache(AbstractNode):
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
        self.use_prefetcher = False  # >>> Should be true
        self.prefetcher = NULL # Maryam

        # Only used for L1 controllers
        self.send_evictions = False
        self.sequencer = NULL

        # Only applies to home nodes
        self.is_HN = False
        self.enable_DMT = False
        self.enable_DCT = False

        # MOESI states
        self.allow_SD = True

        # victim cache, don't allocate on miss
        self.alloc_on_seq_acc = False
        self.alloc_on_seq_line_write = False

        self.alloc_on_readshared = False
        self.alloc_on_readunique = False
        self.alloc_on_readonce = False

        # insert on writeback (victim cache)
        self.alloc_on_writeback = True
        self.alloc_on_atomic = True # Maryam

        # Keep the line if a requestor asks for unique/shared
        ###########################
        # If L3 is a victim cache, does it really matter what we set for these?
        self.dealloc_on_unique = False
        self.dealloc_on_shared = False
        ###########################

        # Allow caches closer to core to keep block even if evicted from L3
        self.dealloc_backinv_unique = False
        self.dealloc_backinv_shared = False

        # Some reasonable default TBE params
        self.number_of_TBEs = 128
        self.number_of_repl_TBEs = 16
        self.number_of_snoop_TBEs = 4
        self.number_of_DVM_TBEs = 16
        self.number_of_DVM_snoop_TBEs = 4
        self.unify_repl_TBEs = False
