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
from gem5.components.processors.abstract_processor import AbstractProcessor
from gem5.utils.override import overrides


class X86SpaceControlBoard(X86Board):
    """
    This class extends the existing X86Board with dax device support and
    space-control like permission checks. We'll replace this with the remote
    memory board later
    """

    def __init__(
        self,
        clk_freq: str,
        processor: AbstractProcessor,
        cache_hierarchy: AbstractCacheHierarchy,
        memory: AbstractMemorySystem,
        remote_memory: AbstractMemorySystem,
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
            cache_hierarchy=cache_hierarchy,
            memory=memory,
        )
        # You need dm caches to connect the ports together
        # self.remote_memory = remote_memory
        self.remoteMemory = remote_memory

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
            # Mark the rest of physical memory as available
            X86E820Entry(
                addr=0x100000,
                size=f"{self.mem_ranges[0].size() - 0x100000:d}B",
                range_type=1,
            ),
            # Hard code the remote memory region.
            # Mark the rest of physical memory as available
            X86E820Entry(
                addr=0x100000000,
                size=f"{self.remoteMemory.get_size()}B",
                range_type=12,
            ),
        ]

        # Reserve the last 16KiB of the 32-bit address space for m5ops
        entries.append(
            X86E820Entry(addr=0xFFFF0000, size="64KiB", range_type=2)
        )

        self.workload.e820_table.entries = entries

    @overrides(X86Board)
    def _setup_memory_ranges(self):
        memory = self.get_memory()

        if memory.get_size() > toMemorySize("3GiB"):
            raise Exception(
                "X86Board currently only supports memory sizes up "
                "to 3GiB because of the I/O hole."
            )
        data_range = AddrRange(memory.get_size())
        memory.set_memory_range([data_range])

        self.remoteMemory.set_memory_range(
            [AddrRange(start=0x100000000, size=self.remoteMemory.get_size())]
        )

        # Add the address range for the IO
        self.mem_ranges = [
            data_range,  # All data
            AddrRange(0xC0000000, size=0x100000),  # For I/0
            AddrRange(start=0x100000000, size=self.remoteMemory.get_size()),
        ]
