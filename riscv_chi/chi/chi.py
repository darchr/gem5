# Copyright (c) 2021 The Regents of the University of California
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

from m5.objects.SubSystem import SubSystem
from gem5.components.cachehierarchies.ruby.abstract_ruby_cache_hierarchy \
    import AbstractRubyCacheHierarchy
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.coherence_protocol import CoherenceProtocol
from gem5.isas import ISA
from gem5.runtime import get_runtime_isa
from gem5.utils.requires import requires
from gem5.utils.override import overrides
from gem5.components.boards.abstract_board import AbstractBoard

from gem5.components.cachehierarchies.ruby.topologies.simple_pt2pt import (
    SimplePt2Pt,
)

from .nodes.l1_cache import L1Cache
from .nodes.dma_requestor import DMARequestor
from .nodes.directory import SimpleDirectory
from .nodes.memory_controller import MemoryController

from m5.objects import (
    DMASequencer,
    NULL,
    RubySystem,
    RubySequencer,
    RubyPortProxy,
)


class PrivateL1CacheHierarchy(AbstractRubyCacheHierarchy):
    """A single level cache based on CHI for RISC-V

    This hierarchy has a number of L1 caches, a single directory (HNF), and as
    many memory controllers (SNF) as memory channels. The directory does not
    have an associated cache.

    The network is a simple point-to-point between all of the controllers.
    """

    def __init__(self, size: str, assoc: int) -> None:
        """
        :param size: The size of each cache in the hierarchy.
        :param assoc: The associativity of each cache.
        """
        super().__init__()

        self._size = size
        self._assoc = assoc

    @overrides(AbstractCacheHierarchy)
    def incorporate_cache(self, board: AbstractBoard) -> None:

        requires(coherence_protocol_required=CoherenceProtocol.CHI)

        self.ruby_system = RubySystem()

        # Ruby's global network.
        self.ruby_system.network = SimplePt2Pt(self.ruby_system)

        # Network configurations
        # virtual networks: 0=request, 1=snoop, 2=response, 3=data
        self.ruby_system.number_of_virtual_networks = 4
        self.ruby_system.network.number_of_virtual_networks = 4

        # Create a single centralized directory
        self.directory = SimpleDirectory(
            self.ruby_system.network,
            cache_line_size=board.get_cache_line_size(),
            clk_domain=board.get_clock_domain(),
        )
        self.directory.ruby_system = self.ruby_system

        # There is a single global list of all of the controllers to make it
        # easier to connect everything to the global network. This can be
        # customized depending on the topology/network requirements.
        # Create one controller for each L1 cache (and the cache mem obj.)
        # Create a single directory controller (Really the memory cntrl).
        self._clusters = []
        for i, core in enumerate(board.get_processor().get_cores()):
            cluster = SubSystem()
            cluster.dcache = L1Cache(
                size=self._size,
                assoc=self._assoc,
                network=self.ruby_system.network,
                core=core,
                cache_line_size=board.get_cache_line_size(),
                target_isa=get_runtime_isa(),
                clk_domain=board.get_clock_domain(),
            )
            cluster.icache = L1Cache(
                size=self._size,
                assoc=self._assoc,
                network=self.ruby_system.network,
                core=core,
                cache_line_size=board.get_cache_line_size(),
                target_isa=get_runtime_isa(),
                clk_domain=board.get_clock_domain(),
            )

            cluster.icache.sequencer = RubySequencer(
                version=i,
                dcache=NULL,
                clk_domain=cluster.icache.clk_domain,
            )
            cluster.dcache.sequencer = RubySequencer(
                version=i,
                dcache=cluster.dcache.cache,
                clk_domain=cluster.dcache.clk_domain,
            )

            if board.has_io_bus():
                cluster.dcache.sequencer.connectIOPorts(board.get_io_bus())

            cluster.dcache.ruby_system = self.ruby_system
            cluster.icache.ruby_system = self.ruby_system

            core.connect_icache(cluster.icache.sequencer.in_ports)
            core.connect_dcache(cluster.dcache.sequencer.in_ports)

            core.connect_walker_ports(
                cluster.dcache.sequencer.in_ports,
                cluster.icache.sequencer.in_ports,
            )

            # Connect the interrupt ports
            if get_runtime_isa() == ISA.X86:
                int_req_port = cluster.dcache.sequencer.interrupt_out_port
                int_resp_port = cluster.dcache.sequencer.in_ports
                core.connect_interrupt(int_req_port, int_resp_port)
            else:
                core.connect_interrupt()

            cluster.dcache.downstream_destinations = [self.directory]
            cluster.icache.downstream_destinations = [self.directory]

            self._clusters.append(cluster)

        self.core_clusters = self._clusters

        # Create the coherent side of the memory controllers
        rng, port = board.get_memory().get_mem_ports()[0]
        self.memory_controller = MemoryController(
            self.ruby_system.network,
            rng,
            port,
        )
        self.memory_controller.ruby_system = self.ruby_system

        self.directory.downstream_destinations = self.memory_controller

        # Create the DMA Controllers, if required.
        self._dma_controllers = []
        if board.has_dma_ports():
            dma_ports = board.get_dma_ports()
            for i, port in enumerate(dma_ports):
                ctrl = DMARequestor(
                    self.ruby_system.network,
                    board.get_cache_line_size(),
                    board.get_clock_domain(),
                )
                version = len(board.get_processor().get_cores()) + i
                ctrl.sequencer = RubySequencer(
                    version=version,
                    in_ports=port
                )
                ctrl.sequencer.dcache = NULL

                ctrl.ruby_system = self.ruby_system
                ctrl.sequencer.ruby_system = self.ruby_system

                ctrl.downstream_destinations = [self.directory]

                self._dma_controllers.append(ctrl)

        self.ruby_system.num_of_sequencers = len(self.core_clusters) * 2 + len(
            self._dma_controllers
        )

        if len(self._dma_controllers) != 0:
            self.dma_controllers = self._dma_controllers

        self.ruby_system.network.connectControllers(
            list(
                chain.from_iterable(
                    [
                        (cluster.dcache, cluster.icache)
                        for cluster in self.core_clusters
                    ]
                )
            )
            + [self.directory, self.memory_controller]
            + self.dma_controllers
            if self._dma_controllers
            else []
        )
        self.ruby_system.network.setup_buffers()

        # Set up a proxy port for the system_port. Used for load binaries and
        # other functional-only things.
        self.ruby_system.sys_port_proxy = RubyPortProxy()
        board.connect_system_port(self.ruby_system.sys_port_proxy.in_ports)
