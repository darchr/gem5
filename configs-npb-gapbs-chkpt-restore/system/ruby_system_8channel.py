# -*- coding: utf-8 -*-
# Copyright (c) 2016 Jason Lowe-Power
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
#
# Authors: Jason Lowe-Power

import m5
from m5.objects import *
from .fs_tools import *
from .MESI_Two_Level import MESITwoLevelCache


class RubySystem8Channel(System):
    def __init__(
        self,
        kernel,
        disk,
        mem_sys,
        num_cpus,
        assoc,
        dcache_size,  # size of 1 channel
        main_mem_size,
        mem_size_per_channel,
        policy,
        is_link,
        link_lat,
        bypass,
        opts,
        restore=False,
    ):
        super(RubySystem8Channel, self).__init__()
        self._opts = opts

        # Use parallel if using KVM. Don't use parallel is restoring cpt
        self._host_parallel = not restore
        self._restore = restore

        # Set up the clock domain and the voltage domain
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = "5GHz"
        self.clk_domain.voltage_domain = VoltageDomain()
        self._main_mem_size = main_mem_size

        self._data_ranges = [
            AddrRange(
                start=0x100000000,
                size=main_mem_size,
                masks=[1 << 6, 1 << 7, 1 << 8],
                intlvMatch=0,
            ),
            AddrRange(
                start=0x100000000,
                size=main_mem_size,
                masks=[1 << 6, 1 << 7, 1 << 8],
                intlvMatch=1,
            ),
            AddrRange(
                start=0x100000000,
                size=main_mem_size,
                masks=[1 << 6, 1 << 7, 1 << 8],
                intlvMatch=2,
            ),
            AddrRange(
                start=0x100000000,
                size=main_mem_size,
                masks=[1 << 6, 1 << 7, 1 << 8],
                intlvMatch=3,
            ),
            AddrRange(
                start=0x100000000,
                size=main_mem_size,
                masks=[1 << 6, 1 << 7, 1 << 8],
                intlvMatch=4,
            ),
            AddrRange(
                start=0x100000000,
                size=main_mem_size,
                masks=[1 << 6, 1 << 7, 1 << 8],
                intlvMatch=5,
            ),
            AddrRange(
                start=0x100000000,
                size=main_mem_size,
                masks=[1 << 6, 1 << 7, 1 << 8],
                intlvMatch=6,
            ),
            AddrRange(
                start=0x100000000,
                size=main_mem_size,
                masks=[1 << 6, 1 << 7, 1 << 8],
                intlvMatch=7,
            ),
        ]

        self.mem_ranges = [
            AddrRange(Addr("128MiB")),  # kernel data
            AddrRange(0xC0000000, size=0x100000),  # For I/0
        ] + self._data_ranges

        self.initFS(num_cpus)

        # Replace these paths with the path to your disk images.
        # The first disk is the root disk. The second could be used for swap
        # or anything else.
        self.setDiskImages(disk, disk)

        # Change this path to point to the kernel you want to use
        self.workload.object_file = kernel
        # Options specified on the kernel command line
        boot_options = [
            "earlyprintk=ttyS0",
            "console=ttyS0",
            "lpj=7999923",
            "root=/dev/hda1",
        ]

        self.workload.command_line = " ".join(boot_options)

        # Create the CPUs for our system.
        self.createCPU(num_cpus)

        # self.intrctrl = IntrControl()
        self._createMemoryControllers(
            assoc,
            dcache_size,
            mem_size_per_channel,
            policy,
            is_link,
            link_lat,
            bypass,
        )

        if self._restore:
            cpus = self.o3Cpu
        else:
            cpus = self.cpu

        # Create the cache hierarchy for the system.
        self.caches = MESITwoLevelCache()
        self.caches.setup(
            self,
            cpus,
            [self.kernel_mem_ctrl] + self.mem_ctrl,
            [self.mem_ranges[0]] + self._data_ranges,
            [self.pc.south_bridge.ide.dma, self.iobus.mem_side_ports],
            self.iobus,
        )

        self.caches.access_backing_store = True
        self.caches.phys_mem = [
            SimpleMemory(range=self.mem_ranges[0], in_addr_map=False),
            SimpleMemory(range=AddrRange(0x100000000, size=main_mem_size), in_addr_map=False),
        ]

        if self._host_parallel:
            # To get the KVM CPUs to run on different host CPUs
            # Specify a different event queue for each CPU
            for i, cpu in enumerate(self.cpu):
                for obj in cpu.descendants():
                    obj.eventq_index = 0

                # the number of eventqs are set based
                # on experiments with few benchmarks

                cpu.eventq_index = i + 1

    def getHostParallel(self):
        return self._host_parallel

    def totalInsts(self):
        return sum([cpu.totalInsts() for cpu in self.cpu])

    def createCPUThreads(self, cpu):
        for c in cpu:
            c.createThreads()

    def createCPU(self, num_cpus):

        if not self._restore:
            # Note KVM needs a VM and atomic_noncaching
            self.cpu = [X86KvmCPU(cpu_id=i) for i in range(num_cpus)]
            self.kvm_vm = KvmVM()
            self.mem_mode = "atomic_noncaching"
            self.createCPUThreads(self.cpu)

            self.atomicCpu = [
                X86AtomicSimpleCPU(cpu_id=i, switched_out=True)
                for i in range(num_cpus)
            ]
            self.createCPUThreads(self.atomicCpu)

            self.timingCpu = [
                X86TimingSimpleCPU(cpu_id=i, switched_out=True)
                for i in range(num_cpus)
            ]
            self.createCPUThreads(self.timingCpu)

            self.o3Cpu = [
                X86O3CPU(cpu_id=i, switched_out=True) for i in range(num_cpus)
            ]
            self.createCPUThreads(self.o3Cpu)
        else:
            self.o3Cpu = [X86O3CPU(cpu_id=i) for i in range(num_cpus)]
            self.mem_mode = "timing"
            self.createCPUThreads(self.o3Cpu)

    def switchCpus(self, old, new):
        assert new[0].switchedOut()
        m5.switchCpus(self, list(zip(old, new)))

    def setDiskImages(self, img_path_1, img_path_2):
        disk0 = CowDisk(img_path_1)
        disk2 = CowDisk(img_path_2)
        self.pc.south_bridge.ide.disks = [disk0, disk2]

    def _createKernelMemoryController(self, cls):
        return MemCtrl(dram=cls(range=self.mem_ranges[0], kvm_map=False))

    def _createMemoryControllers(
        self,
        assoc,
        dcache_size,
        mem_size_per_channel,
        policy,
        is_link,
        link_lat,
        bypass,
    ):
        self.kernel_mem_ctrl = self._createKernelMemoryController(
            DDR3_1600_8x8
        )

        self.mem_ctrl = [
            PolicyManager(range=r, kvm_map=False) for r in self.mem_ranges[2:]
        ]
        self.loc_mem_ctrl = [MemCtrl() for i in range(8)]
        self.far_mem_ctrl = [MemCtrl() for i in range(2)]

        self.membusPolManFarMem = L2XBar(width=64)
        self.membusPolManFarMem.frontend_latency = link_lat
        self.membusPolManFarMem.response_latency = link_lat

        for i in range(0, 8):
            self.mem_ctrl[i].static_frontend_latency = "10ns"
            self.mem_ctrl[i].static_backend_latency = "10ns"
            self.mem_ctrl[i].loc_mem_policy = policy
            self.mem_ctrl[i].assoc = assoc
            self.mem_ctrl[i].orb_max_size = 128
            self.mem_ctrl[i].dram_cache_size = dcache_size
            if bypass == 0:
                self.mem_ctrl[i].bypass_dcache = False
            elif bypass == 1:
                self.mem_ctrl[i].bypass_dcache = True
            self.membusPolManFarMem.cpu_side_ports = self.mem_ctrl[
                i
            ].far_req_port

        # TDRAM cache
        for i in range(8):
            self.loc_mem_ctrl[i].consider_oldest_write = True
            self.loc_mem_ctrl[i].oldest_write_age_threshold = 2500000
            self.loc_mem_ctrl[i].dram = TDRAM(
                range=self._data_ranges[i], in_addr_map=False, kvm_map=False, null = True
            )
            self.loc_mem_ctrl[i].dram.device_size = dcache_size
            self.mem_ctrl[i].loc_mem = self.loc_mem_ctrl[i].dram
            self.loc_mem_ctrl[i].static_frontend_latency = "1ns"
            self.loc_mem_ctrl[i].static_backend_latency = "1ns"
            self.loc_mem_ctrl[i].static_frontend_latency_tc = "0ns"
            self.loc_mem_ctrl[i].static_backend_latency_tc = "0ns"
            self.loc_mem_ctrl[i].dram.read_buffer_size = 64
            self.loc_mem_ctrl[i].dram.write_buffer_size = 64
            self.loc_mem_ctrl[i].port = self.mem_ctrl[i].loc_req_port

        # main memory
        for i in range(2):
            self.far_mem_ctrl[i] = MemCtrl()
            self.far_mem_ctrl[i].dram = DDR5_4400_4x8(
                range=AddrRange(
                    start=0x100000000,
                    size=self._main_mem_size,
                    masks=[1 << 6],
                    intlvMatch=i,
                ),
                in_addr_map=False, kvm_map=False, null = True
            )
            self.far_mem_ctrl[i].dram.device_size = mem_size_per_channel
            self.far_mem_ctrl[i].static_frontend_latency = "1ns"
            self.far_mem_ctrl[i].static_backend_latency = "1ns"
            self.far_mem_ctrl[i].dram.read_buffer_size = 64
            self.far_mem_ctrl[i].dram.write_buffer_size = 64
            self.membusPolManFarMem.mem_side_ports = self.far_mem_ctrl[i].port

    def initFS(self, cpus):
        self.pc = Pc()

        self.workload = X86FsLinux()

        # North Bridge
        self.iobus = IOXBar()

        # connect the io bus
        # Note: pass in a reference to where Ruby will connect to in the future
        # so the port isn't connected twice.
        self.pc.attachIO(self.iobus, [self.pc.south_bridge.ide.dma])

        ###############################################

        # Add in a Bios information structure.
        self.workload.smbios_table.structures = [X86SMBiosBiosInformation()]

        # Set up the Intel MP table
        base_entries = []
        ext_entries = []
        for i in range(cpus):
            bp = X86IntelMPProcessor(
                local_apic_id=i,
                local_apic_version=0x14,
                enable=True,
                bootstrap=(i == 0),
            )
            base_entries.append(bp)
        io_apic = X86IntelMPIOAPIC(
            id=cpus, version=0x11, enable=True, address=0xFEC00000
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
            X86E820Entry(
                addr=0x100000,
                size="%dB" % (self.mem_ranges[0].size() - 0x100000),
                range_type=1,
            ),
            X86E820Entry(
                addr=0x100000000,
                size="%dB" % (self.mem_ranges[2].size()),
                range_type=1,
            ),
        ]

        # Reserve the last 16kB of the 32-bit address space for m5ops
        entries.append(
            X86E820Entry(addr=0xFFFF0000, size="64kB", range_type=2)
        )

        self.workload.e820_table.entries = entries
