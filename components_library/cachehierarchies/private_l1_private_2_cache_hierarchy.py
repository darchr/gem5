# Copyright (c) 2021 The Regents of the University of California
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

from components_library.caches.mmu_cache import MMUCache
from .abstract_classic_cache_hierarchy import AbstractClassicCacheHierarchy
from ..caches.l1dcache import L1DCache
from ..caches.l1icache import L1ICache
from ..caches.l2cache import L2Cache
from ..motherboards.abstract_motherboard import AbstractMotherboard

from m5.objects import L2XBar

from m5.params import Port

from typing import Optional, Tuple

class PrivateL1PrivateL2CacheHierarchy(AbstractClassicCacheHierarchy):
    '''
    A cache setup, where each core has a private Data and Instruction Cache.
    '''

    def __init__(self,
                 l1d_size: str,
                 l1i_size: str,
                 l2_size: str,
                ) -> None:

        self._l1d_size = l1d_size
        self._l1i_size = l1i_size
        self._l2_size = l2_size

    def incorporate_cache(self, motherboard: AbstractMotherboard) -> None:
        for cpu in motherboard.get_processor().get_cpu_simobjects():

            # Create a memory bus, a coherent crossbar, in this case
            cpu.l2bus = L2XBar()

            # Create an L1 instruction and data cache
            cpu.icache = L1ICache(size = self._l1i_size)
            cpu.dcache = L1DCache(size = self._l1d_size)

            # Connect the instruction and data caches to the CPU
            cpu.icache.connect_cpu_side(cpu)
            cpu.dcache.connect_cpu_side(cpu)

            # Hook the CPU ports up to the l2bus
            cpu.icache.connect_bus_side(cpu.l2bus)
            cpu.dcache.connect_bus_side(cpu.l2bus)

            # Create an L2 cache and connect it to the l2bus
            cpu.l2cache = L2Cache(size = self._l2_size)
            cpu.l2cache.connect_cpu_side(cpu.l2bus)

            # Connect the L2 cache to the bus
            cpu.l2cache.connect_bus_side(motherboard.get_membus())

            # Connect the CPU MMU's to the membus
            cpu.mmu.connectWalkerPorts(
                motherboard.get_membus().cpu_side_ports,
                motherboard.get_membus().cpu_side_ports
            )

    def get_interrupt_ports(self) -> Tuple[Port,Port]:
        # HACK: this should return the below, but it's in the system...
        # self.membus.mem_side_ports, self.membus.cpu_side_ports
        return None
