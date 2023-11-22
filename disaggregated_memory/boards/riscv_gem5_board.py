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

from memories.remote_memory import RemoteChanneledMemory
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

from m5.objects import AddrRange, HiFive, Frequency, Port

from m5.util.fdthelper import (
    Fdt,
    FdtNode,
    FdtProperty,
    FdtPropertyStrings,
    FdtPropertyWords,
    FdtState,
)


class RiscvGem5DMBoard(RiscvAbstractDMBoard):
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
        remote_memory: AbstractMemorySystem,
        cache_hierarchy: AbstractCacheHierarchy,
        remote_memory_addr_range: AddrRange = None,
    ) -> None:
        self._localMemory = local_memory
        self._remoteMemory = remote_memory
        # If the remote_memory_addr_range is not provided, we'll assume that
        # it starts at 0x80000000 + local_memory_size and ends at it's own size
        if remote_memory_addr_range is None:
            remote_memory_addr_range = AddrRange(
                0x80000000 + self._localMemory.get_size(),
                size=remote_memory.get_size(),
            )
        super().__init__(
            clk_freq=clk_freq,
            processor=processor,
            local_memory=local_memory,
            remote_memory_addr_range=remote_memory_addr_range,
            cache_hierarchy=cache_hierarchy,
        )
        self.local_memory = local_memory
        self.remote_memory = remote_memory

        if processor.get_isa() != ISA.RISCV:
            raise Exception(
                "The RISCVBoard requires a processor using the"
                "RISCV ISA. Current processor ISA: "
                f"'{processor.get_isa().name}'."
            )

    def get_remote_memory(self) -> "AbstractMemory":
        """Get the memory (RAM) connected to the board.
        :returns: The remote memory system.
        """
        return self._remoteMemory

    def get_remote_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return self.get_remote_memory().get_mem_ports()

    @overrides(RiscvAbstractDMBoard)
    def _incorporate_memory_range(self):
        # If the memory exists in gem5, then, we need to incorporate this
        # memory range.
        self.get_local_memory().set_memory_range(self._local_mem_ranges)
        self.get_remote_memory().set_memory_range(self._remote_mem_ranges)

    @overrides(RiscvAbstractDMBoard)
    def get_default_kernel_args(self) -> List[str]:
        return [
            "console=ttyS0",
            "root={root_value}",
            "init=/root/gem5-init.sh",
            "rw",
        ]

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
        self.get_remote_memory().incorporate_memory(self)

        # Incorporate the cache hierarchy for the motherboard.
        if self.get_cache_hierarchy():
            self.get_cache_hierarchy().incorporate_cache(self)
            # need to connect the remote links to the board.
            if self.get_cache_hierarchy().is_ruby():
                fatal(
                    "remote memory is only supported in classic caches at " +
                    "the moment!")
            if isinstance(self.get_remote_memory(), RemoteChanneledMemory):
                for ports in self.get_remote_memory().remote_links:   
                    self.get_cache_hierarchy().membus.mem_side_ports = \
                        ports.cpu_side_ports
                        
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
        self.get_remote_memory()._post_instantiate()
