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

import enum
from sys import version

from .abstract_cache_hierarchy import AbstractCacheHierarchy
from .abstract_two_level_cache_hierarchy import AbstractTwoLevelCacheHierarchy
from .abstract_ruby_cache_hierarhcy import AbstractRubyCacheHierarchy
from ..motherboards.abstract_motherboard import AbstractMotherboard

from .ruby.topologies.pt2pt import SimplePt2Pt

from .ruby.MESI_Two_Level import L1Cache, L2Cache, Directory, DMAController

from ..utils.override import *

from m5.objects import (
    RubySystem,
    RubySequencer,
    BaseCPU,
    DMASequencer,
    RubyPortProxy,
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
    def incorporate_cache(self, motherboard: AbstractMotherboard) -> None:
        cache_line_size = motherboard.get_system_simobject().cache_line_size

        motherboard.get_system_simobject().ruby_system = RubySystem()
        self._ruby_system = motherboard.get_system_simobject().ruby_system

        # MESI_Two_Level needs 5 virtual networks
        self._ruby_system.number_of_virtual_networks = 5

        self._ruby_system.network = SimplePt2Pt(self._ruby_system)
        self._ruby_system.network.number_of_virtual_networks = 5

        # QUESTION: HOW IS THIS KNOWN???
        iobus = motherboard.get_iobus()

        self._l1_controllers = []
        self._cpu_map = {}
        for i, cpu in enumerate(
            motherboard.get_processor().get_cpu_simobjects()
        ):
            cache = L1Cache(
                self.get_l1d_size(),
                self.get_l1i_assoc(),
                self.get_l1d_size(),
                self.get_l1d_assoc(),
                self._ruby_system.network,
                cpu,
                self._num_l2_banks,
                cache_line_size,
            )
            cache.sequencer = RubySequencer(
                version=i,
                dcache=cache.L1Dcache,
                clk_domain=cache.clk_domain,
                pio_request_port=iobus.cpu_side_ports,
                mem_request_port=iobus.cpu_side_ports,
                pio_response_port=iobus.mem_side_ports,
            )
            cache.ruby_system = self._ruby_system

            cpu.icache_port = cache.sequencer.in_ports
            cpu.dcache_port = cache.sequencer.in_ports

            cpu.mmu.connectWalkerPorts(
                cache.sequencer.in_ports, cache.sequencer.in_ports
            )

            self._l1_controllers.append(cache)
            self._cpu_map[cpu] = cache

        self.l2_controllers = [
            L2Cache(
                self.get_l2_size(),
                self.get_l2_assoc(),
                self._ruby_system.network,
                self._num_l2_banks,
                cache_line_size,
            )
            for _ in range(self._num_l2_banks)
        ]
        # TODO: Make this prettier: The problem is not being able to proxy
        # the ruby system correctly
        for cache in self.l2_controllers:
            cache.ruby_system = self._ruby_system

        self.directory_controllers = [
            Directory(self._ruby_system.network, cache_line_size, range, port)
            for range, port in motherboard.get_memory().get_mem_ports()
        ]
        # TODO: Make this prettier: The problem is not being able to proxy
        # the ruby system correctly
        for dir in self.directory_controllers:
            dir.ruby_system = self._ruby_system

        dma_ports = motherboard.get_dma_ports()
        self.dma_controllers = []
        for i, port in enumerate(dma_ports):
            ctrl = DMAController(self._ruby_system.network, cache_line_size)
            ctrl.dma_sequencer = DMASequencer(version=i, in_ports=port)
            self.dma_controllers.append(ctrl)
            ctrl._ruby_system = self.ruby_system

        self._ruby_system.num_of_sequencers = len(self.l1_controllers) + len(
            self.dma_controllers
        )
        self._ruby_system.l1_controllers = self.l1_controllers
        self._ruby_system.l2_controllers = self.l2_controllers
        self._ruby_system.directory_controllers = self.directory_controllers
        self._ruby_system.dma_controllers = self.dma_controllers

        # Create the network and connect the controllers.
        self._ruby_system.network.connectControllers(
            self.l1_controllers
            + self.l2_controllers
            + self.directory_controllers
            + self.dma_controllers
        )
        self._ruby_system.network.setup_buffers()

        # Set up a proxy port for the system_port. Used for load binaries and
        # other functional-only things.
        self._ruby_system.sys_port_proxy = RubyPortProxy()
        motherboard.get_system_simobject().system_port = (
            self._ruby_system.sys_port_proxy.in_ports
        )
        self._ruby_system.sys_port_proxy.pio_request_port = \
            iobus.cpu_side_ports

    @overrides(AbstractCacheHierarchy)
    def get_interrupt_ports(self, cpu: BaseCPU) -> Tuple[Port, Port]:
        req_port = self._cpu_map[cpu].sequencer.interrupt_out_port
        resp_port = self._cpu_map[cpu].sequencer.in_ports
        return req_port, resp_port
