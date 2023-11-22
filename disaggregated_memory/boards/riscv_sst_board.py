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

from typing import List, Optional, Sequence, Tuple

from boards.riscv_dm_board import RiscvAbstractDMBoard

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

from m5.objects import (
    AddrRange,
    HiFive,
    Frequency,
    Port,
    OutgoingRequestBridge,
    NoncoherentXBar,
)

from m5.util.fdthelper import (
    Fdt,
    FdtNode,
    FdtProperty,
    FdtPropertyStrings,
    FdtPropertyWords,
    FdtState,
)


class RiscvSstDMBoard(RiscvAbstractDMBoard):
    """
    A board capable of full system simulation for multiple RISC-V nodes.
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
        remote_memory: "ExternalRemoteMemoryInterface",
        cache_hierarchy: AbstractCacheHierarchy,
    ) -> None:
        self._localMemory = local_memory
        # Since the remote memory is defined in SST's side, we only need the
        # size of this memory while setting up stuff from Gem5's side.
        self._remoteMemory = remote_memory
        # The remote memory is either setup with a size or an address range.
        # We need to determine if the address range is set. if not, then we
        # need to find the starting and ending of the the external memory
        # range.
        if not self._remoteMemory.get_set_using_addr_ranges():
            # Address ranges were not set, but the system knows the size
            # If the remote_memory_addr_range is not provided, we'll assume
            # that it starts at 0x80000000 + local_memory_size and ends at it's
            # own size
            self._remoteMemory.remote_memory.physical_address_ranges = [
                AddrRange(
                    0x80000000 + self._localMemory.get_size(),
                    size=remote_memory.get_size(),
                )
            ]
        # We need a size as a string to setup this memory.
        self._remoteMemorySize = self._remoteMemory.get_size()

        super().__init__(
            clk_freq=clk_freq,
            processor=processor,
            local_memory=local_memory,
            remote_memory_addr_range=self._remoteMemory.remote_memory.physical_address_ranges[
                0
            ],
            cache_hierarchy=cache_hierarchy,
        )
        self.local_memory = local_memory
        self.remote_memory = self._remoteMemory.remote_memory

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

    def get_remote_memory(self) -> "AbstractMemory":
        """Get the memory (RAM) connected to the board.
        :returns: The remote memory system.
        """
        # raise Exception("cannot call this method")
        return self._remoteMemory

    @overrides(AbstractSystemBoard)
    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return self.get_local_memory().get_mem_ports()

    def get_remote_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return [
            (
                self.get_remote_memory().physical_address_ranges,
                self.get_remote_memory().port,
            )
        ]

    @overrides(RiscvAbstractDMBoard)
    def _incorporate_memory_range(self):
        self.get_local_memory().set_memory_range(self._local_mem_ranges)

    @overrides(RiscvAbstractDMBoard)
    def get_default_kernel_args(self) -> List[str]:
        return ["console=ttyS0", "root={root_value}", "init=/bin/bash", "rw"]

    @overrides(RiscvAbstractDMBoard)
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
        # we need to find whether there is any external latency. if yes, then
        # add xbar to add this latency.

        if self.get_remote_memory().is_xbar_required():
            self.remote_link = NoncoherentXBar(
                frontend_latency=0,
                forward_latency=0,
                response_latency=self.get_remote_memory()._remote_memory_latency,
                width=64,
            )
            # connect the remote memory port to the remote link
            self.get_remote_memory().remote_memory.port = (
                self.remote_link.mem_side_ports
            )
            # The remote link is then connected to the membus
            self.get_cache_hierarchy().membus.mem_side_ports = (
                self.remote_link.cpu_side_ports
            )
        else:
            # Connect the external memory directly to the motherboard.
            self.get_remote_memory().remote_memory.port = (
                self.get_cache_hierarchy().membus.mem_side_ports
            )

        # Incorporate the cache hierarchy for the motherboard.
        if self.get_cache_hierarchy():
            self.get_cache_hierarchy().incorporate_cache(self)

        # Incorporate the processor into the motherboard.
        self.get_processor().incorporate_processor(self)

        self._connect_things_called = True

    @overrides(RiscvAbstractDMBoard)
    def get_default_kernel_args(self) -> List[str]:
        return [
            "console=ttyS0",
            "root={root_value}",
            "init=/root/gem5-init.sh",
            "rw",
        ]

    @overrides(AbstractBoard)
    def _post_instantiate(self):
        """Called to set up anything needed after m5.instantiate"""
        self.get_processor()._post_instantiate()
        if self.get_cache_hierarchy():
            self.get_cache_hierarchy()._post_instantiate()
        self.get_local_memory()._post_instantiate()
