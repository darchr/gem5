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

from gem5.components.memory.simple import SingleChannelSimpleMemory 
from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.boards.x86_board import X86Board
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.processors.abstract_processor import AbstractProcessor
from gem5.utils.override import overrides

from typing import List

class X86PermissionBoard(X86Board):
    """
    This class extends the existing X86Board with MMP-like checks.
    """
    def __init__(self,
                clk_freq: str,
                processor: AbstractProcessor,
                cache_hierarchy: AbstractCacheHierarchy,
                memory: AbstractMemorySystem,
                os_memory_range: str,
                permission_table_range: AddrRange = None):
        """
        The board accepts the standard inputs of any given board with the
        exception of the permission table's address range. This is the region
        of the memory reserved for the permission table.
        """
        super().__init__(clk_freq=clk_freq,
                        processor = processor,
                        cache_hierarchy = cache_hierarchy,
                        memory = memory)
        
        # make sure that the address range of the dual_port object is correctly
        # set. can also be done in connect things tbh.
        # This is needed for the traffic generator later.
        if permission_table_range is not None:
            self.cache_hierarchy.get_permission_table().addr_range = permission_table_range

        self._os_memory_range = os_memory_range
        # This board fixes the I/O hole with a certain margin of error.
        # self.initial_memory = SingleChannelSimpleMemory(size="3GiB", latency="50ns", latency_var="0", bandwidth="100GiB/s")
        
        # for port in self.initial_memory.get_memory_controllers():
        #     port = self.get_cache_hierarchy().membus

    """
    # We also want to fix the memory hole on this board. Hm, lets connect the
    # main memory directly to the 4 GiB+ range. reserve the first X GiB for the
    # permission table
    @overrides(X86Board)
    def _setup_memory_ranges(self):
        # Need to create 3 entries for the memory ranges. X86 expects memory at
        # 0x0 for the initial E820 entries. M5 is mapped to the last part of
        # the 3 GiB range before the I/O hole. Then the next range for the OS
        # and the other range for the permission table out of the same physical
        # memory channel.

        # This gives us the memory connected to this board. Basically a size,
        # which we need to move around.
        memory = self.get_memory()

        memory_size = memory.get_size()

        self.mem_ranges = [
            # Make sure that the initial 3GiB is backed up some sort of memory.
            # This is the error rate of the system. 
            AddrRange(start=0x0, size="3GiB"),
            AddrRange(0xC0000000, size=0x100000),  # For I/0
            # The next range is for the permission table.
            self.get_cache_hierarchy().get_permission_table().addr_range,
            AddrRange(start=0x100000000 + self.get_cache_hierarchy().get_permission_table().addr_range.size(), size=memory.get_size() -  self.get_cache_hierarchy().get_permission_table().addr_range.size())
        ]


        self.initial_memory.set_memory_range(
            [AddrRange(start=0x0, size=self.initial_memory.get_size())]
        )
        memory.set_memory_range(
            [AddrRange(start=0x100000000,
                     size=memory_size)]
        )

    @overrides(X86Board)
    def get_default_kernel_args(self): #  -> List[str]:
        assert(len(self._os_memory_range[0:self._os_memory_range.find("G")]) != 0)
        return [
            "earlyprintk=ttyS0",
            "console=ttyS0",
            "lpj=7999923",
            "root=/dev/sda1",
            "mem=" + self._os_memory_range[0:self._os_memory_range.find("G")]
            # "init=/bin/bash",
        ]

    @overrides(X86Board)
    def _setup_io_devices(self):
        Sets up the x86 IO devices.

        Note: This is mostly copy-paste from prior X86 FS setups. Some of it
        may not be documented and there may be bugs.
        

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
            # Mark the first megabyte of memory as reserved. These entries are
            # backed up by the initial_memory.
            X86E820Entry(addr=0, size="639kB", range_type=1),
            X86E820Entry(addr=0x9FC00, size="385kB", range_type=2),
            # Mark the rest of physical memory as available
            # the local address comes first.
            X86E820Entry(
                addr=0x100000,
                size=f"{self.mem_ranges[0].size() - 0x100000:d}B",
                range_type=1,
            ),

            # This memory range is backed up by the main memory. The first 1 G
            # is reserved for the permission table.
            X86E820Entry(
                addr=0x100000000,
                size=f"{self.mem_ranges[1].size()}B",
                range_type=1,
            ),
        ]

        # Reserve the last 16kB of the 32-bit address space for m5ops.
        # This range is backed up by the initial_memory range.
        entries.append(
            X86E820Entry(addr=0xFFFF0000, size="64kB", range_type=2)
        )

        print(entries)
        self.workload.e820_table.entries = entries

    @overrides(AbstractBoard)
    def _connect_things(self) -> None:
        Connects all the components to the board.

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
        
        super()._connect_things()
        self.initial_memory.incorporate_memory(self)
        # if self._connect_things_called:
        #     raise Exception(
        #         "The `_connect_things` function has already been called."
        #     )
        # for port in self.initial_memory.get_memory_controllers():
        #     prot = self.get_cache_hierarchy().membus
        # # Incorporate the memory into the motherboard.
        # self.get_memory().incorporate_memory(self)

        # # Incorporate the cache hierarchy for the motherboard.
        # if self.get_cache_hierarchy():
        #     self.get_cache_hierarchy().incorporate_cache(self)


        # # connect the system to the remote memory directly.
        # for cntr in self.get_memory().get_memory_controllers():
        #     cntr.port = self.get_cache_hierarchy().get_mem_side_port()
        # # Incorporate the processor into the motherboard.
        # self.get_processor().incorporate_processor(self)

        # self._connect_things_called = True
    """
