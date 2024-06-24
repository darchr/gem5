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

from m5.objects.SubSystem import SubSystem
from gem5.components.cachehierarchies.ruby.abstract_ruby_cache_hierarchy import (
    AbstractRubyCacheHierarchy,
)
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.coherence_protocol import CoherenceProtocol
from gem5.isas import ISA
from gem5.utils.requires import requires
from gem5.utils.override import overrides
from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.processors.abstract_core import AbstractCore

from gem5.components.cachehierarchies.ruby.topologies.simple_pt2pt import (
    SimplePt2Pt,
)

from .core_complex import CoreComplex
from .network import SagaNetwork
from gem5.components.cachehierarchies.chi.nodes.dma_requestor import (
    DMARequestor,
)
from gem5.components.cachehierarchies.chi.nodes.directory import (
    SimpleDirectory,
)
from gem5.components.cachehierarchies.chi.nodes.memory_controller import (
    MemoryController,
)

from m5.objects import NULL, RubySystem, RubySequencer, RubyPortProxy


class SagaCacheHierarchy(AbstractRubyCacheHierarchy):
    """A cache hierarchy based on the AMD Epyc system.

    This hierarchy has private L1/L2 for each core, a shared victim L3 that is
    shared between a "CCX" of 8 cores and a shared full directory between all
    CCX. The directory is sliced the same way as the memory controllers.

    The L1 I/D are both 32 KiB, L2 is 512 KiB (per core), and L3 is 32 MiB per
    CCX. These are number from Zen3.

    Assumptions/requirements:
    - The number of cores is divisible by 8. If this is not the case, then the
      "last" CCX will have fewer than 8 cores.
    - The number of memory controllers is a power of two.
    """

    def __init__(self) -> None:
        """ """
        super().__init__()

        self._cores_per_cluster = 8

    @overrides(AbstractCacheHierarchy)
    def incorporate_cache(self, board: AbstractBoard) -> None:

        requires(coherence_protocol_required=CoherenceProtocol.CHI)

        self.ruby_system = RubySystem()

        # Network configurations
        # virtual networks: 0=request, 1=snoop, 2=response, 3=data
        self.ruby_system.number_of_virtual_networks = 4

        # Ruby's global network.
        self.ruby_system.network = SagaNetwork(
            self.ruby_system, self.ruby_system.number_of_virtual_networks
        )

        # Create the coherent side of the memory controllers
        (
            self.directories,
            self.memory_controllers,
        ) = self._create_memory_controllers(board)
        for d, ctrl in zip(self.directories, self.memory_controllers):
            d.downstream_destinations = [ctrl]

        # Create the CCXs
        cores = board.get_processor().get_cores()
        ccxs = []
        for i in range(len(cores) // self._cores_per_cluster + 1):
            this_ccx_cores = cores[
                i * self._cores_per_cluster : (i + 1) * self._cores_per_cluster
            ]
            if not this_ccx_cores:
                break
            ccxs.append(
                CoreComplex(
                    this_ccx_cores,
                    board,
                    self.ruby_system.network,
                    self.directories,
                    self.ruby_system,
                )
            )
        self.core_complexes = ccxs

        # Create the DMA Controllers, if required.
        if board.has_dma_ports():
            print("the board has dma ports")
            self.dma_controllers = self._create_dma_controllers(board)
            self.ruby_system.num_of_sequencers = len(cores) * 2 + len(
                self.dma_controllers
            )
        else:
            self.ruby_system.num_of_sequencers = len(cores) * 2

        self.ruby_system.network.connect_ccx(self.core_complexes)
        self.ruby_system.network.connect_directory_memory(
            self.directories, self.memory_controllers
        )
        self.ruby_system.network.connect_dma(
            (self.dma_controllers if board.has_dma_ports() else [])
        )
        self.ruby_system.network.finalize()

        self.ruby_system.network.setup_buffers()

        # Set up a proxy port for the system_port. Used for load binaries and
        # other functional-only things.
        self.ruby_system.sys_port_proxy = RubyPortProxy()
        board.connect_system_port(self.ruby_system.sys_port_proxy.in_ports)

    def _create_memory_controllers(
        self, board: AbstractBoard
    ) -> Tuple[List[SimpleDirectory], List[MemoryController]]:

        memory_controllers = []
        directories = []
        # mem_range_ports = []
        # mem_range_ports.append(board.get_bootmem_ports())
        # mem_range_ports.append(board.get_local_memory_ports())
        # mem_range_ports.append(board.get_remote_memory_ports())
        for rng_port in board.get_mem_ports():
            for rng, port in rng_port:
                GB=1024*1024*1024
                print(f"Memory range: {rng.start/GB}  {rng.end/GB}  {rng.size()/GB}")
                print("*******************************************************************")
                mc = MemoryController(self.ruby_system.network, rng, port)
                mc.ruby_system = self.ruby_system
                memory_controllers.append(mc)

                d = SimpleDirectory(
                    self.ruby_system.network,
                    cache_line_size=board.get_cache_line_size(),
                    clk_domain=board.get_clock_domain(),
                )
                d.addr_ranges = [rng]
                d.ruby_system = self.ruby_system
                directories.append(d)
        return directories, memory_controllers

    def _create_dma_controllers(
        self, board: AbstractBoard
    ) -> List[DMARequestor]:
        dma_controllers = []
        for i, port in enumerate(board.get_dma_ports()):
            ctrl = DMARequestor(
                self.ruby_system.network,
                board.get_cache_line_size(),
                board.get_clock_domain(),
            )
            version = len(board.get_processor().get_cores()) + i
            ctrl.sequencer = RubySequencer(version=version, in_ports=port)
            ctrl.sequencer.dcache = NULL

            ctrl.ruby_system = self.ruby_system
            ctrl.sequencer.ruby_system = self.ruby_system

            ctrl.downstream_destinations = self.directories

            dma_controllers.append(ctrl)

        return dma_controllers
