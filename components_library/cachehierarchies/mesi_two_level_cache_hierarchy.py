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

from mem.ruby.system.Sequencer import DMASequencer, RubyPortProxy
from .abstract_cache_hierarchy import AbstractTwoLevelHierarchy
from .abstract_ruby_cache_hierarhcy import AbstractRubyCacheHierarchy
from ..motherboards.abstract_motherboard import AbstractMotherboard

from .ruby.topologies.pt2pt import SimplePt2Pt

from .ruby.MESI_Two_Level import L1Cache, L2Cache, Directory, DMAController

from m5.objects import RubySystem, RubySequencer, BaseCPU

from m5.params import Port

from typing import Tuple


class MESITwoLevelCacheHierarchy(
    AbstractRubyCacheHierarchy, AbstractTwoLevelHierarchy
):
    """A two level private L1 shared L2 MESI hierarchy.

    In addition to the normal two level parameters, you can also change the
    number of L2 banks in this protocol.

    The on-chip network is a point-to-point all-to-all simple network.
    """

    def __init__(
        self,
        l1i_size: str,
        l1i_assoc: str,
        l1d_size: str,
        l1d_assoc: str,
        l2_size: str,
        l2_assoc: str,
        num_l2_banks: int,
    ):
        super(AbstractTwoLevelHierarchy).__init__(
            l1i_size, l1i_assoc, l1d_size, l1d_assoc, l2_size, l2_assoc
        )
        self.num_l2_banks = num_l2_banks

    def incorporate_cache(self, motherboard: AbstractMotherboard) -> None:
        cache_line_size = motherboard.get_system_simobject().cache_line_size

        self.ruby_system = RubySystem()

        self.network = SimplePt2Pt(self.ruby_system)

        # QUESTION: HOW IS THIS KNOWN???
        iobus = motherboard.get_iobus()

        # MESI_Two_Level needs 5 virtual networks
        self.ruby_system.number_of_virtual_networks = 5

        self.l1_controllers = []
        self.cpu_map = {}
        for cpu in motherboard.get_processor().get_cpu_simobjects():
            cache = L1Cache(
                self.l1i_size,
                self.l1i_assoc,
                self.l1d_size,
                self.l1d_assoc,
                self.network,
                cpu,
                self.num_l2_banks,
                cache_line_size,
            )
            cache.sequencer = RubySequencer(
                version=i,
                dcache=ctrl.L1Dcache,
                clk_domain=ctrl.clock_domain,
                pio_request_port=iobus.cpu_side_ports,
                mem_request_port=iobus.cpu_side_ports,
                pio_response_port=iobus.mem_side_ports,
            )

            cpu.icache_port = cache.sequencer.in_ports
            cpu.dcache_port = cache.sequencer.in_ports

            cpu.mmu.connectWalkerPorts(
                cache.sequencer.in_ports, cache.sequencer.in_ports
            )

            self.l1_controllers.append(cache)
            self.cpu_map[cpu] = cache

        self.l2_controllers = [
            L2Cache(
                self.l2_size,
                self.l2_assoc,
                self.network,
                self.num_l2_banks,
                cache_line_size,
            )
            for _ in range(self.num_l2_banks)
        ]

        self.directory_controllers = [
            Directory(self.network, cache_line_size, range, port)
            for range, port in motherboard.get_memory().get_mem_ports()
        ]

        dma_ports = motherboard.get_dma_ports()
        self.dma_controllers = []
        for i, port in enumerate(dma_ports):
            ctrl = DMAController(self.network, cache_line_size)
            ctrl.sequencer = DMASequencer(version=i, in_ports=port)
            self.dma_controllers.append(ctrl)

        self.ruby_system.num_of_sequencers = len(self.l1_controllers) + len(
            self.dma_controllers
        )

        # Create the network and connect the controllers.
        # NOTE: This is quite different if using Garnet!
        self.network.connectControllers(
            self.l1_controllers
            + self.l2_controllers
            + self.directory_controllers
            + self.dma_controllers
        )
        self.network.setup_buffers()

        # Set up a proxy port for the system_port. Used for load binaries and
        # other functional-only things.
        self.ruby_system.sys_port_proxy = RubyPortProxy(
            pio_request_port=iobus.cpu_side_ports
        )
        motherboard.get_system_simobject().system_port = (
            self.ruby_system.sys_port_proxy.in_ports
        )

    def get_interrupt_ports(self, cpu: BaseCPU) -> Tuple[Port, Port]:
        req_port = self.cpu_map[cpu].sequencer.interrupt_out_port
        resp_port = self.cpu_map[cpu].sequencer.in_ports
        return req_port, resp_port
