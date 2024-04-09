# Copyright (c) 2023-24 The Regents of the University of California
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

# The goal of this board is to combine the gem5-only and the gem5-SSt boards
# into one single board.
import os
import sys
from abc import ABCMeta
from typing import (
    List,
    Sequence,
    Tuple,
)

# all the source files are one directory above.
sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
)

from memories.external_remote_memory_v2 import ExternalRemoteMemoryV2

import m5
from m5.objects import (
    AddrRange,
    ArmSystem,
    BadAddr,
    ExternalMemory,
    IOXBar,
    NoncoherentXBar,
    Port,
    SrcClockDomain,
    Terminal,
    VncServer,
    VoltageDomain,
)
from m5.objects.ArmFsWorkload import ArmFsLinux
from m5.objects.ArmSystem import (
    ArmDefaultRelease,
    ArmRelease,
)
from m5.objects.RealView import (
    VExpress_GEM5_Base,
    VExpress_GEM5_Foundation,
)
from m5.util.fdthelper import (
    Fdt,
    FdtNode,
    FdtProperty,
    FdtPropertyStrings,
    FdtPropertyWords,
    FdtState,
)

from gem5.components.boards.arm_board import ArmBoard
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.processors.abstract_processor import AbstractProcessor
from gem5.utils.override import overrides


class ArmComposableMemoryBoard(ArmBoard):
    """
    A high-level ARM board that can zNUMA-capable systems with a remote
    memories. This board is extended from the ArmBoard from Gem5 standard
    library. This board assumes that you will be booting Linux. This board can
    be used to do disaggregated ARM system research while accelerating the
    simulation using kvm.

    The revised ArmComposableMemoryBoard combines the older boards into one
    single board to make the boards compatible with both gem5 and SST.

    **Limitations**
    * kvm is only supported in a gem5-only setup.

    @params
    TODO
    """

    __metaclass__ = ABCMeta

    def __init__(
        self,
        clk_freq: str,
        processor: AbstractProcessor,
        local_memory: AbstractMemorySystem,
        remote_memory: AbstractMemorySystem,
        cache_hierarchy: AbstractCacheHierarchy,
        platform: VExpress_GEM5_Base = VExpress_GEM5_Foundation(),
        release: ArmRelease = ArmDefaultRelease(),
        remote_memory_access_cycles: int = 750,
        remote_memory_address_range: AddrRange = None,
    ) -> None:
        # The parent board calls get_memory(), which needs overriding.
        self._localMemory = local_memory
        self._remoteMemory = remote_memory
        # We need to set the remote memory range before init for the remote
        # memory. If the user did not specify the remote_memory_addr_range,
        # then we'd assume that the remote memory starts where local memory
        # ends.
        # If the user gave a remote memory address range, then set it directly.
        # TODO: This makes the design confusing. Remove this in the future
        # iteration. A remote memory range should only be supplied when
        # initializing the memory.
        self._remoteMemoryAddressRange = None
        if remote_memory_address_range is not None:
            self._remoteMemoryAddressRange = remote_memory_address_range
        else:
            # Is this an external remote memory?
            if isinstance(remote_memory, ExternalRemoteMemoryV2) == True:
                # There is an address range specified when the remote memory
                # was initialized.
                if self._remoteMemory.get_set_using_addr_ranges() == True:
                    # Set the board's memory range as whatever was used.
                    self._remoteMemoryAddressRange = (
                        self._remoteMemory.get_mem_ports()[0][0]
                    )
        # In case that none of the above set the memory range, we'll set it
        # manually
        if self._remoteMemoryAddressRange is None:
            # If the remote_memory_addr_range is not provided, we'll
            # assume that it starts at 0x80000000 + local_memory_size
            # and ends at it's own size.
            self._remoteMemoryAddressRange = AddrRange(
                0x80000000 + self._localMemory.get_size(),
                size=self._remoteMemory.get_size(),
            )
        assert self._remoteMemoryAddressRange is not None

        super().__init__(
            clk_freq=clk_freq,
            processor=processor,
            memory=local_memory,
            cache_hierarchy=cache_hierarchy,
            platform=platform,
            release=release,
        )

        self.local_memory = local_memory
        self.remote_memory = remote_memory

        # The amount of latency to access the remote memory has to be either
        # implemented using a non-coherent crossbar that connects the the
        # remote memory to the rest of the system or passed as a link latency
        # to SST.
        self._remote_memory_access_cycles = remote_memory_access_cycles

        # Set the external simulator variable to whatever the user has set in
        # the ExternalRemoteMemory component.
        self._external_simulator = False
        if isinstance(self.get_remote_memory(), ExternalMemory):
            # TODO: This needs to be standardized.
            self._external_simulator = (
                self.get_remote_memory()._remote_request_bridge.use_sst_sim
            )
            # Check if the user is trying to simulate additional latency with
            # the remote outgoing bridge
            if self._remote_memory_access_cycles > 0:
                warn(
                    "Trying to simulate remote memory with a gem5-side \
                        latency. We recommed adding this latency to the \
                        SST-side script"
                )

    @overrides(ArmBoard)
    def get_memory(self) -> "AbstractMemorySystem":
        """Get the memory (RAM) connected to the board.

        :returns: The memory system.
        """
        raise NotImplementedError

    def get_local_memory(self) -> "AbstractMemorySystem":
        """Get the memory (RAM) connected to the board.
        :returns: The local memory system.
        """
        # get local memory is called at init phase.
        return self._localMemory

    def get_remote_memory(self) -> "AbstractMemorySystem":
        """Get the memory (RAM) connected to the board.
            This has to be implemeted by the child class as we don't know if
            this board is simulating Gem5 memory or some external simulator
            memory.
        :returns: The remote memory system.
        """
        return self._remoteMemory

    def get_remote_memory_size(self) -> "str":
        """Get the remote memory size to setup the NUMA nodes. Since the remote
            memory is an abstract memory system, we should be able to call its
            standard methods.
        :returns: The size of the remote memory system.
        """
        return self.get_remory_memory().get_size()

    @overrides(ArmBoard)
    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return self.get_local_memory().get_mem_ports()

    def get_remote_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        """Get the memory (RAM) ports connected to the board.
            This has to be implemeted by the child class as we don't know if
            this board is simulating Gem5 memory or some external simulator
            memory.
        :returns: A tuple of mem_ports.
        """
        return self.get_remote_memory().get_mem_ports()

    def get_remote_memory_addr_range(self):
        """Get the range of the remote memory. This can be omitted in the
            future iteration of the board.
        :returns: AddrRange of the remote memory
        """
        # Although this is hardcoded to return the first element, this is
        # always valid. This is how the standard library returns
        # get_mem_ports().
        if self._remoteMemoryAddressRange is None:
            return self.get_remote_mem_ports()[0][0]
        else:
            return self._remoteMemoryAddressRange

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
        local_memory = self.get_local_memory()
        print(type(local_memory))
        mem_size = local_memory.get_size()

        # The following code is taken from configs/example/arm/devices.py. It
        # sets up all the memory ranges for the board.
        self.mem_ranges = []
        success = False
        # self.mem_ranges.append(self.get_remote_memory_addr_range())
        for mem_range in self.realview._mem_regions:
            print(mem_size)
            size_in_range = min(mem_size, mem_range.size())
            self.mem_ranges.append(
                AddrRange(start=mem_range.start, size=size_in_range)
            )
            mem_size -= size_in_range

            if mem_size == 0:
                success = True
                break

        if success:
            local_memory.set_memory_range(self.mem_ranges)
        else:
            raise ValueError("Memory size too big for platform capabilities")
        # At the end of the local_memory, append the remote memory range.
        self._set_remote_memory_ranges()
        self.mem_ranges.append(self.get_remote_memory_addr_range())

        # The PCI Devices. PCI devices can be added via the `_add_pci_device`
        # function.
        self._pci_devices = []

    def _set_remote_memory_ranges(self):
        self.get_remote_memory().set_memory_range(
            [self.get_remote_memory_addr_range()]
        )

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

    def add_remote_link(self) -> None:
        """This method creates a non-coherent xbar"""
        self.remote_link = NoncoherentXBar(
            frontend_latency=self._remote_memory_access_cycles,
            forward_latency=0,
            response_latency=0,
            width=64,
        )
        # Connect the remote memory port to the remote link.
        for _, port in self.get_remote_memory().get_mem_ports():
            self.remote_link.mem_side_ports = port

        # Connect the cpu side ports to the cache
        self.remote_link.cpu_side_ports = (
            self.get_cache_hierarchy().get_mem_side_port()
        )

    @overrides(ArmBoard)
    def get_default_kernel_args(self) -> List[str]:
        # The default kernel string is taken from the devices.py file.
        return [
            "console=ttyAMA0",
            "lpj=19988480",
            "norandmaps",
            "root={root_value}",
            "rw",
        ]

    @overrides(ArmBoard)
    def _connect_things(self) -> None:
        """Connects all the components to the board.

        The order of this board is always:

        1. Connect the memory.
        2. Connect the cache hierarchy.
        3. Connect the processor.

        Developers may build upon this assumption when creating components.

        Notes
        -----

        * The processor is incorporated after the cache hierarchy due to a bug
        noted here: https://gem5.atlassian.net/browse/GEM5-1113. Until this
        bug is fixed, this ordering must be maintained.
        * Once this function is called `_connect_things_called` *must* be set
        to `True`.
        """

        if self._connect_things_called:
            raise Exception(
                "The `_connect_things` function has already been called."
            )

        # Incorporate the memory into the motherboard.
        self.get_local_memory().incorporate_memory(self)
        self.get_remote_memory().incorporate_memory(self)

        # Incorporate the cache hierarchy for the motherboard.
        if self.get_cache_hierarchy():
            self.get_cache_hierarchy().incorporate_cache(self)
            # need to connect the remote links to the board.
            if self.get_cache_hierarchy().is_ruby():
                print(
                    "remote memory is only supported in classic caches at "
                    + "the moment!"
                )
            else:
                # Create and connect Xbar for additional latency. This will
                # override the cache's incorporate_cache
                if (
                    self._remote_memory_access_cycles > 0
                    and self._external_simulator == False
                ):
                    self.add_remote_link()
                else:
                    # connect the system to the remote memory directly.
                    for (
                        cntr
                    ) in self.get_remote_memory().get_memory_controllers():
                        cntr.port = (
                            self.get_cache_hierarchy().get_mem_side_port()
                        )

        # Incorporate the processor into the motherboard.
        self.get_processor().incorporate_processor(self)

        self._connect_things_called = True

    @overrides(ArmBoard)
    def _post_instantiate(self):
        """Called to set up anything needed after m5.instantiate. The memory
        has been replaced with local and remote memories in this board."""
        self.get_processor()._post_instantiate()
        if self.get_cache_hierarchy():
            self.get_cache_hierarchy()._post_instantiate()
        self.get_local_memory()._post_instantiate()
        self.get_remote_memory()._post_instantiate()
