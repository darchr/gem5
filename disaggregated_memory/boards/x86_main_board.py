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

# Creating an x86 board that can simulate more than 3 GB memory.

import os
from abc import ABCMeta
from typing import (
    List,
    Sequence,
    Tuple,
)

import m5
from m5.objects import (
    Addr,
    AddrRange,
    BadAddr,
    BaseXBar,
    Bridge,
    CowDiskImage,
    IdeDisk,
    IOXBar,
    OutgoingRequestBridge,
    Pc,
    Port,
    RawDiskImage,
    SrcClockDomain,
    Terminal,
    VncServer,
    VoltageDomain,
    X86E820Entry,
    NoncoherentXBar,
    X86IntelMPBus,
    X86IntelMPBusHierarchy,
    X86IntelMPIOAPIC,
    X86IntelMPIOIntAssignment,
    X86IntelMPProcessor,
    X86SMBiosBiosInformation,
)

from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.boards.x86_board import X86Board
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.processors.abstract_processor import AbstractProcessor
from gem5.utils.override import overrides


class X86ComposableMemoryBoard(X86Board):
    """
    A high-level X86 board that can zNUMA-capable systems with a remote
    memories. This board is extended from the ArmBoard from Gem5 standard
    library. This board assumes that you will be booting Linux. This board can
    be used to do disaggregated ARM system research while accelerating the
    simulation using kvm.

    The revised X86ComposableMemoryBoard combines the older boards into one
    single board to make the boards compatible with both gem5 and SST.
    
    Targets:
        - This board should support memory hotplugging via PROBE
        - We also need to get ACPI SRAT tables set up for the NUMA ranges.

    Limitations:
        - Local memory cannot be more than 3 GB (lazy to make this work).
        - NUMA nodes are faked via the kernel as gem5 X86 does not support
          ACPI SRAT tables.
          
    Args:
        :clk_freq:
        :processor:
        :local_memory:
        :remote_memory:
        :cache_hierarchy:
        :remote_memory_access_cycles:
        :remote_memory_address_range:
        :starting_memory_limit:

    Raises:
        NotImplementedError: _description_
        Exception: _description_

    """
    __metaclass__ = ABCMeta

    def __init__(
        self,
        clk_freq: str,
        processor: AbstractProcessor,
        local_memory: AbstractMemorySystem,
        remote_memory: AbstractMemorySystem,
        cache_hierarchy: AbstractCacheHierarchy,
        remote_memory_access_cycles: int = 0,
        remote_memory_address_range: AddrRange = None,
        starting_memory_limit: str = None
    ) -> None:
        # The parent board calls get_memory(), which needs overriding.
        self._localMemory = local_memory
        self._remoteMemory = remote_memory
        # We need to set the remote memory range before init for the remote
        # memory. If the user did not specify the remote_memory_addr_range,
        # then we'd assume that the remote memory starts where local memory
        # ends.
        if isinstance(remote_memory, OutgoingRequestBridge) == False:
            if remote_memory_address_range is None:
                # If the remote_memory_addr_range is not provided, we'll assume
                # that it starts at 0x100000000 + local_memory_size and ends at
                # it's own size
                self._remoteMemoryAddressRange = AddrRange(
                    0x100000000 + self._localMemory.get_size(),
                    size=self._remoteMemory.get_size(),
                )
            else:
                self._remoteMemoryAddressRange = remote_memory_address_range
        else:
            self._remoteMemoryAddressRange = None
        super().__init__(
            clk_freq=clk_freq,
            processor=processor,
            memory=local_memory,
            cache_hierarchy=cache_hierarchy,
        )

        self.local_memory = local_memory
        self.remote_memory = remote_memory
        
        self._remote_memory_access_cycles = remote_memory_access_cycles
        
        # Set the external simulator variable to whatever the user has set in
        # the ExternalRemoteMemory component.
        self._external_simulator = False
        if (isinstance(self.get_remote_memory(), OutgoingRequestBridge)):
            # TODO: This needs to be standardized.
            self._external_simulator = \
                    self.get_remote_memory()._remote_request_bridge.use_sst_sim

    @overrides(X86Board)
    def get_memory(self) -> AbstractMemorySystem:
        """Get the memory (RAM) connected to the board.

        :returns: The memory system.
        """
        raise NotImplementedError

    def get_local_memory(self) -> AbstractMemorySystem:
        """Get the memory (RAM) connected to the board.
        :returns: The local memory system.
        """
        return self._localMemory

    def get_remote_memory(self) -> AbstractMemorySystem:
        """Get the memory (RAM) connected to the board.
        :returns: The remote memory system.
        """
        return self._remoteMemory

    def get_remote_memory_size(self) -> "str":
        """Get the remote memory size to setup the NUMA nodes. Since the remote
            memory is an abstract memory system, we should be able to call its
            standard methods.
        :returns: The size of the remote memory system.
        """
        return self.get_remote_memory().get_size()

    @overrides(X86Board)
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

    @overrides(X86Board)
    def _setup_memory_ranges(self):
        # Need to create 2 entries for the memory ranges
        local_memory = self.get_local_memory()
        remote_memory = self.get_remote_memory()

        memory_size = [
            local_memory.get_size(),
            remote_memory.get_size()
        ]
        
        memory_ranges = [
            AddrRange(start=0x0, size=local_memory.get_size()),
            AddrRange(start=0x100000000, size=remote_memory.get_size())
        ]

        self.mem_ranges = [
            AddrRange(start=0x0, size=local_memory.get_size()),
            AddrRange(start=0x100000000, size=remote_memory.get_size()),
            AddrRange(0xC0000000, size=0x100000),  # For I/0
        ]
        
        local_memory.set_memory_range([AddrRange(start=0x0, size=local_memory.get_size())])
        remote_memory.set_memory_range([AddrRange(start=0x100000000, size=remote_memory.get_size())])

    @overrides(X86Board)
    def get_default_kernel_args(self) -> List[str]:
        return [
            "earlyprintk=ttyS0",
            "console=ttyS0",
            "lpj=7999923",
            "root=/dev/sda1",
            # "init=/bin/bash",
            "numa=fake=2"
        ]

    @overrides(X86Board)
    def _setup_io_devices(self):
        """Sets up the x86 IO devices.

        Note: This is mostly copy-paste from prior X86 FS setups. Some of it
        may not be documented and there may be bugs.
        """

        # Constants similar to x86_traits.hh
        IO_address_space_base = 0x8000000000000000
        pci_config_address_space_base = 0xC000000000000000
        interrupts_address_space_base = 0xA000000000000000
        APIC_range_size = 1 << 12

        # Setup memory system specific settings.
        if self.get_cache_hierarchy().is_ruby():
            self.pc.attachIO(self.get_io_bus(), [self.pc.south_bridge.ide.dma])
        else:
            self.bridge = Bridge(delay="50ns")
            self.bridge.mem_side_port = self.get_io_bus().cpu_side_ports
            try:
                self.bridge.cpu_side_port = (
                    self.get_cache_hierarchy().get_mem_side_port()
                )
            except:
                print("port not connected!")

            # # Constants similar to x86_traits.hh
            IO_address_space_base = 0x8000000000000000
            pci_config_address_space_base = 0xC000000000000000
            interrupts_address_space_base = 0xA000000000000000
            APIC_range_size = 1 << 12

            self.bridge.ranges = [
                AddrRange(0xC0000000, 0xFFFF0000),
                AddrRange(
                    IO_address_space_base, interrupts_address_space_base - 1
                ),
                AddrRange(pci_config_address_space_base, Addr.max),
            ]

            self.apicbridge = Bridge(delay="50ns")
            self.apicbridge.cpu_side_port = self.get_io_bus().mem_side_ports
            try:
                self.apicbridge.mem_side_port = (
                    self.get_cache_hierarchy().get_cpu_side_port()
                )
            except:
                print("port not connected")
            self.apicbridge.ranges = [
                AddrRange(
                    interrupts_address_space_base,
                    interrupts_address_space_base
                    + self.get_processor().get_num_cores() * APIC_range_size
                    - 1,
                )
            ]
            self.pc.attachIO(self.get_io_bus())

        # Add in a Bios information structure.
        self.workload.smbios_table.structures = [X86SMBiosBiosInformation()]

        # Set up the Intel MP table
        base_entries = []
        ext_entries = []
        for i in range(self.get_processor().get_num_cores()):
            bp = X86IntelMPProcessor(
                local_apic_id=i,
                local_apic_version=0x14,
                enable=True,
                bootstrap=(i == 0),
            )
            base_entries.append(bp)

        io_apic = X86IntelMPIOAPIC(
            id=self.get_processor().get_num_cores(),
            version=0x11,
            enable=True,
            address=0xFEC00000,
        )

        self.pc.south_bridge.io_apic.apic_id = io_apic.id
        base_entries.append(io_apic)
        pci_bus = X86IntelMPBus(bus_id=0, bus_type="PCI   ")
        base_entries.append(pci_bus)
        isa_bus = X86IntelMPBus(bus_id=1, bus_type="ISA   ")
        base_entries.append(isa_bus)
        connect_busses = X86IntelMPBusHierarchy(
            bus_id=1, subtractive_decode=True, parent_bus=0
        )
        ext_entries.append(connect_busses)

        pci_dev4_inta = X86IntelMPIOIntAssignment(
            interrupt_type="INT",
            polarity="ConformPolarity",
            trigger="ConformTrigger",
            source_bus_id=0,
            source_bus_irq=0 + (4 << 2),
            dest_io_apic_id=io_apic.id,
            dest_io_apic_intin=16,
        )

        base_entries.append(pci_dev4_inta)

        def assignISAInt(irq, apicPin):
            assign_8259_to_apic = X86IntelMPIOIntAssignment(
                interrupt_type="ExtInt",
                polarity="ConformPolarity",
                trigger="ConformTrigger",
                source_bus_id=1,
                source_bus_irq=irq,
                dest_io_apic_id=io_apic.id,
                dest_io_apic_intin=0,
            )
            base_entries.append(assign_8259_to_apic)

            assign_to_apic = X86IntelMPIOIntAssignment(
                interrupt_type="INT",
                polarity="ConformPolarity",
                trigger="ConformTrigger",
                source_bus_id=1,
                source_bus_irq=irq,
                dest_io_apic_id=io_apic.id,
                dest_io_apic_intin=apicPin,
            )
            base_entries.append(assign_to_apic)

        assignISAInt(0, 2)
        assignISAInt(1, 1)

        for i in range(3, 15):
            assignISAInt(i, i)

        self.workload.intel_mp_table.base_entries = base_entries
        self.workload.intel_mp_table.ext_entries = ext_entries

        entries = [
            # Mark the first megabyte of memory as reserved
            X86E820Entry(addr=0, size="639kB", range_type=1),
            X86E820Entry(addr=0x9FC00, size="385kB", range_type=2),
            # Mark the rest of physical memory as available
            # the local address comes first.
            X86E820Entry(
                addr=0x100000,
                size=f"{self.mem_ranges[0].size() - 0x100000:d}B",
                range_type=1,
            ),
            X86E820Entry(
                addr=0x100000000,
                size=f"{self.mem_ranges[1].size()}B",
                range_type=1,
            ),
        ]

        # Reserve the last 16kB of the 32-bit address space for m5ops
        entries.append(
            X86E820Entry(addr=0xFFFF0000, size="64kB", range_type=2)
        )

        print(entries)
        self.workload.e820_table.entries = entries

    def add_remote_link(self) -> None:
        """This method creates a non-coherent xbar"""
        self.remote_link = NoncoherentXBar(
                frontend_latency=self._remote_memory_access_cycles,
                forward_latency=0,
                response_latency=0,
                width=64
        )
        # Connect the remote memory port to the remote link.
        for _, port in self.get_remote_memory().get_mem_ports():
            self.remote_link.mem_side_ports = port

        # Connect the cpu side ports to the cache
        self.remote_link.cpu_side_ports = \
                self.get_cache_hierarchy().get_mem_side_port()


    @overrides(AbstractBoard)
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
            
        # Create and connect Xbar for additional latency. This will override
        # the cache's incorporate_cache
        if self._remote_memory_access_cycles > 0 and self._external_simulator == False:
            self.add_remote_link()
        else:
            # connect the system to the remote memory directly.
            for cntr in self.get_remote_memory().get_memory_controllers():
                cntr.port = self.get_cache_hierarchy().get_mem_side_port()
        # Incorporate the processor into the motherboard.
        self.get_processor().incorporate_processor(self)

        self._connect_things_called = True

    @overrides(AbstractBoard)
    def _post_instantiate(self):
        """Called to set up anything needed after m5.instantiate"""
        self.get_processor()._post_instantiate()
        if self.get_cache_hierarchy():
            self.get_cache_hierarchy()._post_instantiate()
        self.get_local_memory()._post_instantiate()
        self.get_remote_memory()._post_instantiate()
