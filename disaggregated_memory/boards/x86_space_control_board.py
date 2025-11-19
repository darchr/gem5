
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

from typing import List

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
    NoncoherentXBar,
    OutgoingRequestBridge,
    Pc,
    Port,
    RawDiskImage,
    SrcClockDomain,
    Terminal,
    VncServer,
    VoltageDomain,
    X86ACPIMadt,
    X86ACPIMadtIntSourceOverride,
    X86ACPIMadtIOAPIC,
    X86ACPIMadtLAPIC,
    X86E820Entry,
    X86FsLinux,
    X86IntelMPBus,
    X86IntelMPBusHierarchy,
    X86IntelMPIOAPIC,
    X86IntelMPIOIntAssignment,
    X86IntelMPProcessor,
    X86SMBiosBiosInformation,
)
from m5.util.convert import toMemorySize

from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.boards.x86_board import X86Board
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)


from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.memory.simple import SingleChannelSimpleMemory
from gem5.components.memory import SingleChannelDDR4_2400
from gem5.components.processors.abstract_processor import AbstractProcessor
from gem5.utils.override import overrides

# For ExtenalMemory
from boards.x86_main_board import X86ComposableMemoryBoard
from boards.x86_shared_board import X86SharedMemoryBoard


class X86SpaceControlBoard(X86SharedMemoryBoard):
    """
    This class extends the existing X86Board with dax device support and
    space-control like permission checks. We'll replace this with the remote
    memory board later.

    This board also allows the user to map > 3 GiB memory in the local memory
    space. There is a 64MiB Kernel memory allocated at 0x0 but is kernel
    reserved.
    The main memory range starts at 4G and goes to 20G.
    The remote memory starts at 20G and goes to 36G.
    """

    def __init__(
        self,
        clk_freq: str,
        processor: AbstractProcessor,
        cache_hierarchy: AbstractCacheHierarchy,
        local_memory: AbstractMemorySystem,
        remote_memory: AbstractMemorySystem,
        remote_memory_access_cycles: int = 0,
        remote_memory_address_range: AddrRange = None,
        starting_memory_limit: str = None,
        permission_table_range: AddrRange = None,
    ):
        """
        The board accepts the standard inputs of any given board with the
        exception of the permission table's address range. This is the region
        of the memory reserved for the permission table.
        """
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
        # You need dm caches to connect the ports together
        # self.remote_memory = remote_memory
        # self.remoteMemory = remote_memory
        # The kernel uses memory at 0x0 so we need a tiny range of memory for
        # the kernel to function properly
        self.kernelMemory = SingleChannelDDR4_2400(size="256MiB")

    def _setup_io_devices(self):
        """Sets up the x86 IO devices.

        .. note::

            This is mostly copy-paste from prior X86 FS setups. Some of it
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
            self.bridge.cpu_side_port = (
                self.get_cache_hierarchy().get_mem_side_port()
            )

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
            self.apicbridge.mem_side_port = (
                self.get_cache_hierarchy().get_cpu_side_port()
            )
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
        # Updated the X86 board with MADT entries.
        madt_entries = []
        for i in range(self.get_processor().get_num_cores()):
            bp = X86IntelMPProcessor(
                local_apic_id=i,
                local_apic_version=0x14,
                enable=True,
                bootstrap=(i == 0),
            )
            base_entries.append(bp)
            lapic = X86ACPIMadtLAPIC(acpi_processor_id=i, apic_id=i, flags=1)
            madt_entries.append(lapic)

        io_apic = X86IntelMPIOAPIC(
            id=self.get_processor().get_num_cores(),
            version=0x11,
            enable=True,
            address=0xFEC00000,
        )

        self.pc.south_bridge.io_apic.apic_id = io_apic.id
        base_entries.append(io_apic)
        madt_entries.append(
            X86ACPIMadtIOAPIC(
                id=io_apic.id, address=io_apic.address, int_base=0
            )
        )

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
            X86E820Entry(addr=0, size="639KiB", range_type=1),
            X86E820Entry(addr=0x9FC00, size="385KiB", range_type=2),
            # take care of excess memory
            X86E820Entry(
                addr=0x100000,
                size=f"{self.kernelMemory.get_size() - 0x100000:d}B",
                range_type=1,
            ),
            # Hard code the remote memory region.
            # Mark the rest of physical memory as available
            X86E820Entry(
                addr=0x100000000,
                size=f"{self.memory.get_size()}B",
                range_type=1,
            ),
            X86E820Entry(
                addr=int(self._remoteMemoryAddressRange.start),
                size=f"{self.remote_memory.get_size()}B",
                range_type=12,
            ),

            # X86E820Entry(
            #     addr=0x100000000 + self.memory.get_size(),
            #     size=f"{0x100000000 + self.memory.get_size() + self.remote_memory.get_size()}B",
            #     range_type=12,
            # ),
        ]

        # Reserve the last 16KiB of the 32-bit address space for m5ops
        entries.append(
            X86E820Entry(addr=0xFFFF0000, size="64KiB", range_type=2)
        )

        self.workload.e820_table.entries = entries

    @overrides(X86Board)
    def _setup_memory_ranges(self):
        memory = self.get_local_memory()



        # excess_mem_size = \
        #      memory.get_size() - toMemorySize(
        #     "3GiB")

        self.mem_ranges = [
            # range at 0x0 for the kernel stuff
            AddrRange(0x0, size=self.kernelMemory.get_size()),
            AddrRange(0xC0000000, size=0x100000),  # For I/0
            AddrRange(0x100000000, size=self.memory.get_size()),
            AddrRange(int(self._remoteMemoryAddressRange.start), size=self.remote_memory.get_size())
            # AddrRange(0x100000000 + self.memory.get_size(), size=self.remote_memory.get_size())
        ]

        memory.set_memory_range([self.mem_ranges[2]])
        self.kernelMemory.set_memory_range([self.mem_ranges[0]])
        self.remote_memory.set_memory_range([self.mem_ranges[3]])

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
            
        self.kernelMemory.incorporate_memory(self)

        # Create and connect Xbar for additional latency. This will override
        # the cache's incorporate_cache
        if (
            self._remote_memory_access_cycles > 0
            and self._external_simulator == False
        ):
            self.add_remote_link()
        else:
            # connect the system to the remote memory directly.
            for cntr in self.get_remote_memory().get_memory_controllers():
                cntr.port = self.get_cache_hierarchy().get_mem_side_port()
        # Incorporate the processor into the motherboard.
        self.get_processor().incorporate_processor(self)

        self._connect_things_called = True


    @overrides(X86ComposableMemoryBoard)
    def get_default_kernel_args(self) -> List[str]:
        return [
            "earlyprintk=ttyS0",
            "console=ttyS0",
            "lpj=7999923",
            "root=/dev/sda2",
        ]
