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
from ..boards.isas import ISA

from m5.objects import L2XBar, BaseCPU, BaseXBar, SystemXBar, BadAddr, \
                       Bridge, AddrRange, Addr, Cache

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

        # Set up the system port for functional access from the simulator.
        board.system_port = self.membus.cpu_side_ports

        #######################################################################
        # TODO: I'm really unsure about all this. Specialized to X86

        board.bridge = Bridge(delay='50ns')
        board.bridge.mem_side_port = board.get_io_bus().cpu_side_ports
        board.bridge.cpu_side_port = self.membus.mem_side_ports

        # Constants similar to x86_traits.hh
        IO_address_space_base = 0x8000000000000000
        pci_config_address_space_base = 0xc000000000000000
        interrupts_address_space_base = 0xa000000000000000
        APIC_range_size = 1 << 12

        board.bridge.ranges = \
             [
             AddrRange(0xC0000000, 0xFFFF0000),
             AddrRange(IO_address_space_base,
                       interrupts_address_space_base - 1),
             AddrRange(pci_config_address_space_base,
                       Addr.max)
             ]

        board.apicbridge = Bridge(delay='50ns')
        board.apicbridge.cpu_side_port = board.get_io_bus().mem_side_ports
        board.apicbridge.mem_side_port = self.membus.cpu_side_ports
        board.apicbridge.ranges = [AddrRange(interrupts_address_space_base,
                 interrupts_address_space_base +
                 board.get_processor().get_num_cores() * APIC_range_size
                 - 1)]

        # connect the io bus
        # TODO: This interface needs fixed. The PC should not be accessed in
        # This way.
        board.pc.attachIO(board.get_io_bus())

        # Add a tiny cache to the IO bus.
        # This cache is required for the classic memory model for coherence
        self.iocache = Cache(assoc=8,
                             tag_latency = 50,
                             data_latency = 50,
                             response_latency = 50,
                             mshrs = 20,
                             size = '1kB',
                             tgts_per_mshr = 12,
                             addr_ranges = \
                                 board.mem_ranges)

        self.iocache.cpu_side = board.get_io_bus().mem_side_ports
        self.iocache.mem_side = self.membus.cpu_side_ports

        for cntr in board.get_memory().get_memory_controllers():
            cntr.port = self.membus.mem_side_ports

        ######################################################################

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
            cpu.l2cache.connect_bus_side(self.membus)

            # Connect the CPU MMU's to the membus.
            cpu.mmu.connectWalkerPorts(
                self.membus.cpu_side_ports,
                self.membus.cpu_side_ports
            )

            # Connect the interrupt ports
            if board.get_runtime_isa() == ISA.X86 :
                int_req_port = self.membus.mem_side_ports
                int_resp_port = self.membus.cpu_side_ports
                cpu.interrupts[0].pio = int_req_port
                cpu.interrupts[0].int_requestor = int_resp_port
                cpu.interrupts[0].int_responder = int_req_port

   # @overrides(AbstractCacheHierarchy)
  #  def get_interrupt_ports(self, cpu: BaseCPU) -> Tuple[Port,Port]:
  #      return self.get_membus().mem_side_ports, \
   #            self.get_membus().cpu_side_ports

   # @overrides(AbstractCacheHierarchy)
  #  def get_membus(self) -> BaseXBar:
    #    return self.membus
