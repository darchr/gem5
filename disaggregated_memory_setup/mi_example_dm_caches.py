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

from gem5.components.cachehierarchies.ruby.mi_example_cache_hierarchy import (
    MIExampleCacheHierarchy,
)
from gem5.components.cachehierarchies.ruby.caches.mi_example.l1_cache import (
    L1Cache,
)
from gem5.components.cachehierarchies.ruby.caches.mi_example.dma_controller import (
    DMAController,
)
from gem5.components.cachehierarchies.ruby.caches.mi_example.directory import (
    Directory,
)
from gem5.components.cachehierarchies.ruby.topologies.simple_pt2pt import (
    SimplePt2Pt,
)

# from gem5.components.cachehierarchies.ruby.abstract_ruby_cache_hierarchy import AbstractRubyCacheHierarchy
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.components.boards.abstract_board import AbstractBoard
from gem5.coherence_protocol import CoherenceProtocol
from gem5.isas import ISA
from gem5.utils.override import overrides
from gem5.utils.requires import requires
from m5.objects import RubySystem, RubySequencer, DMASequencer, RubyPortProxy

# from gem5.components.cachehierarchies.classic.caches.l1dcache import L1DCache
# from gem5.components.cachehierarchies.classic.caches.l1icache import L1ICache
# from gem5.components.cachehierarchies.classic.caches.l2cache import L2Cache
# from gem5.components.cachehierarchies.classic.caches.mmu_cache import MMUCache
# from gem5.components.boards.abstract_board import AbstractBoard
# from gem5.isas import ISA
# from m5.objects import Cache, L2XBar, BaseXBar, SystemXBar, BadAddr, Port

# from gem5.utils.override import overrides


class MIExampleDMCache(MIExampleCacheHierarchy):
    def __init__(self, size: str, assoc: str):
        """
        :param size: The size of each cache in the heirarchy.
        :param assoc: The associativity of each cache.
        """
        super().__init__(size, assoc)

    @overrides(MIExampleCacheHierarchy)
    def incorporate_cache(self, board: AbstractBoard) -> None:

        requires(coherence_protocol_required=CoherenceProtocol.MI_EXAMPLE)

        self.ruby_system = RubySystem()

        # Ruby's global network.
        self.ruby_system.network = SimplePt2Pt(self.ruby_system)

        # MI Example users 5 virtual networks.
        self.ruby_system.number_of_virtual_networks = 5
        self.ruby_system.network.number_of_virtual_networks = 5

        # There is a single global list of all of the controllers to make it
        # easier to connect everything to the global network. This can be
        # customized depending on the topology/network requirements.
        # Create one controller for each L1 cache (and the cache mem obj.)
        # Create a single directory controller (Really the memory cntrl).
        self._controllers = []
        for i, core in enumerate(board.get_processor().get_cores()):
            cache = L1Cache(
                size=self._size,
                assoc=self._assoc,
                network=self.ruby_system.network,
                core=core,
                cache_line_size=board.get_cache_line_size(),
                target_isa=board.get_processor().get_isa(),
                clk_domain=board.get_clock_domain(),
            )

            cache.sequencer = RubySequencer(
                version=i,
                dcache=cache.cacheMemory,
                clk_domain=cache.clk_domain,
            )

            if board.has_io_bus():
                cache.sequencer.connectIOPorts(board.get_io_bus())

            cache.ruby_system = self.ruby_system

            core.connect_icache(cache.sequencer.in_ports)
            core.connect_dcache(cache.sequencer.in_ports)

            core.connect_walker_ports(
                cache.sequencer.in_ports, cache.sequencer.in_ports
            )

            # Connect the interrupt ports
            if board.get_processor().get_isa() == ISA.X86:
                int_req_port = cache.sequencer.interrupt_out_port
                int_resp_port = cache.sequencer.in_ports
                core.connect_interrupt(int_req_port, int_resp_port)
            else:
                core.connect_interrupt()

            cache.ruby_system = self.ruby_system
            self._controllers.append(cache)

        # Create the directory controllers
        self._directory_controllers = []
        for range, port in board.get_mem_ports():
            dir = Directory(
                self.ruby_system.network,
                board.get_cache_line_size(),
                range,
                port,
            )
            dir.ruby_system = self.ruby_system
            self._directory_controllers.append(dir)

        for range, port in board.get_remote_mem_ports():
            dir = Directory(
                self.ruby_system.network,
                board.get_cache_line_size(),
                range,
                port,
            )
            dir.ruby_system = self.ruby_system
            self._directory_controllers.append(dir)

        # Create the DMA Controllers, if required.
        self._dma_controllers = []
        if board.has_dma_ports():
            dma_ports = board.get_dma_ports()
            for i, port in enumerate(dma_ports):
                ctrl = DMAController(
                    self.ruby_system.network, board.get_cache_line_size()
                )
                ctrl.dma_sequencer = DMASequencer(version=i, in_ports=port)

                ctrl.ruby_system = self.ruby_system
                ctrl.dma_sequencer.ruby_system = self.ruby_system

                self._dma_controllers.append(ctrl)

        self.ruby_system.num_of_sequencers = len(self._controllers) + len(
            self._dma_controllers
        )

        # Connect the controllers.
        self.ruby_system.controllers = self._controllers
        self.ruby_system.directory_controllers = self._directory_controllers

        if len(self._dma_controllers) != 0:
            self.ruby_system.dma_controllers = self._dma_controllers

        self.ruby_system.network.connectControllers(
            self._controllers
            + self._directory_controllers
            + self._dma_controllers
        )
        self.ruby_system.network.setup_buffers()

        # Set up a proxy port for the system_port. Used for load binaries and
        # other functional-only things.
        self.ruby_system.sys_port_proxy = RubyPortProxy()
        board.connect_system_port(self.ruby_system.sys_port_proxy.in_ports)
