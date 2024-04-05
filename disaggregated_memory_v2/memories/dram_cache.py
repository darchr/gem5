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

from gem5.components.memory.memory import ChanneledMemory
# from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.memory.dram_interfaces.hbm import TDRAM
from gem5.components.boards.abstract_board import AbstractBoard
from gem5.utils.override import overrides
from m5.objects import AddrRange, Port, PolicyManager
from typing import Type, Optional, Sequence, Tuple, List

class DRAMCacheSystem(AbstractMemorySystem):
    """
    This class creates a DRAM cache based memory system.
    It can connect two memory systems with a DRAM cache
    policy manager.
    """

    def __init__(
        self,
        loc_mem: Type[ChanneledMemory],
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
        self.policy_manager.orb_max_size = 64
        self.policy_manager.assoc = 1


        self.loc_mem = loc_mem()
        for dram in self.loc_mem._dram:
            dram.in_addr_map = False
            dram.kvm_map = False
            dram.null = True
        self.policy_manager.loc_mem = self.loc_mem._dram[0]
        self._loc_mem_controller = self.loc_mem.get_memory_controllers()[0]
        self._loc_mem_controller.dram.device_size = cache_size
        self._loc_mem_controller.dram.read_buffer_size = 64
        self._loc_mem_controller.dram.write_buffer_size = 64
        self._loc_mem_controller.consider_oldest_write= True
        self._loc_mem_controller.oldest_write_age_threshold = 2500000
        self._loc_mem_controller.static_frontend_latency = "1ns"
        self._loc_mem_controller.static_backend_latency = "1ns"
        self._loc_mem_controller.static_frontend_latency_tc = "0ns"
        self._loc_mem_controller.static_backend_latency_tc = "0ns"

        self._loc_mem_controller.port = self.policy_manager.loc_req_port


    @overrides(AbstractMemorySystem)
    def get_size(self) -> int:
        return self._size

    @overrides(AbstractMemorySystem)
    def set_memory_range(self, ranges: List[AddrRange]) -> None:
        self.policy_manager.range = ranges[0]
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
    def get_far_mem_port(self) -> Sequence[Tuple[AddrRange, Port]]:
        return [(self.policy_manager.range, self.policy_manager.far_req_port)]

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
        'CascadeLakeNoPartWrs',
        size='64GiB',
        cache_size=cache_size)

def TDRAMCache(cache_size) -> AbstractMemorySystem:
    return DRAMCacheSystem(
        SingleChannelTDRAM,
        'TDRAM',
        size='64GiB',
        cache_size=cache_size)