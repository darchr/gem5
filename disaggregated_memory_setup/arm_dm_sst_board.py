# Copyright (c) 2023 The Regents of the University of California
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
    AddrRange,
    VoltageDomain,
    SrcClockDomain,
    Terminal,
    VncServer,
    IOXBar,
    BadAddr,
    ArmSystem,
)

from m5.objects.RealView import VExpress_GEM5_Base, VExpress_GEM5_Foundation
from m5.objects.ArmSystem import ArmRelease, ArmDefaultRelease
from m5.objects.ArmFsWorkload import ArmFsLinux

from m5.util.fdthelper import (
    Fdt,
    FdtNode,
    FdtProperty,
    FdtPropertyStrings,
    FdtPropertyWords,
    FdtState,
)

import os
import m5
from abc import ABCMeta
from gem5.components.boards.arm_board import ArmBoard
from gem5.components.processors.abstract_processor import AbstractProcessor
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.utils.override import overrides

from typing import List, Sequence, Tuple


class ArmDMSSTBoard(ArmBoard):
    __metaclass__ = ABCMeta

    def __init__(
        self,
        clk_freq: str,
        processor: AbstractProcessor,
        memory: AbstractMemorySystem,
        cache_hierarchy: AbstractCacheHierarchy,
        remote_memory_range: AddrRange,
        platform: VExpress_GEM5_Base = VExpress_GEM5_Foundation(),
        release: ArmRelease = ArmDefaultRelease(),
    ) -> None:

        self._remote_memory_range = remote_memory_range
        super().__init__(
            clk_freq=clk_freq,
            processor=processor,
            memory=memory,
            cache_hierarchy=cache_hierarchy,
            platform=platform,
            release=release,
        )

    def get_remote_memory_addr_range(self):
        return self._remote_memory_range

    @overrides(ArmBoard)
    def _setup_board(self) -> None:

        # This board is expected to run full-system simulation.
        # Loading ArmFsLinux() from `src/arch/arm/ArmFsWorkload.py`
        self.workload = ArmFsLinux()

        # We are fixing the following variable for the ArmSystem to work. The
        # security extension is checked while generating the dtb file in
        # realview. This board does not have security extension enabled.
        self._have_psci = False

        # highest_el_is_64 is set to True. True if the register width of the
        # highest implemented exception level is 64 bits.
        self.highest_el_is_64 = True

        # Setting up the voltage and the clock domain here for the ARM board.
        # The ArmSystem/RealView expects voltage_domain to be a parameter.
        # The voltage and the clock frequency are taken from the devices.py
        # file from configs/example/arm. We set the clock to the same frequency
        # as the user specified in the config script.
        self.voltage_domain = VoltageDomain(voltage="1.0V")
        self.clk_domain = SrcClockDomain(
            clock=self._clk_freq, voltage_domain=self.voltage_domain
        )

        # The ARM board supports both Terminal and VncServer.
        self.terminal = Terminal()
        self.vncserver = VncServer()

        # Incoherent I/O Bus
        self.iobus = IOXBar()
        self.iobus.badaddr_responder = BadAddr()
        self.iobus.default = self.iobus.badaddr_responder.pio

        # We now need to setup the dma_ports.
        self._dma_ports = None

        # RealView sets up most of the on-chip and off-chip devices and GIC
        # for the ARM board. These devices' information is also used to
        # generate the dtb file. We then connect the I/O devices to the
        # I/O bus.
        self._setup_io_devices()

        # Once the realview is setup, we can continue setting up the memory
        # ranges. ArmBoard's memory can only be setup once realview is
        # initialized.
        memory = self.get_memory()
        mem_size = memory.get_size()

        # The following code is taken from configs/example/arm/devices.py. It
        # sets up all the memory ranges for the board.
        self.mem_ranges = []
        success = False
        # self.mem_ranges.append(self.get_remote_memory_addr_range())
        for mem_range in self.realview._mem_regions:
            size_in_range = min(mem_size, mem_range.size())
            self.mem_ranges.append(
                AddrRange(start=mem_range.start, size=size_in_range)
            )

            mem_size -= size_in_range
            if mem_size == 0:
                success = True
                break

        if success:
            memory.set_memory_range(self.mem_ranges)
        else:
            raise ValueError("Memory size too big for platform capabilities")

        self.mem_ranges.append(self.get_remote_memory_addr_range())

        # The PCI Devices. PCI devices can be added via the `_add_pci_device`
        # function.
        self._pci_devices = []

    @overrides(ArmSystem)
    def generateDeviceTree(self, state):
        # Generate a device tree root node for the system by creating the root
        # node and adding the generated subnodes of all children.
        # When a child needs to add multiple nodes, this is done by also
        # creating a node called '/' which will then be merged with the
        # root instead of appended.

        def generateMemNode(numa_node_id, mem_range):
            node = FdtNode(f"memory@{int(mem_range.start):x}")
            node.append(FdtPropertyStrings("device_type", ["memory"]))
            node.append(
                FdtPropertyWords(
                    "reg",
                    state.addrCells(mem_range.start)
                    + state.sizeCells(mem_range.size()),
                )
            )
            node.append(FdtPropertyWords("numa-node-id", [numa_node_id]))
            return node

        root = FdtNode("/")
        root.append(state.addrCellsProperty())
        root.append(state.sizeCellsProperty())

        # Add memory nodes
        for mem_range in self.mem_ranges:
            root.append(generateMemNode(0, mem_range))
        root.append(generateMemNode(1, self.get_remote_memory_addr_range()))

        for node in self.recurseDeviceTree(state):
            # Merge root nodes instead of adding them (for children
            # that need to add multiple root level nodes)
            if node.get_name() == root.get_name():
                root.merge(node)
            else:
                root.append(node)

        return root

    @overrides(ArmBoard)
    def get_default_kernel_args(self) -> List[str]:

        # The default kernel string is taken from the devices.py file.
        return [
            "console=ttyAMA0",
            "lpj=19988480",
            "norandmaps",
            "root={root_value}",
            "rw",
            f"mem={self.get_memory().get_size()}",
        ]
