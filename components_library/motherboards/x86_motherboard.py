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


from components_library.motherboards.isas import ISA
import m5
from m5.objects import Cache, Pc, AddrRange, X86FsLinux, \
                       Addr, X86SMBiosBiosInformation, X86IntelMPProcessor,\
                       X86IntelMPIOAPIC, X86IntelMPBus,X86IntelMPBusHierarchy,\
                       X86IntelMPIOIntAssignment, X86E820Entry, Bridge,\
                       IOXBar, IdeDisk, CowDiskImage, RawDiskImage


from .simple_motherboard import SimpleMotherboard
from ..processors.abstract_processor import AbstractProcessor
from ..memory.abstract_memory import AbstractMemory
from ..cachehierarchies.abstract_classic_cache_hierarchy import \
                                    AbstractClassicCacheHierarchy


from typing import Optional


class X86Motherboard(SimpleMotherboard):
    """
    A motherboard capable of full system simulation for X86.
    """

    def __init__(self, clk_freq: str,
                 processor: AbstractProcessor,
                 memory: AbstractMemory,
                 cache_hierarchy: AbstractClassicCacheHierarchy,
                 xbar_width: Optional[int] = 64,
                ) -> None:
        super(X86Motherboard, self).__init__(
                                            clk_freq = clk_freq,
                                            processor=processor,
                                            memory=memory,
                                            cache_hierarchy=cache_hierarchy,
                                            xbar_width = xbar_width,
                                               )

        if self.get_runtime_isa() != ISA.X86:
                raise EnvironmentError("X86Motherboard will only work with the"
                                       " X86 ISA.")

        # Add the address range for the IO
        # TODO: This should definitely NOT be hardcoded to 3GB
        self.mem_ranges = [AddrRange(Addr('3GB')), # All data
                           AddrRange(0xC0000000, size=0x100000), # For I/0
                           ]
        #self.get_system_simobject().mem_ranges.append(
         #       AddrRange(Addr(self.get_memory().get_size_str()),
          #          size=0x100000))

        # Copy and paste ugliness:

        self.get_system_simobject().pc = Pc()

        self.get_system_simobject().workload = X86FsLinux()

        # Constants similar to x86_traits.hh
        IO_address_space_base = 0x8000000000000000
        pci_config_address_space_base = 0xc000000000000000
        interrupts_address_space_base = 0xa000000000000000
        APIC_range_size = 1 << 12

        # North Bridge
        self.get_system_simobject().iobus = IOXBar()
        self.get_system_simobject().bridge = Bridge(delay='50ns')
        self.get_system_simobject().bridge.mem_side_port = \
            self.get_system_simobject().iobus.cpu_side_ports
        self.get_system_simobject().bridge.cpu_side_port = \
            self.get_membus().mem_side_ports

        # Allow the bridge to pass through:
        #  1) kernel configured PCI device memory map address: address range
        #  [0xC0000000, 0xFFFF0000). (The upper 64kB are reserved for m5ops.)
        #  2) the bridge to pass through the IO APIC (two pages, already
        #     contained in 1),
        #  3) everything in the IO address range up to the local APIC, and
        #  4) then the entire PCI address space and beyond.
        #self.get_system_simobject().bridge.ranges = \
        #    [
        #   AddrRange(Addr(self.get_memory().get_size_str()), size=0x3FFF0000),
         #   AddrRange(IO_address_space_base,
         #             interrupts_address_space_base - 1),
          #  AddrRange(pci_config_address_space_base,
          #            Addr.max)
         #   ]

        self.get_system_simobject().bridge.ranges = \
            [
            AddrRange(0xC0000000, 0xFFFF0000),
            AddrRange(IO_address_space_base,
                      interrupts_address_space_base - 1),
            AddrRange(pci_config_address_space_base,
                      Addr.max)
            ]

        # Create a bridge from the IO bus to the memory bus to allow access
        # to the local APIC (two pages)
        self.get_system_simobject().apicbridge = Bridge(delay='50ns')
        self.get_system_simobject().apicbridge.cpu_side_port = \
            self.get_system_simobject().iobus.mem_side_ports
        self.get_system_simobject().apicbridge.mem_side_port = \
            self.get_membus().cpu_side_ports
        self.get_system_simobject().apicbridge.ranges = \
            [AddrRange(interrupts_address_space_base,
                interrupts_address_space_base +
                self.get_processor().get_num_cores() * APIC_range_size
                - 1)]

        # connect the io bus
        self.get_system_simobject().pc.attachIO(
            self.get_system_simobject().iobus)

        # Add a tiny cache to the IO bus.
        # This cache is required for the classic memory model for coherence
        self.get_system_simobject().iocache = Cache(assoc=8,
                            tag_latency = 50,
                            data_latency = 50,
                            response_latency = 50,
                            mshrs = 20,
                            size = '1kB',
                            tgts_per_mshr = 12,
                            addr_ranges = \
                                self.get_system_simobject().mem_ranges)

        self.get_system_simobject().iocache.cpu_side = \
            self.get_system_simobject().iobus.mem_side_ports
        self.get_system_simobject().iocache.mem_side = \
            self.get_system_simobject().membus.cpu_side_ports

        # TODO: For some reason I can't find the IntrControl object? (WHY?!)
        #self.get_system_simobject().intrctrl = IntrControl()

        # Add in a Bios information structure.
        self.get_system_simobject().workload.smbios_table.structures = \
            [X86SMBiosBiosInformation()]

        # Set up the Intel MP table
        base_entries = []
        ext_entries = []
        for i in range(self.get_processor().get_num_cores()):
            bp = X86IntelMPProcessor(
                    local_apic_id = i,
                    local_apic_version = 0x14,
                    enable = True,
                    bootstrap = (i ==0))
            base_entries.append(bp)

        io_apic = X86IntelMPIOAPIC(
                id = self.get_processor().get_num_cores(),
                version = 0x11,
                enable = True,
                address = 0xfec00000)

        self.get_system_simobject().pc.south_bridge.io_apic.apic_id = \
            io_apic.id
        base_entries.append(io_apic)
        pci_bus = X86IntelMPBus(bus_id = 0, bus_type='PCI   ')
        base_entries.append(pci_bus)
        isa_bus = X86IntelMPBus(bus_id = 1, bus_type='ISA   ')
        base_entries.append(isa_bus)
        connect_busses = X86IntelMPBusHierarchy(bus_id=1,
                subtractive_decode=True, parent_bus=0)
        ext_entries.append(connect_busses)

        pci_dev4_inta = X86IntelMPIOIntAssignment(
                interrupt_type = 'INT',
                polarity = 'ConformPolarity',
                trigger = 'ConformTrigger',
                source_bus_id = 0,
                source_bus_irq = 0 + (4 << 2),
                dest_io_apic_id = io_apic.id,
                dest_io_apic_intin = 16)

        base_entries.append(pci_dev4_inta)

        def assignISAInt(irq, apicPin):

            assign_8259_to_apic = X86IntelMPIOIntAssignment(
                    interrupt_type = 'ExtInt',
                    polarity = 'ConformPolarity',
                    trigger = 'ConformTrigger',
                    source_bus_id = 1,
                    source_bus_irq = irq,
                    dest_io_apic_id = io_apic.id,
                    dest_io_apic_intin = 0)
            base_entries.append(assign_8259_to_apic)

            assign_to_apic = X86IntelMPIOIntAssignment(
                    interrupt_type = 'INT',
                    polarity = 'ConformPolarity',
                    trigger = 'ConformTrigger',
                    source_bus_id = 1,
                    source_bus_irq = irq,
                    dest_io_apic_id = io_apic.id,
                    dest_io_apic_intin = apicPin)
            base_entries.append(assign_to_apic)

        assignISAInt(0, 2)
        assignISAInt(1, 1)

        for i in range(3, 15):
            assignISAInt(i, i)

        self.get_system_simobject().workload.intel_mp_table.base_entries = \
            base_entries
        self.get_system_simobject().workload.intel_mp_table.ext_entries = \
            ext_entries

        entries = \
           [
            # Mark the first megabyte of memory as reserved
            X86E820Entry(addr = 0, size = '639kB', range_type = 1),
            X86E820Entry(addr = 0x9fc00, size = '385kB', range_type = 2),
            # Mark the rest of physical memory as available
            X86E820Entry(addr = 0x100000,
                    size = '%dB' % (self.get_system_simobject()\
                        .mem_ranges[0].size() - 0x100000),
                    range_type = 1),
            ]

        # Reserve the last 16kB of the 32-bit address space for m5ops
        entries.append(X86E820Entry(addr = 0xFFFF0000, size = '64kB',
                                    range_type=2))

        self.get_system_simobject().workload.e820_table.entries = entries

    def set_workload(self, kernel: str, disk_image: str, command: str):

        # Set the Linux kernel to use.
        self.get_system_simobject().workload.object_file = kernel

        # Options specified on the kernel command line.
        self.get_system_simobject().workload.command_line = ' '.join(
            [
                'earlyprintk=ttyS0',
                'console=ttyS0',
                'lpj=7999923',
                'root=/dev/hda1',
            ]
        )

        # Create the Disk image SimObject.
        ide_disk = IdeDisk()
        ide_disk.driveID = 'device0'
        ide_disk.image = CowDiskImage(child = RawDiskImage(read_only=True),
                                      read_only=False)
        ide_disk.image.child.image_file = disk_image

        # Attach the SimObject to the system.
        self.get_system_simobject().pc.south_bridge.ide.disks = [ide_disk]

        # Set the script to be passed to the simulated system to execute after
        # boot.
        file_name = '{}/run'.format(m5.options.outdir)
        bench_file = open(file_name, 'w+')
        bench_file.write(command)
        bench_file.close()

        # Set to the system readfile
        self.get_system_simobject().readfile = file_name



