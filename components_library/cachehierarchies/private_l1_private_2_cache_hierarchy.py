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

from .abstract_cache_hierarchy import AbstractCacheHierarchy
from .abstract_classic_cache_hierarchy import AbstractClassicCacheHierarchy
from .abstract_two_level_cache_hierarchy import AbstractTwoLevelCacheHierarchy
from ..caches.l1dcache import L1DCache
from ..caches.l1icache import L1ICache
from ..caches.l2cache import L2Cache
from ..boards.abstract_board import AbstractBoard

from m5.objects import L2XBar, BaseCPU, BaseXBar, SystemXBar, BadAddr

from m5.params import Port

from typing import Optional, Tuple

from ..utils.override import *

class PrivateL1PrivateL2CacheHierarchy(AbstractClassicCacheHierarchy,
                                       AbstractTwoLevelCacheHierarchy):
    '''
    A cache setup where each core has a private L1 Data and Instruction Cache,
    and a private L2 cache.
    '''

    @staticmethod
    def _get_default_membus() -> SystemXBar:
        """
        A method used to obtain the default memory bus of 64 bit in width for
        the PrivateL1PrivateL2 CacheHierarchy.

        :returns: The default memory bus for the PrivateL1PrivateL2
        CacheHierarchy.

        :rtype: SystemXBar
        """
        membus = SystemXBar(width = 64)
        membus.badaddr_responder = BadAddr()
        membus.default = membus.badaddr_responder.pio
        return membus

    def __init__(self,
                 l1d_size: str,
                 l1i_size: str,
                 l2_size: str,
                 membus: Optional[BaseXBar] = _get_default_membus.__func__()
                ) -> None:
        """
        :param l1d_size: The size of the L1 Data Cache (e.g., "32kB").

        :type l1d_size: str

        :param  l1i_size: The size of the L1 Instruction Cache (e.g., "32kB").

        :type l1i_size: str

        :param l2_size: The size of the L2 Cache (e.g., "256kB").

        :type l2_size: str

        :param membus: The memory bus. This parameter is optional parameter and
        will default to a 64 bit width SystemXBar is not specified.

        :type membus: Optional[BaseXBar]
        """

        AbstractClassicCacheHierarchy.__init__(self=self)
        AbstractTwoLevelCacheHierarchy.__init__(
            self,
            l1i_size = l1i_size,
            l1i_assoc = 1, #TODO: Is this correct? I'm a Cache Hierarchy noob.
            l1d_size = l1d_size,
            l1d_assoc = 1, #TODO: Same as above.
            l2_size = l2_size,
            l2_assoc = 1, #TODO: Same as above.
        )

        self.membus = membus

    @overrides(AbstractCacheHierarchy)
    def incorporate_cache(self, board: AbstractBoard) -> None:


        #board.get_system_simobject().cache_hierarchy = self
        # Connect the membus to the system.
        #motherboard.get_system_simobject().membus = self.get_membus()

        # Set up the system port for functional access from the simulator.
        board.get_system_simobject().system_port = \
            self.get_membus().cpu_side_ports

        for cpu in board.get_processor().get_cpu_simobjects():

            # Create a memory bus, a coherent crossbar, in this case.
            cpu.l2bus = L2XBar()

            # Create an L1 instruction and data cache.
            cpu.icache = L1ICache(size = self.get_l1i_size())
            cpu.dcache = L1DCache(size = self.get_l1d_size())

            # Connect the instruction and data caches to the CPU.
            cpu.icache.connect_cpu_side(cpu)
            cpu.dcache.connect_cpu_side(cpu)

            # Hook the CPU ports up to the l2bus.
            cpu.icache.connect_bus_side(cpu.l2bus)
            cpu.dcache.connect_bus_side(cpu.l2bus)

            # Create an L2 cache and connect it to the l2bus.
            cpu.l2cache = L2Cache(size = self.get_l2_size())
            cpu.l2cache.connect_cpu_side(cpu.l2bus)

            # Connect the L2 cache to the bus.
            cpu.l2cache.connect_bus_side(self.get_membus())

            # Connect the CPU MMU's to the membus.
            cpu.mmu.connectWalkerPorts(
                self.get_membus().cpu_side_ports,
                self.get_membus().cpu_side_ports
            )

    @overrides(AbstractCacheHierarchy)
    def get_interrupt_ports(self, cpu: BaseCPU) -> Tuple[Port,Port]:
        return self.get_membus().mem_side_ports, \
               self.get_membus().cpu_side_ports

    @overrides(AbstractCacheHierarchy)
    def get_membus(self) -> BaseXBar:
        return self.membus
