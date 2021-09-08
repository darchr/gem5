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

from m5.objects.Clint import Clint
from m5.objects.LupioBLK import LupioBLK
from m5.objects.LupioRNG import LupioRNG
from m5.objects.LupioRTC import LupioRTC
from m5.objects.LupioTTY import LupioTTY
from m5.objects.Platform import Platform
from m5.objects.Plic import Plic
from m5.objects.Terminal import Terminal
from m5.params import Param
from m5.params import AddrRange
from m5.util.fdthelper import (
    FdtNode,
    FdtProperty,
    FdtPropertyStrings,
    FdtPropertyWords,
    FdtState
)

class LupV(Platform):
    """LupV Platform copied from dev/riscv/HiFive.py
    This platform will continually be upadated to
    incorporate the LupIO devices.

    Implementation:
        This is the base class for the LupV board
        series. It contains the CLINT and PLIC interrupt
        controllers, LupioRTC, Uart and Disk.

    Setup:
        The following sections outline the required
        setup for a RISC-V LupV platform. See
        configs/example/riscv/fs_linux.py for example.

    Driving CLINT:
        CLINT has an interrupt pin which increments
        mtime. It can be connected to any interrupt
        source pin which acts as the RTCCLK pin. An
        abstract RTC wrapper called RiscvRTC can be
        used.

    Attaching PLIC devices:
        PLIC handles external interrupts. Interrupt
        PioDevices should inherit from PlicIntDevice
        (PCI and DMA not yet implemented). It contains
        a parameter interrupt_id which should be used
        to call platform->postPciInt(id).
        All PLIC interrupt devices should be returned
        by _off_chip_devices(). Calling attachPlic sets
        up the PLIC interrupt source count.

    LupioRTC:
        The LupioRTC is a real time clock that supplies
        the current date and time.

    LupioTTY:
        The LupV platform also has an lupio_tty_int_id.
        This is because LupioTTY uses postConsoleInt
        instead of postPciInt. In the future if a Uart
        that inherits PlicIntDevice is implemented,
        this can be removed.

    Disk:
        See fs_linux.py for setup example.

    PMAChecker:
        The PMAChecker will be attached to the MMU of
        each CPU (which allows them to differ). See
        fs_linux.py for setup example.
    """

    type = 'LupV'
    cxx_header = "dev/riscv/lupv.hh"
    cxx_class = 'gem5::LupV'

    # CLINT
    clint = Param.Clint(Clint(pio_addr=0x2000000), "CLINT")

    # PLIC
    pic = Param.Plic(Plic(pio_addr=0xc000000), "PLIC")

    # LUPIO RNG
    lupio_rng = LupioRNG(pio_addr=0x20005000)
    # Interrupt ID for PLIC
    lupio_rng_int_id = Param.Int(lupio_rng.lupio_rng_int_id,
                        "PLIC LupioRNG interrupt ID")
    # LUPIO RTC
    lupio_rtc = Param.LupioRTC(LupioRTC(pio_addr=0x20004000), "LupioRTC")

    # LUPIO TTY
    lupio_tty = LupioTTY(pio_addr=0x20007000)

    # Int source ID to redirect console interrupts to
    lupio_tty_int_id = Param.Int(0xa, "PLIC LupioTTY interrupt ID")
    terminal = Terminal()

    # LUPIO BLK
    lupio_blk = LupioBLK(pio_addr=0x20000000)

    # Int source ID to redirect interrupts to
    lupio_blk_int_id = Param.Int(0xb, "PLIC LupioBLK interrupt ID")

    # Dummy param for generating devicetree
    cpu_count = Param.Int(0, "dummy")

    def _on_chip_devices(self):
        """Returns a list of on-chip peripherals
        """
        return [
            self.clint,
            self.pic
        ]

    def _off_chip_devices(self):
        """Returns a list of off-chip peripherals
        """
        devices = [
            self.lupio_blk,
            self.lupio_rtc,
            self.lupio_rng,
            self.lupio_tty
        ]

        return devices

    def _on_chip_ranges(self):
        """Returns a list of on-chip peripherals
            address range
        """
        return [
            AddrRange(dev.pio_addr, size=dev.pio_size)
            for dev in self._on_chip_devices()
        ]

    def _off_chip_ranges(self):
        """Returns a list of off-chip peripherals
            address range
        """
        return [
            AddrRange(dev.pio_addr, size=dev.pio_size)
            for dev in self._off_chip_devices()
        ]

    def attachPlic(self):
        """Count number of PLIC interrupt sources
        """
        pic_srcs = [self.lupio_blk_int_id, self.lupio_tty_int_id, 
                    self.lupio_rng_int_id]
        for device in self._off_chip_devices():
            if hasattr(device, "interrupt_id"):
                pic_srcs.append(device.interrupt_id)
        self.pic.n_src = max(pic_srcs) + 1

    def attachOnChipIO(self, bus):
        """Attach on-chip IO devices, needs modification
            to support DMA and PCI
        """
        for device in self._on_chip_devices():
            device.pio = bus.mem_side_ports

    def attachOffChipIO(self, bus):
        """Attach off-chip IO devices, needs modification
            to support DMA and PCI
        """
        for device in self._off_chip_devices():
            device.pio = bus.mem_side_ports
        self.lupio_blk.dma = bus.cpu_side_ports
