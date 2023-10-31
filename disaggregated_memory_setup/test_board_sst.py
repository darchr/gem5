# Copyright (c) 2021-2023 The Regents of the University of California
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

from m5.objects import Port, AddrRange, OutgoingRequestBridge
from gem5.resources.resource import AbstractResource
from gem5.components.boards.kernel_disk_workload import KernelDiskWorkload
from gem5.components.boards.abstract_system_board import AbstractSystemBoard
from gem5.components.processors.abstract_processor import AbstractProcessor
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.components.processors.abstract_generator import AbstractGenerator
from gem5.components.boards.test_board import TestBoard

from typing import List, Optional, Sequence, Tuple
from gem5.utils.override import overrides


class TestBoardForSST(TestBoard):
    """This board implements a test board for SST/External Memory devices. It
    is assumed that the device has two memories.
    """

    def __init__(
        self,
        clk_freq: str,
        generator: AbstractGenerator,
        remote_memory_size: str,
        memory: Optional[AbstractMemorySystem],
        cache_hierarchy: Optional[AbstractCacheHierarchy],
    ):
        self._localMemory = None
        if memory is not None:
            self._localMemory = memory
        self._remoteMemory = OutgoingRequestBridge()
        self._remoteMemorySize = remote_memory_size
        super().__init__(
            clk_freq=clk_freq,
            generator=generator,
            memory=self._localMemory,
            cache_hierarchy=cache_hierarchy,
        )
        self.local_memory = self._localMemory
        self.remote_memory = self._remoteMemory

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

    @overrides(AbstractSystemBoard)
    def _setup_memory_ranges(self):
        # The local memory can be empty in this case.
        local_memory = None
        remote_memory = self.get_remote_mem()
        # This is a string
        remote_mem_size = self._remoteMemorySize
        # using a _global_ memory range to keep a track of all the memory
        # ranges. This is used to generate the dtb for this machine
        start_addr_for_remote = 0x0
        self._global_mem_ranges = []
        if self.get_local_memory() is not None:
            local_memory = self.get_local_memory()
            self._global_mem_ranges.append(
                AddrRange(start=0x0, size=local_memory.get_size())
            )
            start_addr_for_remote = local_memory.get_size()
            local_memory.set_memory_range(self._global_mem_ranges[0])

        self._global_mem_ranges.append(
            AddrRange(start=start_addr_for_remote, size=remote_mem_size())
        )

        remote_memory.physical_address_ranges = self._global_mem_ranges[-1]

        # the memory has to be setup for both the memory ranges. there is one
        # local memory range, close to the host machine and the other range is
        # pure memory, far from the host.

        # self._local_mem_ranges = [
        #     AddrRange(start=0x80000000, size=local_mem_size)
        # ]

        # The remote memory starts where the local memory ends. Therefore it
        # has to be offset by the local memory's size.
        # self._remote_mem_ranges = [
        #     AddrRange(start=0x80000000 + local_mem_size, size=remote_mem_size)
        # ]

        # keeping a hole in the mem ranges to simulate multiple nodes without
        # using a translator simobject.
        # remote_memory_start_addr = 0x80000000 + local_mem_size + self._instanceCount * 0x80000000
        # self._remote_mem_ranges = [
        #     AddrRange(start=remote_memory_start_addr, size=remote_mem_size)
        # ]

        # self._global_mem_ranges.append(self._local_mem_ranges[0])
        # self._global_mem_ranges.append(self._remote_mem_ranges[0])

        # setting the memory ranges for both of the memory ranges.
        # local_memory.set_memory_range(self._local_mem_ranges)
        # remote_memory.physical_address_ranges = self._remote_mem_ranges
        # remote_memory.set_memory_range(self._remote_mem_ranges)

    @overrides(TestBoard)
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
        if self.get_local_memory() is not None:
            self.get_local_memory().incorporate_memory(self)

        # # Add a NoncoherentXBar here

        # self.remote_link = NoncoherentXBar(
        #     frontend_latency = 0,
        #     forward_latency = 0,
        #     response_latency = 0,
        #     width = 64
        # )
        # self.get_remote_memory().port = self.remote_link.mem_side_ports
        # self.get_cache_hierarchy().membus.mem_side_ports = self.remote_link.cpu_side_ports

        self.get_remote_memory().port = (
            self.get_cache_hierarchy().membus.mem_side_ports
        )
        # self.get_remote_memory().incorporate_memory(self)

        # Incorporate the cache hierarchy for the motherboard.
        if self.get_cache_hierarchy():
            self.get_cache_hierarchy().incorporate_cache(self)

        # Incorporate the processor into the motherboard.
        self.get_processor().incorporate_processor(self)

        self._connect_things_called = True

        if not self.get_cache_hierarchy():
            # If we have no caches, then there must be a one-to-one
            # connection between the generators and the memories.
            assert len(self.get_processor().get_cores()) == 1
            # assert len(self.get_memory().get_mem_ports()) == 1
            self.get_processor().get_cores()[0].connect_dcache(
                self.get_remote_memory().get_remote_mem_ports()[0][1]
            )

    @overrides(TestBoard)
    def _post_instantiate(self):
        """Called to set up anything needed after m5.instantiate"""
        print(
            "__ranges__", self.get_remote_memory().physical_address_ranges[0]
        )
        self.get_processor()._post_instantiate()
        if self.get_cache_hierarchy():
            self.get_cache_hierarchy()._post_instantiate()
        if self.get_local_memory() is not None:
            self.get_local_memory()._post_instantiate()
        # self.get_remote_memory()._post_instantiate()
