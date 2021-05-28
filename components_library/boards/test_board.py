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

from m5.objects import SrcClockDomain, ClockDomain, VoltageDomain, Port

from .mem_mode import MEM_MODE, mem_mode_to_string
from ..utils.override import overrides
from .abstract_board import AbstractBoard
from ..processors.abstract_processor import AbstractProcessor
from ..memory.abstract_memory_system import AbstractMemorySystem
from ..cachehierarchies.abstract_cache_hierarchy import AbstractCacheHierarchy


from typing import List


class TestBoard(AbstractBoard):
    def __init__(
        self,
        clk_freq: str,
        processor: AbstractProcessor,
        memory: AbstractMemorySystem,
        cache_hierarchy: AbstractCacheHierarchy,
    ):
        super(TestBoard, self).__init__(
            processor=processor,
            memory=memory,
            cache_hierarchy=cache_hierarchy,
        )
        self.clk_domain = SrcClockDomain(
            clock=clk_freq, voltage_domain=VoltageDomain()
        )

        self.mem_ranges = memory.get_memory_ranges()

    def connect_system_port(self, port: Port) -> None:
        self.system_port = port

    def connect_things(self) -> None:
        self.get_cache_hierarchy().incorporate_cache(self)

        self.get_processor().incorporate_processor(self)

        self.get_memory().incorporate_memory(self)

    def get_clock_domain(self) -> ClockDomain:
        return self.clk_domain

    def get_dma_ports(self) -> List[Port]:
        return []

    @overrides(AbstractBoard)
    def set_mem_mode(self, mem_mode: MEM_MODE) -> None:
        self.mem_mode = mem_mode_to_string(mem_mode=mem_mode)
