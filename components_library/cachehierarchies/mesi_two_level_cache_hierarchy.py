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
from .abstract_two_level_cache_hierarchy import AbstractTwoLevelCacheHierarchy
from .abstract_ruby_cache_hierarhcy import AbstractRubyCacheHierarchy
from ..boards.abstract_board import AbstractBoard
from ..boards.isas import ISA
from ..boards.coherence_protocol import CoherenceProtocol

from .ruby.topologies.pt2pt import SimplePt2Pt

from .ruby.MESI_Two_Level import L1Cache, L2Cache, Directory, DMAController

from ..utils.override import *

from m5.objects import (
    RubySystem,
    RubySequencer,
    BaseCPU,
    DMASequencer,
    RubyPortProxy,
    MyNetwork,
    DirController
)

from m5.params import Port
from m5.util.convert import toMemorySize

from typing import Tuple


class MESITwoLevelCacheHierarchy(AbstractRubyCacheHierarchy,
                                 AbstractTwoLevelCacheHierarchy):
    """
    A two level private L1 shared L2 MESI hierarchy.

    In addition to the normal two level parameters, you can also change the
    number of L2 banks in this protocol.

    The on-chip network is a point-to-point all-to-all simple network.
    """

    def __init__(
        self,
        l1i_size: str,
        l1i_assoc: int,
        l1d_size: str,
        l1d_assoc: int,
        l2_size: str,
        l2_assoc: int,
        num_l2_banks: int,
    ):
        """
        :param l1i_size: The size of the L1 Instruction cache (e.g. "32kB").

        :type l1i_size: str

        :param l1i_assoc:

        :type l1i_assoc: int

        :param l1dsize: The size of the LL1 Data cache (e.g. "32kB").

        :type l1dsize: str

        :param l1d_assoc:

        :type l1d_assoc: int

        :param l2_size: The size of the L2 cache (e.g., "256kB").

        :type l2_size: str

        :param l2_assoc:

        :type l2_assoc: int

        :param num_l2_banks: The number of L2 banks.

        :type num_l2_banks: int
        """

        AbstractTwoLevelCacheHierarchy.__init__(
            self = self,
            l1i_size = l1i_size,
            l1i_assoc = l1i_assoc,
            l1d_size = l1d_size,
            l1d_assoc = l1d_assoc,
            l2_size = l2_size,
            l2_assoc = l2_assoc,
            )

        self._num_l2_banks = num_l2_banks
        # TODO: check to be sure that the size of the cache is divisible
        # by the number of banks
        l2_bank_size = toMemorySize(l2_size) // num_l2_banks
        MESITwoLevelCacheHierarchy.__init__(
            self=self,
            l1i_size = l1i_size,
            l1i_assoc = l1i_assoc,
            l1d_size = l1d_size,
            l1d_assoc = l1d_assoc,
            l2_size = str(l2_bank_size)+'B',
            l2_assoc = l2_assoc,
        )


    @overrides(AbstractCacheHierarchy)
    def incorporate_cache(self, board: AbstractBoard) -> None:

        if board.get_runtime_coherence_protocol() != \
            CoherenceProtocol.MESI_TWO_LEVEL:
            AssertionError("The MESI_Two_Level Cache Hierarchy must be run"
                           " with the MESI_Two_Level gem5 compilation "
                           "variant")

        # Ruby's global network.
        self.network = MyNetwork(self)

        # MESI_Two_Level example uses 5 virtual networks
        # TOD: Can't this just be deleted?
        self.number_of_virtual_networks = 5
        self.network.number_of_virtual_networks = 5

        # TODO: This is hardcoded but shouldn't be. We need a way to access DMA
        # ports.
        dma_ports = [board.pc.south_bridge.ide.dma, board.iobus.mem_side_ports]

        # There is a single global list of all of the controllers to make it
        # easier to connect everything to the global network. This can be
        # customized depending on the topology/network requirements.
        # L1 caches are private to a core, hence there are one L1 cache per CPU
        # core. The number of L2 caches are dependent to the architecture.
        self.controllers = \
            [L1Cache(board, self, cpu, 1) for cpu in board.get_processor()\
                .get_cpu_simobjects()] + \
            [L2Cache(board, self, 1) for num in \
            range(1)] + [DirController(self, \
            board.mem_ranges, board.get_memory().get_memory_controllers())] \
                + [DMAController(self) for i \
            in range(len(dma_ports))]


        # Create one sequencer per CPU and dma controller.
        # Sequencers for other controllers can be here here.
        self.sequencers = [RubySequencer(version = i,
                                # I/D cache is combined and grab from ctrl
                                icache = self.controllers[i].L1Icache,
                                dcache = self.controllers[i].L1Dcache,
                                clk_domain = self.controllers[i].clk_domain,
                                # TODO: This board access is dangerious. How
                                # should we access this?
                                pio_request_port = board.iobus.cpu_side_ports,
                                mem_request_port = board.iobus.cpu_side_ports,
                                pio_response_port = board.iobus.mem_side_ports
                                ) for i in range(len(board.get_processor()\
                                    .get_num_cores()))] + \
                          [DMASequencer(version = i,
                                        in_ports = port)
                            for i,port in enumerate(dma_ports)
                          ]

        for i,c in enumerate(self.controllers[:board.get_processor()\
            .get_num_cores()]):
            c.sequencer = self.sequencers[i]

        #Connecting the DMA sequencer to DMA controller
        for i,d in enumerate(self.controllers[-len(dma_ports):]):
            i += board.get_processor().get_num_cores()
            d.dma_sequencer = self.sequencers[i]

        self.num_of_sequencers = len(self.sequencers)

        # Create the network and connect the controllers.
        # NOTE: This is quite different if using Garnet!
        self.network.connectControllers(self.controllers)
        self.network.setup_buffers()

        # Set up a proxy port for the system_port. Used for load binaries and
        # other functional-only things.
        self.sys_port_proxy = RubyPortProxy()
        board.system_port = self.sys_port_proxy.in_ports
        self.sys_port_proxy.pio_request_port = board.iobus.cpu_side_ports

        # Connect the cpu's cache, interrupt, and TLB ports to Ruby
        for i,cpu in enumerate(board.get_processor().get_cpu_simobjects()):

            cpu.icache_port = self.sequencers[i].in_ports
            cpu.dcache_port = self.sequencers[i].in_ports

            isa = board.get_runtime_isa()
            if isa == ISA.X86:
                cpu.interrupts[0].pio = self.sequencers[i].interrupt_out_port
                cpu.interrupts[0].int_requestor = self.sequencers[i].in_ports
                cpu.interrupts[0].int_responder = \
                                        self.sequencers[i].interrupt_out_port

            if isa == ISA.X86 or isa == ISA.ARM:
                cpu.itb.walker.port = self.sequencers[i].in_ports
                cpu.dtb.walker.port = self.sequencers[i].in_ports

    @overrides(AbstractCacheHierarchy)
    def get_interrupt_ports(self, cpu: BaseCPU) -> Tuple[Port, Port]:
        req_port = self._cpu_map[cpu].sequencer.interrupt_out_port
        resp_port = self._cpu_map[cpu].sequencer.in_ports
        return req_port, resp_port


class L1Cache(L1Cache_Controller):

    _version = 0
    @classmethod
    def versionCount(cls):
        cls._version += 1 # Use count for this particular type
        return cls._version - 1

    def __init__(self, system, ruby_system, cpu, num_l2Caches):
        """Creating L1 cache controller. Consist of both instruction
           and data cache. The size of data cache is 512KB and
           8-way set associative. The instruction cache is 32KB,
           2-way set associative.
        """
        super(L1Cache, self).__init__()

        self.version = self.versionCount()
        block_size_bits = int(math.log(system.cache_line_size, 2))
        l1i_size = '32kB'
        l1i_assoc = '2'
        l1d_size = '512kB'
        l1d_assoc = '8'
        # This is the cache memory object that stores the cache data and tags
        self.L1Icache = RubyCache(size = l1i_size,
                                assoc = l1i_assoc,
                                start_index_bit = block_size_bits ,
                                is_icache = True)

        self.L1Dcache = RubyCache(size = l1d_size,
                            assoc = l1d_assoc,
                            start_index_bit = block_size_bits,
                            is_icache = False)

        self.l2_select_num_bits = int(math.log(num_l2Caches , 2))
        self.clk_domain = cpu.clk_domain
        self.prefetcher = RubyPrefetcher()
        self.send_evictions = self.sendEvicts(cpu)
        self.transitions_per_cycle = 4
        self.enable_prefetch = False
        self.ruby_system = ruby_system
        self.connectQueues(ruby_system)

    def getBlockSizeBits(self, system):
        bits = int(math.log(system.cache_line_size, 2))
        if 2**bits != system.cache_line_size.value:
            panic("Cache line size not a power of 2!")
        return bits

    def sendEvicts(self, cpu):
        """True if the CPU model or ISA requires sending evictions from caches
           to the CPU. Two scenarios warrant forwarding evictions to the CPU:
           1. The O3 model must keep the LSQ coherent with the caches
           2. The x86 mwait instruction is built on top of coherence
           3. The local exclusive monitor in ARM systems
        """
        if type(cpu) is DerivO3CPU or \
           buildEnv['TARGET_ISA'] in ('x86', 'arm'):
            return True
        return False

    def connectQueues(self, ruby_system):
        """Connect all of the queues for this controller.
        """
        self.mandatoryQueue = MessageBuffer()
        self.requestFromL1Cache = MessageBuffer()
        self.requestFromL1Cache.out_port = ruby_system.network.in_port
        self.responseFromL1Cache = MessageBuffer()
        self.responseFromL1Cache.out_port = ruby_system.network.in_port
        self.unblockFromL1Cache = MessageBuffer()
        self.unblockFromL1Cache.out_port = ruby_system.network.in_port
        self.optionalQueue = MessageBuffer()
        self.requestToL1Cache = MessageBuffer()
        self.requestToL1Cache.in_port = ruby_system.network.out_port
        self.responseToL1Cache = MessageBuffer()
        self.responseToL1Cache.in_port = ruby_system.network.out_port