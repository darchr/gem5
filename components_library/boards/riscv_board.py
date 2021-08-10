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

import os
from typing import Optional

from ..utils.override import overrides
from .simple_board import SimpleBoard
from .abstract_board import AbstractBoard
from ..processors.abstract_processor import AbstractProcessor
from ..memory.abstract_memory_system import AbstractMemorySystem
from ..cachehierarchies.abstract_cache_hierarchy import AbstractCacheHierarchy
from ..isas import ISA
from ..runtime import get_runtime_isa

import m5

from m5.objects import (
    Bridge,
    PMAChecker,
    RiscvLinux,
    AddrRange,
    IOXBar,
    RiscvRTC,
    HiFive,
    CowDiskImage,
    RawDiskImage,
    MmioVirtIO,
    VirtIOBlock,
    Frequency,
)

from m5.util.fdthelper import (
    Fdt,
    FdtNode,
    FdtProperty,
    FdtPropertyStrings,
    FdtPropertyWords,
    FdtState,
)


class RiscvBoard(SimpleBoard):
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
        memory: AbstractMemorySystem,
        cache_hierarchy: AbstractCacheHierarchy,
    ) -> None:
        super().__init__(clk_freq, processor, memory, cache_hierarchy)

        if get_runtime_isa() != ISA.RISCV:
            raise EnvironmentError(
                "RiscvBoard will only work with the RISC-V ISA. Please"
                " recompile gem5 with ISA=RISCV."
            )
        if cache_hierarchy.is_ruby():
            raise EnvironmentError("RiscvBoard is not compatible with Ruby")

        self.workload = RiscvLinux()

        # Contains a CLINT, PLIC, UART, and some functions for the dtb, etc.
        self.platform = HiFive()
        # Note: This only works with single threaded cores.
        self.platform.plic.n_contexts = self.processor.get_num_cores() * 2
        self.platform.attachPlic()
        self.platform.clint.num_threads = self.processor.get_num_cores()

        # TODO: Make the mem size not hardcoded
        self.mem_ranges = [AddrRange(start=0x80000000, size="512MB")]

        # Add the RTC
        # TODO: Why 100MHz? Does something else need to change when this does?
        self.platform.rtc = RiscvRTC(frequency=Frequency("100MHz"))
        self.platform.clint.int_pin = self.platform.rtc.int_pin

    def _setup_io_devices(self) -> None:
        """Connect the I/O devices to the I/O bus"""
        # Incoherent I/O bus
        self.iobus = IOXBar()

        for device in self.platform._off_chip_devices():
            device.pio = self.iobus.mem_side_ports
        for device in self.platform._on_chip_devices():
            device.pio = self.get_cache_hierarchy().get_mem_side_port()

        self.bridge = Bridge(delay="10ns")
        self.bridge.mem_side_port = self.iobus.cpu_side_ports
        self.bridge.cpu_side_port = (
            self.get_cache_hierarchy().get_mem_side_port()
        )
        self.bridge.ranges = self.platform._off_chip_ranges()

        # Modify this for Ruby
        # Create a small cache to allow DMA from devices back to the coherent
        # memory bus.
        # self.iocache = Cache(
        #     assoc=8,
        #     tag_latency=50,
        #     data_latency=50,
        #     response_latency=50,
        #     mshrs=20,
        #     size="1kB",
        #     tgts_per_mshr=12,
        #     addr_ranges=self.mem_ranges,
        # )
        # self.iocache.cpu_side = self.iobus.mem_side_ports
        #self.iocache.mem_side = self.get_cache_hierarchy().get_cpu_side_port()

        self.iobridge = Bridge(delay="10ns", ranges=self.mem_ranges)
        self.iobridge.cpu_side_port = self.iobus.mem_side_ports
        self.iobridge.mem_side_port = (
            self.get_cache_hierarchy().get_cpu_side_port()
        )

    def _setup_pma(self) -> None:
        """Set the PMA devices on each core"""

        uncacheable_range = [
            *self.platform._on_chip_ranges(),
            *self.platform._off_chip_ranges(),
        ]

        # TODO: Not sure if this should be done per-core like in the example
        for cpu in self.get_processor().get_cores():
            cpu.get_mmu().pma_checker = PMAChecker(
                uncacheable=uncacheable_range
            )

    @overrides(AbstractBoard)
    def has_io_bus(self) -> bool:
        return True

    @overrides(AbstractBoard)
    def get_io_bus(self) -> IOXBar:
        return self.iobus

    def set_workload(
        self, bootloader: str, disk_image: str, command: Optional[str] = None
    ):
        """Setup the full system files

        See <url> for the currently tested kernels and OSes.

        The command is an optional string to execute once the OS is fully
        booted, assuming the disk image is setup to run `m5 readfile` after
        booting.

        After the workload is set up, this functino will generate the device
        tree file and output it to the output directory.

        **Limitations**
        * Only supports a Linux kernel
        * Disk must be configured correctly to use the command option
        * This board doesn't support the command option

        :param bootloader: The compiled bootloader with the kernel as a payload
        :param disk_image: A disk image containing the OS data. The first
            partition should be the root partition.
        :param command: The command(s) to run with bash once the OS is booted
        """

        self.workload.object_file = bootloader

        image = CowDiskImage(
            child=RawDiskImage(read_only=True), read_only=False
        )
        image.child.image_file = disk_image
        self.platform.disk = MmioVirtIO(
            vio=VirtIOBlock(image=image),
            interrupt_id=0x8,
            pio_size=4096,
            pio_addr=0x10008000,
        )

        self.workload.command_line = "console=ttyS0 root=/dev/vda ro"

        # TODO: This must be called after set_workload???
        # Because it looks for a attribute named "disk" and connects
        self._setup_io_devices()

        self._setup_pma()

        # Default DTB address if bbl is built with --with-dts option
        self.workload.dtb_addr = 0x87E00000

        # We need to wait to generate the device tree until after the disk is
        # set up. Now that the disk and workload are set, we can generate the
        # device tree file.
        self.generate_device_tree(m5.options.outdir)
        self.workload.dtb_filename = os.path.join(
            m5.options.outdir, "device.dtb"
        )

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

        for mem_range in self.mem_ranges:
            node = FdtNode("memory@%x" % int(mem_range.start))
            node.append(FdtPropertyStrings("device_type", ["memory"]))
            node.append(
                FdtPropertyWords(
                    "reg",
                    state.addrCells(mem_range.start)
                    + state.sizeCells(mem_range.size()),
                )
            )
            root.append(node)

        sections = self.get_processor().get_cores() + [self.platform]

        for section in sections:
            for node in section.generateDeviceTree(state):
                if node.get_name() == root.get_name():
                    root.merge(node)
                else:
                    root.append(node)

        fdt = Fdt()
        fdt.add_rootnode(root)
        fdt.writeDtsFile(os.path.join(outdir, "device.dts"))
        fdt.writeDtbFile(os.path.join(outdir, "device.dtb"))

    # TODO: Finish implementing this.
    def generateDeviceTree(self, state):
        cpus_node = FdtNode("cpus")
        cpus_node.append(FdtPropertyWords("timebase-frequency", [10000000]))
        yield cpus_node

        node = FdtNode("soc")
        local_state = FdtState(
            addr_cells=2, size_cells=2, cpu_cells=state.cpu_cells
        )
        node.append(local_state.addrCellsProperty())
        node.append(local_state.sizeCellsProperty())
        node.append(FdtProperty("ranges"))
        node.appendCompatible(["simple-bus"])

        for subnode in self.recurseDeviceTree(local_state):
            node.append(subnode)

        yield node
