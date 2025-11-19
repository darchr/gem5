
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

# from memories.external_remote_memory import ExternalRemoteMemory
from boards.x86_main_board import X86ComposableMemoryBoard

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
    X86ACPIMadt,
    NoncoherentXBar,
    X86IntelMPBus,
    X86IntelMPBusHierarchy,
    X86IntelMPIOAPIC,
    X86IntelMPIOIntAssignment,
    X86ACPIMadtIntSourceOverride,
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

class X86SharedMemoryBoard(X86ComposableMemoryBoard):
    """
    A high-level X86 board that allows users to simulate reserved memory
    ranges to enable memory sharing.
    """
    def __init__(
        self,
        clk_freq: str,
        processor: AbstractProcessor,
        local_memory: AbstractMemorySystem,
        remote_memory: AbstractMemorySystem,
        cache_hierarchy: AbstractCacheHierarchy,
        remote_memory_access_cycles: int = 0,
        remote_memory_address_range: AddrRange = None,
        starting_memory_limit: str = None,
        
    ) -> None:
        # call the composable memory board that start the simulation.
        super().__init__(
            clk_freq=clk_freq,
            processor=processor,
            local_memory=local_memory,
            remote_memory=remote_memory,
            cache_hierarchy=cache_hierarchy,
            remote_memory_access_cycles=remote_memory_access_cycles,
            remote_memory_address_range=remote_memory_address_range,
            starting_memory_limit=starting_memory_limit
        )

    @overrides(X86ComposableMemoryBoard)
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
        madt_entries = []
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
        pci_dev4_inta_madt = X86ACPIMadtIntSourceOverride(
            bus_source=pci_dev4_inta.source_bus_id,
            irq_source=pci_dev4_inta.source_bus_irq,
            sys_int=pci_dev4_inta.dest_io_apic_intin,
            flags=0,
        )
        madt_entries.append(pci_dev4_inta_madt)

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
            # acpi
            assign_to_apic_acpi = X86ACPIMadtIntSourceOverride(
                bus_source=1, irq_source=irq, sys_int=apicPin, flags=0
            )
            madt_entries.append(assign_to_apic_acpi)

        assignISAInt(0, 2)
        assignISAInt(1, 1)

        for i in range(3, 15):
            assignISAInt(i, i)

        self.workload.intel_mp_table.base_entries = base_entries
        self.workload.intel_mp_table.ext_entries = ext_entries

        madt = X86ACPIMadt(
            local_apic_address=0, records=madt_entries, oem_id="madt"
        )
        self.workload.acpi_description_table_pointer.rsdt.entries.append(madt)
        self.workload.acpi_description_table_pointer.xsdt.entries.append(madt)
        self.workload.acpi_description_table_pointer.oem_id = "gem5"
        self.workload.acpi_description_table_pointer.rsdt.oem_id = "gem5"
        self.workload.acpi_description_table_pointer.xsdt.oem_id = "gem5"
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
            # The second range of memory will be shared and the type of this
            # range has to be 12 - `persistent`
            X86E820Entry(
                addr=0x100000000,
                size=f"{self.mem_ranges[1].size()}B",
                range_type=12,
            ),
        ]

        # Reserve the last 16kB of the 32-bit address space for m5ops
        entries.append(
            X86E820Entry(addr=0xFFFF0000, size="64kB", range_type=2)
        )
        self.workload.e820_table.entries = entries        


    @overrides(X86ComposableMemoryBoard)
    def get_default_kernel_args(self) -> List[str]:
        return [
            "no_systemd=true",
            "earlyprintk=ttyS0",
            "console=ttyS0",
            "lpj=7999923",
            "root=/dev/sda2",
            # "mtrr=debug"
            "no_systemd=true",
            # "init=/bin/bash"
        ]