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
    Clint,
    LupioBLK,
    LupioIPI,
    LupioPIC,
    LupioRNG,
    LupioRTC,
    LupioTMR,
    LupioTTY,
    LupV,
    Plic,
    Terminal,
    AddrRange,
    CowDiskImage,
    RawDiskImage,
    Frequency,
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

class LupvBoard(SimpleBoard):
    """
    A board capable of full system simulation for RISC-V
    This board uses a set of LupIO education-friendly devices.
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

        excep_code = { 'INT_TIMER_MACHINE': 7, 'INT_EXT_SUPER': 9,
                        'INT_EXT_MACHINE': 11}

        # CLINT
        self.clint = Clint(pio_addr=0x2000000)

        # PLIC
        self.pic = Plic(pio_addr=0xc000000)

        # LUPIO IPI
        self.lupio_ipi = LupioIPI(pio_addr=0x20001000,
                                    int_type = excep_code['INT_EXT_SUPER'],
                                    num_cores = self.processor.get_num_cores())

        # LUPIO PIC
        self.lupio_pic = LupioPIC(pio_addr=0x20002000,
                                    int_type = excep_code['INT_EXT_SUPER'])

        # Interrupt IDs for PIC
        int_ids = { 'TTY': 10, 'BLK': 11, 'RNG': 12}

        #LupV Platform
        self.lupv = LupV(pic = self.lupio_pic,
                        lupio_tty_int_id = int_ids['TTY'])

        # LUPIO RNG
        self.lupio_rng = LupioRNG(pio_addr=0x20005000, platform = self.lupv,
                                    int_id = int_ids['RNG'])

        # LUPIO RTC
        self.lupio_rtc = LupioRTC(pio_addr=0x20004000)

        # LUPIO TMR
        self.lupio_tmr = LupioTMR(pio_addr=0x20006000,
                                    int_type = excep_code['INT_TIMER_MACHINE'],
                                    num_cores = self.processor.get_num_cores())

        # LUPIO TTY
        self.lupio_tty = LupioTTY(pio_addr=0x20007000, platform = self.lupv,
                                    int_id = int_ids['TTY'])
        self.terminal = Terminal()

        # LUPIO BLK
        self.lupio_blk = LupioBLK(pio_addr=0x20000000, platform = self.lupv,
                                    int_id = int_ids['BLK'])

        # Note: This only works with single threaded cores.
        self.pic.n_contexts = self.processor.get_num_cores()

        # Interrupt IDs for LupIO devices
        pic_srcs = [
            self.lupio_tty.int_id,
            self.lupio_blk.int_id,
            self.lupio_rng.int_id
        ]

        self.pic.n_src = 0

        self.lupio_pic.n_src = max(pic_srcs) + 1

        self.lupio_pic.num_threads = self.processor.get_num_cores()

#        self.clint.num_threads = self.processor.get_num_cores()
        self.clint.num_threads = 0

        # Add the RTC
        # TODO: Why 100MHz? Does something else need to change when this does?
        self.rtc = RiscvRTC(frequency=Frequency("100MHz"))
        self.clint.int_pin = self.rtc.int_pin

        # Incoherent I/O bus
        self.iobus = IOXBar()

        # Note: This overrides the platform's code because the platform isn't
        # general enough.
        self._on_chip_devices = [
            self.clint,
            self.pic,
            self.lupio_ipi,
            self.lupio_pic,
            self.lupio_tmr
        ]
        self._off_chip_devices = [
            self.lupio_blk,
            self.lupio_rtc,
            self.lupio_rng,
            self.lupio_tty
        ]

    def _setup_io_devices(self) -> None:
        """Connect the I/O devices to the I/O bus"""
        for device in self._off_chip_devices:
            device.pio = self.iobus.mem_side_ports
        self.lupio_blk.dma = self.iobus.cpu_side_ports

        for device in self._on_chip_devices:
            device.pio = self.get_cache_hierarchy().get_mem_side_port()
        self.bridge = Bridge(delay="10ns")
        self.bridge.mem_side_port = self.iobus.cpu_side_ports
        self.bridge.cpu_side_port = (
            self.get_cache_hierarchy().get_mem_side_port()
        )
        self.bridge.ranges = [
            AddrRange(dev.pio_addr, size=dev.pio_size)
            for dev in self._off_chip_devices
        ]

    def _setup_pma(self) -> None:
        """Set the PMA devices on each core"""
        uncacheable_range = [
            AddrRange(dev.pio_addr, size=dev.pio_size)
            for dev in self._on_chip_devices + self._off_chip_devices
        ]
        # TODO: Not sure if this should be done per-core like in the example
        for cpu in self.get_processor().get_cores():
            cpu.get_mmu().pma_checker = PMAChecker(
                uncacheable=uncacheable_range
            )
        print("pma set up\n")

    @overrides(AbstractBoard)
    def has_io_bus(self) -> bool:
        return True
    @overrides(AbstractBoard)
    def get_io_bus(self) -> IOXBar:
        return self.iobus
    def has_coherent_io(self) -> bool:
        return True
    def get_mem_side_coherent_io_port(self) -> Port:
        return self.iobus.mem_side_ports
    @overrides(AbstractBoard)
    def setup_memory_ranges(self):
        memory = self.get_memory()
        mem_size = memory.get_size()
        self.mem_ranges = [AddrRange(start=0x80000000, size=mem_size)]
        memory.set_memory_range(self.mem_ranges)
    def set_workload(
        self, bootloader: str, disk_image: str, command: Optional[str] = None
    ):
        """Setup the full system files
        See https://github.com/darchr/lupio-gem5/blob/lupio/README.md
        for running the full system, and downloading the right files to do so.
        The command passes in a boot loader and disk image, as well as the
        script to start the simulaiton.
        After the workload is set up, this function will generate the device
        tree file and output it to the output directory.
        **Limitations**
        * Only supports a Linux kernel
        * Must use the provided bootloader and disk image as denoted in the
        README above.
        """
        self.workload.object_file = bootloader
        image = CowDiskImage(
            child=RawDiskImage(read_only=True), read_only=False
        )
        image.child.image_file = disk_image
        self.lupio_blk.image = image

        # Linux boot command flags
        kernel_cmd = [
            "earlycon console=ttyLIO0",
            "root=/dev/lda1",
            "ro"
        ]
        self.workload.command_line = " ".join(kernel_cmd)

        # Note: This must be called after set_workload because it looks for an
        # attribute named "disk" and connects
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
            node.append(FdtPropertyStrings("mmu-type", "riscv,sv48"))
            node.append(FdtPropertyStrings("status", "okay"))
            node.append(FdtPropertyStrings("riscv,isa", "rv64imafdc"))
            # TODO: Should probably get this from the core.
            freq = self.clk_domain.clock[0].frequency
            node.appendCompatible(["riscv"])
            int_phandle = state.phandle(f"cpu@{i}.int_state")
            int_node = FdtNode("interrupt-controller")
            int_state = FdtState(interrupt_cells=1)
            int_phandle = state.phandle(f"cpu@{i}.int_state")
            int_node.append(int_state.interruptCellsProperty())
            int_node.append(FdtProperty("interrupt-controller"))
            int_node.appendCompatible("riscv,cpu-intc")
            int_node.append(FdtPropertyWords("phandle", [int_phandle]))
            node.append(int_node)
            cpus_node.append(node)
        root.append(cpus_node)

        # Chosen node for LupIO-TTY
        chosen_node = FdtNode("chosen")
        chosen_node.append(FdtPropertyStrings("stdout-path",
                            "/soc/lupio-tty@20007000"))
        root.append(chosen_node)

        soc_node = FdtNode("soc")
        soc_state = FdtState(addr_cells=2, size_cells=2)
        soc_node.append(soc_state.addrCellsProperty())
        soc_node.append(soc_state.sizeCellsProperty())
        soc_node.append(FdtProperty("ranges"))
        soc_node.appendCompatible(["simple-bus"])

        # CLINT node
        clint = self.clint
        clint_node = clint.generateBasicPioDeviceNode(
            soc_state, "clint", clint.pio_addr, clint.pio_size
        )
        int_extended = list()
        for i, core in enumerate(self.get_processor().get_cores()):
            phandle = state.phandle(f"cpu@{i}.int_state")
            int_extended.append(phandle)
            int_extended.append(0x3)
            int_extended.append(phandle)
            int_extended.append(0x7)
        clint_node.append(
            FdtPropertyWords("interrupts-extended", int_extended)
        )
        clint_node.appendCompatible(["riscv,clint0"])
        soc_node.append(clint_node)

        # Clock
        clk_node = FdtNode("lupv-clk")
        clk_phandle = state.phandle(clk_node)
        clk_node.append(FdtPropertyWords("phandle", [clk_phandle]))
        clk_node.append(FdtPropertyWords("clock-frequency", [100000000]))
        clk_node.append(FdtPropertyWords("#clock-cells", [0]))
        clk_node.appendCompatible(["fixed-clock"])
        root.append(clk_node)

        # LupioTMR
        lupio_tmr = self.lupio_tmr
        lupio_tmr_node = lupio_tmr.generateBasicPioDeviceNode(soc_state,
                            "lupio-tmr", lupio_tmr.pio_addr,
                            lupio_tmr.pio_size)
        #int_state = FdtState(addr_cells=0, interrupt_cells=1)
        lupio_tmr_node.append(FdtPropertyWords("clocks", [clk_phandle]))
        int_extended = list()
        for i, core in enumerate(self.get_processor().get_cores()):
            phandle = state.phandle(f"cpu@{i}.int_state")
            int_extended.append(phandle)
            int_extended.append(0x5)    #IRQ_S_TIMER
        lupio_tmr_node.append(
            FdtPropertyWords("interrupts-extended", int_extended))
        lupio_tmr_node.appendCompatible(["lupio,tmr"])
        soc_node.append(lupio_tmr_node)

        # PLIC node
        plic = self.pic
        plic_node = plic.generateBasicPioDeviceNode(
            soc_state, "plic", plic.pio_addr, plic.pio_size
        )
        int_state = FdtState(addr_cells=0, interrupt_cells=1)
        plic_node.append(int_state.addrCellsProperty())
        plic_node.append(int_state.interruptCellsProperty())
        phandle = state.phandle(plic)
        plic_node.append(FdtPropertyWords("phandle", [phandle]))
        plic_node.append(FdtPropertyWords("riscv,ndev", 0))
        int_extended = list()
        for i, core in enumerate(self.get_processor().get_cores()):
            phandle = state.phandle(f"cpu@{i}.int_state")
            int_extended.append(phandle)
            int_extended.append(0xB)    #IRQ_M_EXT
        plic_node.append(FdtPropertyWords("interrupts-extended", int_extended))
        plic_node.append(FdtProperty("interrupt-controller"))
        plic_node.appendCompatible(["riscv,plic0"])
        soc_node.append(plic_node)

        # LupioPIC Device
        lupio_pic = self.lupio_pic
        lupio_pic_node = lupio_pic.generateBasicPioDeviceNode(soc_state,
                            "lupio-pic", lupio_pic.pio_addr,
                            lupio_pic.pio_size)
        int_state = FdtState(interrupt_cells=1)
        lupio_pic_node.append(int_state.interruptCellsProperty())
        phandle = state.phandle(lupio_pic)
        lupio_pic_node.append(FdtPropertyWords("phandle", [phandle]))
        int_extended = list()
        for i, core in enumerate(self.get_processor().get_cores()):
            phandle = state.phandle(f"cpu@{i}.int_state")
            int_extended.append(phandle)
            int_extended.append(0x9)    #IRQ_S_EXT
        lupio_pic_node.append(
            FdtPropertyWords("interrupts-extended", int_extended))
        lupio_pic_node.append(FdtProperty("interrupt-controller"))
        lupio_pic_node.appendCompatible(["lupio,pic"])
        soc_node.append(lupio_pic_node)

        # LupioIPI Device
        lupio_ipi = self.lupio_ipi
        lupio_ipi_node = lupio_ipi.generateBasicPioDeviceNode(soc_state,
                                "lupio-ipi", lupio_ipi.pio_addr,
                                lupio_ipi.pio_size)
        int_extended = list()
        for i, core in enumerate(self.get_processor().get_cores()):
            phandle = state.phandle(f"cpu@{i}.int_state")
            int_extended.append(phandle)
            int_extended.append(0x1)    #IRQ_S_SOFT
        lupio_ipi_node.append(
            FdtPropertyWords("interrupts-extended", int_extended))
        lupio_ipi_node.append(FdtProperty("interrupt-controller"))
        lupio_ipi_node.appendCompatible(["lupio,ipi"])
        soc_node.append(lupio_ipi_node)

        # LupioBLK Device
        lupio_blk = self.lupio_blk
        lupio_blk_node = lupio_blk.generateBasicPioDeviceNode(soc_state,
                            "lupio-blk", lupio_blk.pio_addr,
                            lupio_blk.pio_size)
        lupio_blk_node.appendCompatible(["lupio,blk"])
        lupio_blk_node.append(
                FdtPropertyWords("interrupts",
                [self.lupio_blk.int_id]))
        lupio_blk_node.append(
                FdtPropertyWords("interrupt-parent",
                state.phandle(self.lupio_pic)))
        soc_node.append(lupio_blk_node)

        # LupioRNG Device
        lupio_rng = self.lupio_rng
        lupio_rng_node = lupio_rng.generateBasicPioDeviceNode(soc_state,
                            "lupio-rng", lupio_rng.pio_addr,lupio_rng.pio_size)
        lupio_rng_node.appendCompatible(["lupio,rng"])
        lupio_rng_node.append(
                FdtPropertyWords("interrupts",
                [self.lupio_rng.int_id]))
        lupio_rng_node.append(
                FdtPropertyWords("interrupt-parent",
                state.phandle(self.lupio_pic)))
        soc_node.append(lupio_rng_node)

        # LupioRTC Device
        lupio_rtc = self.lupio_rtc
        lupio_rtc_node = lupio_rtc.generateBasicPioDeviceNode(soc_state,
                        "lupio-rtc", lupio_rtc.pio_addr, lupio_rtc.pio_size)
        lupio_rtc_node.appendCompatible(["lupio,rtc"])
        soc_node.append(lupio_rtc_node)

        # LupioTTY Device
        lupio_tty = self.lupio_tty
        lupio_tty_node = lupio_tty.generateBasicPioDeviceNode(soc_state,
                        "lupio-tty", lupio_tty.pio_addr, lupio_tty.pio_size)
        lupio_tty_node.appendCompatible(["lupio,tty"])
        lupio_tty_node.append(
                FdtPropertyWords("interrupts",
                [self.lupio_tty.int_id]))
        lupio_tty_node.append(
                FdtPropertyWords("interrupt-parent",
                state.phandle(self.lupio_pic)))
        soc_node.append(lupio_tty_node)

        root.append(soc_node)
        fdt = Fdt()
        fdt.add_rootnode(root)
        fdt.writeDtsFile(os.path.join(outdir, "device.dts"))
        fdt.writeDtbFile(os.path.join(outdir, "device.dtb"))
