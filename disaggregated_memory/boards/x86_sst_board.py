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


from m5.objects import (
    AddrRange,
    VoltageDomain,
    SrcClockDomain,
    Terminal,
    VncServer,
    IOXBar,
    BadAddr,
    Port,
    Pc,
    AddrRange,
    X86FsLinux,
    Addr,
    X86SMBiosBiosInformation,
    X86IntelMPProcessor,
    X86IntelMPIOAPIC,
    X86IntelMPBus,
    X86IntelMPBusHierarchy,
    X86IntelMPIOIntAssignment,
    X86E820Entry,
    Bridge,
    IOXBar,
    IdeDisk,
    CowDiskImage,
    RawDiskImage,
    BaseXBar,
    Port,
    NoncoherentXBar,
    OutgoingRequestBridge,
)

import os
import m5

from abc import ABCMeta

from gem5.utils.override import overrides

from typing import List, Sequence, Tuple

from memories.remote_memory import RemoteChanneledMemory
from boards.x86_dm_board import X86AbstractDMBoard

from gem5.components.processors.abstract_processor import AbstractProcessor
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)

class X86SstDMBoard(X86AbstractDMBoard):
    
    __metaclass__ = ABCMeta
    
    def __init__(
        self,
        clk_freq: str,
        processor: AbstractProcessor,
        local_memory: AbstractMemorySystem,
        remote_memory: "ExternalRemoteMemoryInterface",
        cache_hierarchy: AbstractCacheHierarchy,
        remote_memory_addr_range: AddrRange = None
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
            remote_memory_addr_range=remote_memory_addr_range,
            cache_hierarchy=cache_hierarchy,
        )
        self.local_memory = local_memory
        self.remote_memory = self._remoteMemory.remote_memory
        
    @overrides(X86AbstractDMBoard)
    def get_remote_memory(self) -> "AbstractMemory":
        """Get the memory (RAM) connected to the board.
        :returns: The remote memory system.
        """
        return self._remoteMemory

    @overrides(X86AbstractDMBoard)
    def get_remote_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return [
            (
                self.get_remote_memory().physical_address_ranges,
                self.get_remote_memory().port,
            )
        ]

    # @overrides(X86AbstractDMBoard)
    # def _set_remote_memory_ranges(self):
    #     self.get_remote_memory().set_memory_range(
    #         [self._remoteMemoryAddrRange]
    #     )

    # @overrides(X86AbstractDMBoard)
    # def get_default_kernel_args(self) -> List[str]:

    #     # The default kernel string is taken from the devices.py file.
    #     return [
    #         "console=ttyAMA0",
    #         "lpj=19988480",
    #         "norandmaps",
    #         "root={root_value}",
    #         "rw",
    #         "init=/root/gem5-init.sh",
    #         "kernelcore=2048M"
    #     ]


    @overrides(X86AbstractDMBoard)
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

    @overrides(X86AbstractDMBoard)
    def _post_instantiate(self):
        """Called to set up anything needed after m5.instantiate"""
        self.get_processor()._post_instantiate()
        if self.get_cache_hierarchy():
            self.get_cache_hierarchy()._post_instantiate()
        self.get_local_memory()._post_instantiate()
