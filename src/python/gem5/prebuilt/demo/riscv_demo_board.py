# Copyright (c) 2024 The Regents of the University of California
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

import m5
from m5.objects import (
    AddrRange,
    BadAddr,
    Bridge,
    CowDiskImage,
    Frequency,
    GenericRiscvPciHost,
    HiFive,
    IGbE_e1000,
    IOXBar,
    PMAChecker,
    Port,
    RawDiskImage,
    RiscvBootloaderKernelWorkload,
    RiscvMmioVirtIO,
    RiscvRTC,
    VirtIOBlock,
    VirtIORng,
)
from m5.util import warn

from ...components.boards.riscv_board import RiscvBoard
from ...components.boards.se_binary_workload import SEBinaryWorkload
from ...components.cachehierarchies.classic.private_l1_shared_l2_cache_hierarchy import (
    PrivateL1SharedL2CacheHierarchy,
)
from ...components.memory import DualChannelDDR4_2400
from ...components.processors.cpu_types import CPUTypes
from ...components.processors.simple_processor import SimpleProcessor
from ...isas import ISA
from ...resources.resource import AbstractResource
from ...utils.override import overrides
from ...utils.requires import requires


class RiscvDemoBoard(RiscvBoard, SEBinaryWorkload):
    """
    This board is based on the X86DemoBoard.

    This prebuilt RISCV board is used for demonstration purposes. It simulates
    an RISCV 1.4GHz dual-core system with a 4GiB DDR4_2400 memory system. A
    private L1, shared L2 cache hierarchy is set with a l1 data and instruction
    cache, each 64KiB with an associativity of 8, and a single bank l2 cache of
    1MiB with an associativity of 16.

    **DISCLAIMER**: This board is solely for demonstration purposes. This board
    is not known to be representative of any real-world system or produce
    reliable statistical results.

    """

    def __init__(self):
        requires(
            isa_required=ISA.RISCV,
        )

        warn(
            "The RiscvDemoBoard is solely for demonstration purposes. "
            "This board is not known to be be representative of any "
            "real-world system. Use with caution."
        )

        memory = DualChannelDDR4_2400(size="4GiB")

        processor = SimpleProcessor(
            cpu_type=CPUTypes.TIMING,
            isa=ISA.RISCV,
            num_cores=2,
        )

        # Here we setup the parameters of the l1 and l2 caches.
        cache_hierarchy = PrivateL1SharedL2CacheHierarchy(
            l1d_size="64KiB", l1i_size="64KiB", l2_size="1MiB"
        )

        super().__init__(
            clk_freq="1.4GHz",
            processor=processor,
            memory=memory,
            cache_hierarchy=cache_hierarchy,
        )

    # Taken from Riscv Matched board. Below are functions that are needed to
    # get SE mode to work.

    @overrides(RiscvBoard)
    def _setup_board(self) -> None:
        if self._is_fs:
            self.workload = RiscvBootloaderKernelWorkload()

            # Contains a CLINT, PLIC, UART, and some functions for the dtb, etc.
            self.platform = HiFive()
            # Note: This only works with single threaded cores.
            self.platform.plic.hart_config = ",".join(
                ["MS" for _ in range(self.processor.get_num_cores())]
            )
            self.platform.attachPlic()
            self.platform.clint.num_threads = self.processor.get_num_cores()

            # Add the RTC
            self.platform.rtc = RiscvRTC(
                frequency=Frequency("100MHz")
            )  # page 77, section 7.1
            self.platform.clint.int_pin = self.platform.rtc.int_pin

            # Incoherent I/O bus
            self.iobus = IOXBar()
            self.iobus.badaddr_responder = BadAddr()
            self.iobus.default = self.iobus.badaddr_responder.pio

            # The virtio disk
            self.disk = RiscvMmioVirtIO(
                vio=VirtIOBlock(),
                interrupt_id=0x8,
                pio_size=4096,
                pio_addr=0x10008000,
            )

            # The virtio rng
            self.rng = RiscvMmioVirtIO(
                vio=VirtIORng(),
                interrupt_id=0x8,
                pio_size=4096,
                pio_addr=0x10007000,
            )

            # Note: This overrides the platform's code because the platform isn't
            # general enough.
            self._on_chip_devices = [self.platform.clint, self.platform.plic]
            self._off_chip_devices = [self.platform.uart, self.disk, self.rng]

        else:
            pass

    @overrides(RiscvBoard)
    def has_io_bus(self) -> bool:
        return self._is_fs

    @overrides(RiscvBoard)
    def get_io_bus(self) -> IOXBar:
        if self.has_io_bus():
            return self.iobus
        else:
            raise NotImplementedError(
                "RiscvDemoBoard does not have an IO bus. "
                "Use `has_io_bus()` to check this."
            )

    @overrides(RiscvBoard)
    def has_coherent_io(self) -> bool:
        return self._is_fs

    @overrides(RiscvBoard)
    def get_mem_side_coherent_io_port(self) -> Port:
        if self.has_coherent_io():
            return self.iobus.mem_side_ports
        else:
            raise NotImplementedError(
                "RiscvDemoBoard does not have any I/O ports. Use has_coherent_io to "
                "check this."
            )