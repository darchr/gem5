# Copyright (c) 2022 The Regents of the University of California
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


from m5.objects import (
    DMASequencer,
    RubyPortProxy,
    RubySequencer,
    RubySystem,
)

from ....coherence_protocol import CoherenceProtocol
from ....utils.override import overrides
from ....utils.requires import requires

requires(coherence_protocol_required=CoherenceProtocol.MESI_THREE_LEVEL)

from ....isas import ISA
from ...boards.abstract_board import AbstractBoard
from ..abstract_cache_hierarchy import AbstractCacheHierarchy
from ..abstract_three_level_cache_hierarchy import (
    AbstractThreeLevelCacheHierarchy,
)
from .abstract_ruby_cache_hierarchy import AbstractRubyCacheHierarchy
from .caches.mesi_three_level.directory import Directory
from .caches.mesi_three_level.dma_controller import DMAController
from .caches.mesi_three_level.l1_cache import L1Cache
from .caches.mesi_three_level.l2_cache import L2Cache
from .caches.mesi_three_level.l3_cache import L3Cache
from .topologies.sensible_network import SensibleNetwork


class MESIThreeLevelCacheHierarchy(
    AbstractRubyCacheHierarchy, AbstractThreeLevelCacheHierarchy
):
    """A three-level private-L1-private-L2-shared-L3 MESI hierarchy.

    The on-chip network is a point-to-point all-to-all simple network.
    """

    _sequencer_version = 0
    _dma_sequencer_version = 0

    @classmethod
    def get_next_sequencer_version(cls):
        cls._sequencer_version += 1
        return cls._sequencer_version - 1

    @classmethod
    def get_next_dma_sequencer_version(cls):
        cls._dma_sequencer_version += 1
        return cls._dma_sequencer_version - 1

    def __init__(
        self,
        l1i_size: str,
        l1i_assoc: int,
        l1d_size: str,
        l1d_assoc: int,
        l2_size: str,
        l2_assoc: int,
        l3_size: str,
        l3_assoc: int,
        num_l3_banks: int,
        cores_per_die: int,
        channels_per_die: int,
        has_remote_memory: bool,
    ):
        AbstractRubyCacheHierarchy.__init__(self=self)
        AbstractThreeLevelCacheHierarchy.__init__(
            self,
            l1i_size=l1i_size,
            l1i_assoc=l1i_assoc,
            l1d_size=l1d_size,
            l1d_assoc=l1d_assoc,
            l2_size=l2_size,
            l2_assoc=l2_assoc,
            l3_size=l3_size,
            l3_assoc=l3_assoc,
        )

        self._num_l3_banks = num_l3_banks
        self._cores_per_die = cores_per_die
        self._channels_per_die = channels_per_die
        self._has_remote_memory = has_remote_memory

    @overrides(AbstractCacheHierarchy)
    def get_coherence_protocol(self):
        return CoherenceProtocol.MESI_THREE_LEVEL

    def incorporate_cache(self, board: AbstractBoard) -> None:
        super().incorporate_cache(board)
        cache_line_size = board.get_cache_line_size()

        self.ruby_system = RubySystem()

        # MESI_Three_Level needs 3 virtual networks
        self.ruby_system.number_of_virtual_networks = 3

        self.ruby_system.network = SensibleNetwork(self.ruby_system)
        self.ruby_system.network.number_of_virtual_networks = 3

        self._core_cluster_routers = []
        self._channel_routers = []
        self._remote_routers = []
        self._dma_routers = []

        cores = board.get_processor().get_cores()
        self.l1_controllers = [
            L1Cache(
                l1i_size=self._l1i_size,
                l1i_assoc=self._l1i_assoc,
                l1d_size=self._l1d_size,
                l1d_assoc=self._l1d_assoc,
                network=self.ruby_system.network,
                core=core,
                cache_line_size=cache_line_size,
                clk_domain=board.get_clock_domain(),
            )
            for core in cores
        ]
        self.l2_controllers = [
            L2Cache(
                l2_size=self._l2_size,
                l2_assoc=self._l2_assoc,
                network=self.ruby_system.network,
                num_l3Caches=self._num_l3_banks,
                cache_line_size=cache_line_size,
                clk_domain=board.get_clock_domain(),
            )
            for _ in range(len(cores))
        ]
        for core, l1_cache, l2_cache in zip(
            cores, self.l1_controllers, self.l2_controllers
        ):
            l1_cache.sequencer = RubySequencer(
                version=MESIThreeLevelCacheHierarchy.get_next_sequencer_version(),
                dcache=l1_cache.Dcache,
                clk_domain=l1_cache.clk_domain,
                ruby_system=self.ruby_system,
            )

            if board.has_io_bus():
                l1_cache.sequencer.connectIOPorts(board.get_io_bus())

            l1_cache.ruby_system = self.ruby_system
            core.connect_icache(l1_cache.sequencer.in_ports)
            core.connect_dcache(l1_cache.sequencer.in_ports)

            core.connect_walker_ports(
                l1_cache.sequencer.in_ports, l1_cache.sequencer.in_ports
            )

            # Connect the interrupt ports
            if board.get_processor().get_isa() == ISA.X86:
                int_req_port = l1_cache.sequencer.interrupt_out_port
                int_resp_port = l1_cache.sequencer.in_ports
                core.connect_interrupt(int_req_port, int_resp_port)
            else:
                core.connect_interrupt()

            l2_cache.ruby_system = self.ruby_system
            # L0Cache in the ruby backend is l1 cache in stdlib
            # L1Cache in the ruby backend is l2 cache in stdlib
            l2_cache.bufferFromL0 = l1_cache.bufferToL1
            l2_cache.bufferToL0 = l1_cache.bufferFromL1

            self._core_cluster_routers.append(
                self.ruby_system.network.add_core_cluster(
                    l1cache=l1_cache, l2cache=l2_cache
                )
            )

        self.l3_controllers = [
            L3Cache(
                l3_size=self._l3_size,
                l3_assoc=self._l3_assoc,
                network=self.ruby_system.network,
                ruby_system=self.ruby_system,
                num_l3Caches=self._num_l3_banks,
                cache_line_size=cache_line_size,
            )
            for _ in range(self._num_l3_banks)
        ]
        self.directory_controllers = [
            Directory(
                self.ruby_system.network,
                self.ruby_system,
                cache_line_size,
                range,
                port,
            )
            for range, port in board.get_mem_ports()
        ]

        if self._has_remote_memory:
            local_directory_controllers = self.directory_controllers[:-1]
            remote_directory_controllers = self.directory_controllers[-1:]
        else:
            local_directory_controllers = self.directory_controllers
            remote_directory_controllers = []

        if self._num_l3_banks % len(local_directory_controllers) != 0:
            print(f"self._num_l3_banks: {self._num_l3_banks}")
            print(f"len(board.get_mem_ports()): {len(board.get_mem_ports())}")
            raise ValueError(
                "Number of L3 banks must be divisible by the number of memory "
                "controllers"
            )
        l3_banks_per_channel = self._num_l3_banks // len(
            local_directory_controllers
        )
        for channel_id in range(len(local_directory_controllers)):
            self._channel_routers.append(
                self.ruby_system.network.add_uncore_cluster(
                    [local_directory_controllers[channel_id]]
                    + self.l3_controllers[
                        channel_id
                        * l3_banks_per_channel : (channel_id + 1)
                        * l3_banks_per_channel
                    ]
                )
            )

        self._remote_routers.append(
            self.ruby_system.network.add_uncore_cluster(
                remote_directory_controllers
            )
        )

        if board.has_dma_ports():
            self.dma_controllers = [
                DMAController(
                    DMASequencer(
                        version=MESIThreeLevelCacheHierarchy.get_next_dma_sequencer_version(),
                        in_ports=port,
                        ruby_system=self.ruby_system,
                    ),
                    self.ruby_system,
                )
                for port in board.get_dma_ports()
            ]
            self._dma_routers
            self.ruby_system.num_of_sequencers = len(
                self.l1_controllers
            ) + len(self.dma_controllers)
        else:
            self.ruby_system.num_of_sequencers = len(self.l1_controllers)

        if (
            len(self._core_cluster_routers) // self._cores_per_die
            != len(self._channel_routers) // self._channels_per_die
        ):
            raise ValueError(
                "This cache hierachy includes one or multiple dies. The number"
                " of dies is determined by dividing the number of cores by "
                "`cores_per_die`. Since there can only be an integer number of"
                " memory controllers per die, the number of memory channels "
                "should be divisible by the number of dies. In other words:\n"
                "\tnum_cores/cores_per_die == num_mem_channels/channels_per_die"
            )
        num_dies = len(self._core_cluster_routers) // self._cores_per_die
        for die_number in range(num_dies):
            self.ruby_system.network.make_dance_hall(
                self._core_cluster_routers[
                    die_number
                    * self._cores_per_die : (die_number + 1)
                    * self._cores_per_die
                ],
                self._channel_routers[
                    die_number
                    * self._channels_per_die : (die_number + 1)
                    * self._channels_per_die
                ],
            )

        if board.has_dma_ports():
            self.ruby_system.network.make_all_to_all(
                self._channel_routers
                + self._remote_routers
                + self._dma_routers
            )
        else:
            self.ruby_system.network.make_all_to_all(
                self._channel_routers + self._remote_routers
            )

        self.ruby_system.network.finalize()
        self.ruby_system.network.setup_buffers()

        # Set up a proxy port for the system_port. Used for load binaries and
        # other functional-only things.
        self.ruby_system.sys_port_proxy = RubyPortProxy(
            ruby_system=self.ruby_system
        )
        board.connect_system_port(self.ruby_system.sys_port_proxy.in_ports)

    @overrides(AbstractRubyCacheHierarchy)
    def _reset_version_numbers(self):
        MESIThreeLevelCacheHierarchy._sequencer_version = 0
        MESIThreeLevelCacheHierarchy._dma_sequencer_version = 0
        Directory._version = 0
        L1Cache._version = 0
        L2Cache._version = 0
        L3Cache._version = 0
        DMAController._version = 0
