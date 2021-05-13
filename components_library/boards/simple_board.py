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

from components_library.processors.cpu_types import CPUTypes

from m5.objects import AddrRange, SrcClockDomain, VoltageDomain,\
                       Addr, Process, SEWorkload, IOXBar, Port

from .abstract_board import AbstractBoard
from ..processors.abstract_processor import AbstractProcessor
from ..memory.abstract_memory_system import AbstractMemorySystem
from ..cachehierarchies.abstract_cache_hierarchy import AbstractCacheHierarchy
from ..utils.override import overrides

from typing import List


class SimpleBoard(AbstractBoard):
    """
    This is an incredibly simple system. It contains no I/O, and will work to
    work with a classic cache hierarchy setup.

    You can run a bare-metal executable via the `set_workload` function (SE
    mode only).
    """
    def __init__(self, clk_freq: str,
                 processor: AbstractProcessor,
                 memory: AbstractMemorySystem,
                 cache_hierarchy: AbstractCacheHierarchy,
                ) -> None:
        super(SimpleBoard, self).__init__(
                                        processor=processor,
                                        memory=memory,
                                        cache_hierarchy=cache_hierarchy,
                                         )

        # Set up the clock domain and the voltage domain.
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = clk_freq
        self.clk_domain.voltage_domain = VoltageDomain()

        # Set the memory mode.
        # TODO: The CPU has a method for this.
        if self.get_processor().get_cpu_type() == CPUTypes.TIMING \
            or self.get_processor().get_cpu_type() == CPUTypes.O3 :
            self.mem_mode = 'timing'
        elif self.get_processor().get_cpu_type() == CPUTypes.KVM :
            self.mem_mode = "atomic_noncaching"
        elif self.get_processor().get_cpu_type() == CPUTypes.ATOMIC :
            # TODO: FOR Ruby, this should convert to 'atomic_noncaching' and
            # throw a warning that this change has occurred.
            self.mem_mode = "atomic_caching"
        else:
            raise NotImplementedError

        self.mem_ranges = [AddrRange(Addr(memory.get_size_str()))]

    @overrides(AbstractBoard)
    def connect_things(self) -> None:
        # Incorporate the cache hierarchy for the motherboard.
        self.get_cache_hierarchy().incorporate_cache(self)

        # Incorporate the processor into the motherboard.
        self.get_processor().incorporate_processor(self)

        # Incorporate the memory into the motherboard.
        self.get_memory().incorporate_memory(self)

    @overrides(AbstractBoard)
    def get_io_bus(self) -> IOXBar:
        raise NotImplementedError("SimpleBoard does not have an IO Bus.")

    @overrides(AbstractBoard)
    def get_dma_ports(self) -> List[Port]:
        raise NotImplementedError("SimpleBoard does not have DMA Ports.")

    def set_workload(self, binary:str) ->None:
        # This is very limited, single binary, putting on a single Core.

        self.workload = SEWorkload.init_compatible(binary)

        process = Process()
        process.cmd = [binary]
        self.get_processor().get_cpu_simobjects()[0].workload = process
        self.get_processor().get_cpu_simobjects()[0].createThreads()
