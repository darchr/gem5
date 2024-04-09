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

import os
from abc import ABCMeta
from typing import (
    List,
    Optional,
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
    ExternalMemory,
    Frequency,
    HiFive,
    Port,
)
from m5.util.fdthelper import (
    Fdt,
    FdtNode,
    FdtProperty,
    FdtPropertyStrings,
    FdtPropertyWords,
    FdtState,
)

from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.boards.abstract_system_board import AbstractSystemBoard
from gem5.components.boards.kernel_disk_workload import KernelDiskWorkload
from gem5.components.boards.riscv_board import RiscvBoard
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.processors.abstract_processor import AbstractProcessor
from gem5.isas import ISA
from gem5.resources.resource import AbstractResource
from gem5.utils.override import overrides


class RiscvComposableMemoryBoard(RiscvBoard):
    """
    A high-level RISCV board that can zNUMA-capable systems with a remote
    memories. This board is extended from the ArmBoard from Gem5 standard
    library. This board assumes that you will be booting Linux. This board can
    be used to do disaggregated ARM system research while accelerating the
    simulation using kvm.

    The revised ArmComposableMemoryBoard combines the older boards into one
    single board to make the boards compatible with both gem5 and SST.

    **Limitations**
    TBD

    @params
    TODO
    """

    # __metaclass__ = ABCMeta

    def __init__(
        self,
        clk_freq: str,
        processor: AbstractProcessor,
        local_memory: AbstractMemorySystem,
        remote_memory: AbstractMemorySystem,
        cache_hierarchy: AbstractCacheHierarchy,
        remote_memory_access_cycles: int = 0,
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
                        latency. We recommend adding this latency to the \
                        SST-side script"
                )

    @overrides(RiscvBoard)
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
        return self.get_remote_memory().get_size()

    @overrides(RiscvBoard)
    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return self.get_local_memory().get_mem_ports()

    def get_remote_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        """Get the memory (RAM) ports connected to the board.
            This has to be implemented by the child class as we don't know if
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

    @overrides(RiscvBoard)
    def _setup_memory_ranges(self):
        # the memory has to be setup for both the memory ranges. there is one
        # local memory range, close to the host machine and the other range is
        # pure memory, far from the host.
        local_memory = self.get_local_memory()
        # remote_memory = self.get_remote_memory_size()

        local_mem_size = local_memory.get_size()
        remote_mem_size = self.get_remote_memory_size()

        # local memory range will always start from 0x80000000. The remote
        # memory can start and end anywhere as long as it is consistent
        # with the dtb.
        self._local_mem_ranges = [
            AddrRange(start=0x80000000, size=local_mem_size)
        ]

        # The remote memory starts anywhere after the local memory ends. We
        # rely on the user to start and end this range.
        self._remote_mem_ranges = [
            self.get_remote_memory().get_mem_ports()[0][0]
        ]
        # using a _global_ memory range to keep a track of all the memory
        # ranges. This is used to generate the dtb for this machine
        self._global_mem_ranges = []
        self._global_mem_ranges.append(self._local_mem_ranges[0])
        self._global_mem_ranges.append(self._remote_mem_ranges[0])

        # setting the memory ranges for both of the memory ranges. we cannot
        # incorporate the memory at using this abstract board.

        self._incorporate_memory_range()

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

    # @overrides(RiscvBoard)
    def _incorporate_memory_range(self):
        # If the memory exists in gem5, then, we need to incorporate this
        # memory range.
        self.get_local_memory().set_memory_range(self._local_mem_ranges)
        self.get_remote_memory().set_memory_range(self._remote_mem_ranges)

    @overrides(RiscvBoard)
    def get_default_kernel_args(self) -> List[str]:
        return [
            "console=ttyS0",
            "root={root_value}",
            "init=/root/gem5-init.sh",
            "rw",
        ]

    @overrides(RiscvBoard)
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

    @overrides(RiscvBoard)
    def _post_instantiate(self):
        """Called to set up anything needed after m5.instantiate"""
        self.get_processor()._post_instantiate()
        if self.get_cache_hierarchy():
            self.get_cache_hierarchy()._post_instantiate()
        self.get_local_memory()._post_instantiate()
        self.get_remote_memory()._post_instantiate()
