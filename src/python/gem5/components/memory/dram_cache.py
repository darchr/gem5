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

""" DRAM Cache based memory system
    Uses Policy Manager and two other memory systems
"""

from .memory import ChanneledMemory
from .abstract_memory_system import AbstractMemorySystem
from ..boards.abstract_board import AbstractBoard
from math import log
from ...utils.override import overrides
from m5.objects import AddrRange, DRAMInterface, Port, PolicyManager, L2XBar, IOXBar
from typing import Type, Optional, Union, Sequence, Tuple, List
from .memory import _try_convert
from .dram_interfaces.hbm import TDRAM
from .dram_interfaces.ddr4 import DDR4_2400_8x8
from .multi_channel import DualChannelDDR4_2400
from .single_channel import SingleChannelDDR4_2400

class DRAMCacheSystem(AbstractMemorySystem):
    """
    This class creates a DRAM cache based memory system.
    It can connect two memory systems with a DRAM cache
    policy manager.
    """

    def __init__(
        self,
        loc_mem: Type[ChanneledMemory],
        far_mem: Type[ChanneledMemory],
        loc_mem_policy: [str] = None,
        size: [str] = None,
        cache_size: [str] = None,
    ) -> None:
        """
        :param loc_mem_policy: DRAM cache policy to be used
        :param size: Optionally specify the size of the DRAM controller's
            address space. By default, it starts at 0 and ends at the size of
            the DRAM device specified
        """
        super().__init__()

        self._size = size

        self.policy_manager = PolicyManager()
        self.policy_manager.static_frontend_latency = "10ns"
        self.policy_manager.static_backend_latency = "10ns"


        self.policy_manager.loc_mem_policy = loc_mem_policy
        self.policy_manager.bypass_dcache = False

        self.policy_manager.dram_cache_size = cache_size
        self.policy_manager.cache_warmup_ratio = 0.95

        self.loc_mem = loc_mem()
        self.far_mem = far_mem()

        for dram in self.loc_mem._dram:
            dram.in_addr_map = False
            dram.null = True
            #dram.range = AddrRange('1GiB')

        # TODO: this loc_mem in policy manager
        # is a single DRAM interface, which probably
        # needs to be updated for a multi-channel local
        # memory, the stdlib component can then be updated.
        self.policy_manager.loc_mem = self.loc_mem._dram[0]

        self.policy_manager.orb_max_size = 128

        for dram in self.far_mem._dram:
            dram.in_addr_map = False
            dram.null = True
            # DRAM interface DDR4_2400_16x4
            # by default has a write buffer size of 128
            dram.read_buffer_size = 64
            dram.write_buffer_size = 64

        self._loc_mem_controller = self.loc_mem.get_memory_controllers()[0]
        self._far_mem_controller = self.far_mem.get_memory_controllers()[0]

        self._loc_mem_controller.consider_oldest_write= True
        self._loc_mem_controller.static_frontend_latency = "1ns"
        self._loc_mem_controller.static_backend_latency = "1ns"

        self._far_mem_controller.static_frontend_latency = "1ns"
        self._far_mem_controller.static_backend_latency = "1ns"

        self._loc_mem_controller.port = self.policy_manager.loc_req_port
        self._far_mem_controller.port = self.policy_manager.far_req_port

        # If need to use XBar
        #self.farMemXBar = L2XBar(width=64)
        #self.nearMemXBar = L2XBar(width=64)

        #self.policy_manager.far_req_port = self.farMemXBar.cpu_side_ports
        #self.policy_manager.loc_req_port = self.nearMemXBar.cpu_side_ports

        #for ctrl in self.loc_mem.get_memory_controllers():
        #    self.nearMemXBar.mem_side_ports = ctrl.port

        #for ctrl in self.far_mem.get_memory_controllers():
        #    self.farMemXBar.mem_side_ports = ctrl.port

    @overrides(AbstractMemorySystem)
    def get_size(self) -> int:
        return self._size

    @overrides(AbstractMemorySystem)
    def set_memory_range(self, ranges: List[AddrRange]) -> None:

        self.policy_manager.range = ranges[0]
        for dram in self.far_mem._dram:
            dram.range = ranges[0]
        for dram in self.loc_mem._dram:
            dram.range = ranges[0]

    @overrides(AbstractMemorySystem)
    def incorporate_memory(self, board: AbstractBoard) -> None:
        pass

    @overrides(AbstractMemorySystem)
    def get_memory_controllers(self):
        return [self.policy_manager]

    @overrides(AbstractMemorySystem)
    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return [(self.policy_manager.range, self.policy_manager.port)]

def SingleChannelTDRAM(
    size: Optional[str] = None,
) -> AbstractMemorySystem:
    if not size:
        size = "1GiB"
    return ChanneledMemory(
        TDRAM,
        1,
        64,
        size=size
    )


def CascadeLakeCache(cache_size) -> AbstractMemorySystem:
    return DRAMCacheSystem(
        SingleChannelTDRAM,
        SingleChannelDDR4_2400,
        'CascadeLakeNoPartWrs',
        size='64GiB',
        cache_size=cache_size)

def OracleCache(cache_size) -> AbstractMemorySystem:
    return DRAMCacheSystem(
        SingleChannelTDRAM,
        SingleChannelDDR4_2400,
        'Oracle',
        size='64GiB',
        cache_size=cache_size)

def TDRAMNPCache(cache_size) -> AbstractMemorySystem:
    return DRAMCacheSystem(
        SingleChannelTDRAM,
        SingleChannelDDR4_2400,
        'TDRAM_NP',
        size='64GiB',
        cache_size=cache_size)