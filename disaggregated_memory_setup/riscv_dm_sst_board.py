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

import os

from typing import List

from gem5.components.boards.riscv_board import RiscvBoard

from gem5.components.boards.abstract_board import AbstractBoard
from gem5.utils.override import overrides
from gem5.resources.resource import AbstractResource
from gem5.components.boards.kernel_disk_workload import KernelDiskWorkload
from gem5.components.boards.abstract_system_board import AbstractSystemBoard
from gem5.components.processors.abstract_processor import AbstractProcessor
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)

from gem5.isas import ISA

import m5

from m5.objects import AddrRange, HiFive, Frequency

from m5.util.fdthelper import (
    Fdt,
    FdtNode,
    FdtProperty,
    FdtPropertyStrings,
    FdtPropertyWords,
    FdtState,
)


class RiscvDMSSTBoard(RiscvBoard):
    """
    A board capable of full system simulation for RISC-V
    At a high-level, this is based on the HiFive Unmatched board from SiFive.
    This board assumes that you will be booting Linux.

    **Limitations**
    * Only works with classic caches
    """

    def __init__(
        self,
        clk_freq: str,
        processor: AbstractProcessor,
        local_memory: AbstractMemorySystem,
        remote_memory_addr_range: AddrRange,
        cache_hierarchy: AbstractCacheHierarchy,
    ) -> None:
        self._localMemory = local_memory
        self._remoteMemoryAddrRange = remote_memory_addr_range
        super().__init__(
            clk_freq=clk_freq,
            processor=processor,
            memory=local_memory,
            cache_hierarchy=cache_hierarchy,
        )
        self.local_memory = local_memory
        # self.remote_memory = remote_memory

        if processor.get_isa() != ISA.RISCV:
            raise Exception(
                "The RISCVBoard requires a processor using the"
                "RISCV ISA. Current processor ISA: "
                f"'{processor.get_isa().name}'."
            )

    @overrides(AbstractSystemBoard)
    def get_memory(self) -> "AbstractMemory":
        """Get the memory (RAM) connected to the board.

        :returns: The memory system.
        """
        raise NotImplementedError

    def get_local_memory(self) -> "AbstractMemory":
        """Get the memory (RAM) connected to the board.
        :returns: The local memory system.
        """
        return self._localMemory

    # def get_remote_memory(self) -> "AbstractMemory":
    #    """Get the memory (RAM) connected to the board.
    #    :returns: The remote memory system.
    #    """
    #    return self._remoteMemory

    def get_remote_memory_addr_range(self) -> AddrRange:
        return self._remoteMemoryAddrRange

    @overrides(AbstractSystemBoard)
    def _setup_memory_ranges(self):

        # the memory has to be setup for both the memory ranges. there is one
        # local memory range, close to the host machine and the other range is
        # pure memory, far from the host.
        local_memory = self.get_local_memory()
        # remote_memory = self.get_remote_memory()

        local_mem_size = local_memory.get_size()
        # remote_mem_size = remote_memory.get_size()

        self._local_mem_ranges = [
            AddrRange(start=0x80000000, size=local_mem_size)
        ]

        # The remote memory starts where the local memory ends. Therefore it
        # has to be offset by the local memory's size.
        self._remote_mem_ranges = [
            # AddrRange(start=0x80000000 + local_mem_size, size=remote_mem_size)
            self.get_remote_memory_addr_range()
        ]

        # using a _global_ memory range to keep a track of all the memory
        # ranges. This is used to generate the dtb for this machine
        self._global_mem_ranges = []
        self._global_mem_ranges.append(self._local_mem_ranges[0])
        if self._remote_mem_ranges[0]:
            self._global_mem_ranges.append(self._remote_mem_ranges[0])

        # setting the memory ranges for both of the memory ranges.
        local_memory.set_memory_range(self._local_mem_ranges)
        # remote_memory.set_memory_range(self._remote_mem_ranges)

    @overrides(RiscvBoard)
    def generate_device_tree(self, outdir: str) -> None:
        """Creates the dtb and dts files.
        Creates two files in the outdir: 'device.dtb' and 'device.dts'
        :param outdir: Directory to output the files
        """

        state = FdtState(addr_cells=2, size_cells=2, cpu_cells=1)
        root = FdtNode("/")
        root.append(state.addrCellsProperty())
        root.append(state.sizeCellsProperty())
        root.appendCompatible(["riscv-virtio"])

        for idx, mem_range in enumerate(self._global_mem_ranges):
            node = FdtNode("memory@%x" % int(mem_range.start))
            node.append(FdtPropertyStrings("device_type", ["memory"]))
            node.append(
                FdtPropertyWords(
                    "reg",
                    state.addrCells(mem_range.start)
                    + state.sizeCells(mem_range.size()),
                )
            )
            # adding the NUMA node information so that the OS can identify all
            # the NUMA ranges.
            node.append(FdtPropertyWords("numa-node-id", [idx]))
            root.append(node)

        # See Documentation/devicetree/bindings/riscv/cpus.txt for details.
        cpus_node = FdtNode("cpus")
        cpus_state = FdtState(addr_cells=1, size_cells=0)
        cpus_node.append(cpus_state.addrCellsProperty())
        cpus_node.append(cpus_state.sizeCellsProperty())
        # Used by the CLINT driver to set the timer frequency. Value taken from
        # RISC-V kernel docs (Note: freedom-u540 is actually 1MHz)
        cpus_node.append(FdtPropertyWords("timebase-frequency", [100000000]))

        for i, core in enumerate(self.get_processor().get_cores()):
            node = FdtNode(f"cpu@{i}")
            node.append(FdtPropertyStrings("device_type", "cpu"))
            node.append(FdtPropertyWords("reg", state.CPUAddrCells(i)))
            # The CPUs are also associated to the NUMA nodes. All the CPUs are
            # bound to the first NUMA node.
            node.append(FdtPropertyWords("numa-node-id", [0]))
            node.append(FdtPropertyStrings("mmu-type", "riscv,sv48"))
            node.append(FdtPropertyStrings("status", "okay"))
            node.append(FdtPropertyStrings("riscv,isa", "rv64imafdc"))
            # TODO: Should probably get this from the core.
            freq = self.clk_domain.clock[0].frequency
            node.append(FdtPropertyWords("clock-frequency", freq))
            node.appendCompatible(["riscv"])
            int_phandle = state.phandle(f"cpu@{i}.int_state")
            node.appendPhandle(f"cpu@{i}")

            int_node = FdtNode("interrupt-controller")
            int_state = FdtState(interrupt_cells=1)
            int_phandle = int_state.phandle(f"cpu@{i}.int_state")
            int_node.append(int_state.interruptCellsProperty())
            int_node.append(FdtProperty("interrupt-controller"))
            int_node.appendCompatible("riscv,cpu-intc")
            int_node.append(FdtPropertyWords("phandle", [int_phandle]))

            node.append(int_node)
            cpus_node.append(node)

        root.append(cpus_node)

        soc_node = FdtNode("soc")
        soc_state = FdtState(addr_cells=2, size_cells=2)
        soc_node.append(soc_state.addrCellsProperty())
        soc_node.append(soc_state.sizeCellsProperty())
        soc_node.append(FdtProperty("ranges"))
        soc_node.appendCompatible(["simple-bus"])

        # CLINT node
        clint = self.platform.clint
        clint_node = clint.generateBasicPioDeviceNode(
            soc_state, "clint", clint.pio_addr, clint.pio_size
        )
        int_extended = list()
        for i, core in enumerate(self.get_processor().get_cores()):
            phandle = soc_state.phandle(f"cpu@{i}.int_state")
            int_extended.append(phandle)
            int_extended.append(0x3)
            int_extended.append(phandle)
            int_extended.append(0x7)
        clint_node.append(
            FdtPropertyWords("interrupts-extended", int_extended)
        )
        # NUMA information is also associated with the CLINT controller.
        # In this board, the objective to associate one NUMA node to the CPUs
        # and the other node with no CPUs. To generalize this, an additional
        # CLINT controller has to be created on this board, which will make it
        # completely NUMA, instead of just disaggregated NUMA-like board.
        clint_node.append(FdtPropertyWords("numa-node-id", [0]))
        clint_node.appendCompatible(["riscv,clint0"])
        soc_node.append(clint_node)

        # PLIC node
        plic = self.platform.plic
        plic_node = plic.generateBasicPioDeviceNode(
            soc_state, "plic", plic.pio_addr, plic.pio_size
        )

        int_state = FdtState(addr_cells=0, interrupt_cells=1)
        plic_node.append(int_state.addrCellsProperty())
        plic_node.append(int_state.interruptCellsProperty())

        phandle = int_state.phandle(plic)
        plic_node.append(FdtPropertyWords("phandle", [phandle]))
        # Similar to the CLINT interrupt controller, another PLIC controller is
        # required to make this board a general NUMA like board.
        plic_node.append(FdtPropertyWords("numa-node-id", [0]))
        plic_node.append(FdtPropertyWords("riscv,ndev", [plic.n_src - 1]))

        int_extended = list()
        for i, core in enumerate(self.get_processor().get_cores()):
            phandle = state.phandle(f"cpu@{i}.int_state")
            int_extended.append(phandle)
            int_extended.append(0xB)
            int_extended.append(phandle)
            int_extended.append(0x9)

        plic_node.append(FdtPropertyWords("interrupts-extended", int_extended))
        plic_node.append(FdtProperty("interrupt-controller"))
        plic_node.appendCompatible(["riscv,plic0"])

        soc_node.append(plic_node)

        # PCI
        pci_state = FdtState(
            addr_cells=3, size_cells=2, cpu_cells=1, interrupt_cells=1
        )
        pci_node = FdtNode("pci")

        if int(self.platform.pci_host.conf_device_bits) == 8:
            pci_node.appendCompatible("pci-host-cam-generic")
        elif int(self.platform.pci_host.conf_device_bits) == 12:
            pci_node.appendCompatible("pci-host-ecam-generic")
        else:
            m5.fatal("No compatibility string for the set conf_device_width")

        pci_node.append(FdtPropertyStrings("device_type", ["pci"]))

        # Cell sizes of child nodes/peripherals
        pci_node.append(pci_state.addrCellsProperty())
        pci_node.append(pci_state.sizeCellsProperty())
        pci_node.append(pci_state.interruptCellsProperty())
        # PCI address for CPU
        pci_node.append(
            FdtPropertyWords(
                "reg",
                soc_state.addrCells(self.platform.pci_host.conf_base)
                + soc_state.sizeCells(self.platform.pci_host.conf_size),
            )
        )

        # Ranges mapping
        # For now some of this is hard coded, because the PCI module does not
        # have a proper full understanding of the memory map, but adapting the
        # PCI module is beyond the scope of what I'm trying to do here.
        # Values are taken from the ARM VExpress_GEM5_V1 platform.
        ranges = []
        # Pio address range
        ranges += self.platform.pci_host.pciFdtAddr(space=1, addr=0)
        ranges += soc_state.addrCells(self.platform.pci_host.pci_pio_base)
        ranges += pci_state.sizeCells(0x10000)  # Fixed size

        # AXI memory address range
        ranges += self.platform.pci_host.pciFdtAddr(space=2, addr=0)
        ranges += soc_state.addrCells(self.platform.pci_host.pci_mem_base)
        ranges += pci_state.sizeCells(0x40000000)  # Fixed size
        pci_node.append(FdtPropertyWords("ranges", ranges))

        # Interrupt mapping
        plic_handle = int_state.phandle(plic)
        int_base = self.platform.pci_host.int_base

        interrupts = []

        for i in range(int(self.platform.pci_host.int_count)):
            interrupts += self.platform.pci_host.pciFdtAddr(
                device=i, addr=0
            ) + [int(i) + 1, plic_handle, int(int_base) + i]

        pci_node.append(FdtPropertyWords("interrupt-map", interrupts))

        int_count = int(self.platform.pci_host.int_count)
        if int_count & (int_count - 1):
            fatal("PCI interrupt count should be power of 2")

        intmask = self.platform.pci_host.pciFdtAddr(
            device=int_count - 1, addr=0
        ) + [0x0]
        pci_node.append(FdtPropertyWords("interrupt-map-mask", intmask))

        if self.platform.pci_host._dma_coherent:
            pci_node.append(FdtProperty("dma-coherent"))

        soc_node.append(pci_node)

        # UART node
        uart = self.platform.uart
        uart_node = uart.generateBasicPioDeviceNode(
            soc_state, "uart", uart.pio_addr, uart.pio_size
        )
        uart_node.append(
            FdtPropertyWords("interrupts", [self.platform.uart_int_id])
        )
        uart_node.append(FdtPropertyWords("clock-frequency", [0x384000]))
        uart_node.append(
            FdtPropertyWords("interrupt-parent", soc_state.phandle(plic))
        )
        uart_node.appendCompatible(["ns8250"])
        soc_node.append(uart_node)

        # VirtIO MMIO disk node
        disk = self.disk
        disk_node = disk.generateBasicPioDeviceNode(
            soc_state, "virtio_mmio", disk.pio_addr, disk.pio_size
        )
        disk_node.append(FdtPropertyWords("interrupts", [disk.interrupt_id]))
        disk_node.append(
            FdtPropertyWords("interrupt-parent", soc_state.phandle(plic))
        )
        disk_node.appendCompatible(["virtio,mmio"])
        soc_node.append(disk_node)

        # VirtIO MMIO rng node
        rng = self.rng
        rng_node = rng.generateBasicPioDeviceNode(
            soc_state, "virtio_mmio", rng.pio_addr, rng.pio_size
        )
        rng_node.append(FdtPropertyWords("interrupts", [rng.interrupt_id]))
        rng_node.append(
            FdtPropertyWords("interrupt-parent", soc_state.phandle(plic))
        )
        rng_node.appendCompatible(["virtio,mmio"])
        soc_node.append(rng_node)

        root.append(soc_node)

        fdt = Fdt()
        fdt.add_rootnode(root)
        fdt.writeDtsFile(os.path.join(outdir, "device.dts"))
        fdt.writeDtbFile(os.path.join(outdir, "device.dtb"))

    @overrides(KernelDiskWorkload)
    def get_default_kernel_args(self) -> List[str]:
        return ["console=ttyS0", "root={root_value}", "init=/bin/bash", "rw"]

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
        # self.get_remote_memory().incorporate_memory(self)

        # Incorporate the cache hierarchy for the motherboard.
        if self.get_cache_hierarchy():
            self.get_cache_hierarchy().incorporate_cache(self)

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
        # self.get_remote_memory()._post_instantiate()
