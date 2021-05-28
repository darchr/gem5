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

from m5.ticks import fromSeconds
from m5.util.convert import toLatency, toMemoryBandwidth

from ..utils.override import overrides
from ..boards.mem_mode import MEM_MODE
from .complex_generator_core import ComplexGeneratorCore

from .abstract_processor import AbstractProcessor
from ..boards.abstract_board import AbstractBoard


class ComplexGenerator(AbstractProcessor):
    def __init__(self, num_cores: int = 1) -> None:
        super(ComplexGenerator, self).__init__(
            cores=[ComplexGeneratorCore() for i in range(num_cores)]
        )
        """The complext generator

        This class defines an external interface to create a list of complex
        generator cores that could replace the processing cores in a board. All

        :param num_cores: The number of complex generator cores to create.
        """

    @overrides(AbstractProcessor)
    def incorporate_processor(self, board: AbstractBoard) -> None:
        board.set_mem_mode(MEM_MODE.TIMING)

    def add_linear(
        self,
        duration: str = "1ms",
        rate: str = "100GB/s",
        block_size: int = 64,
        min_addr: int = 0,
        max_addr: int = 32768,
        rd_perc: int = 100,
        data_limit: int = 0,
    ) -> None:
        duration = fromSeconds(toLatency(duration))
        rate = toMemoryBandwidth(rate)
        period = fromSeconds(block_size / rate)
        for core in self.cores:
            core.add_linear(
                duration,
                period,
                block_size,
                min_addr,
                max_addr,
                rd_perc,
                data_limit,
            )

    def add_random(
        self,
        duration: str = "1ms",
        rate: str = "100GB/s",
        block_size: int = 64,
        min_addr: int = 0,
        max_addr: int = 32768,
        rd_perc: int = 100,
        data_limit: int = 0,
    ) -> None:
        duration = fromSeconds(toLatency(duration))
        rate = toMemoryBandwidth(rate)
        period = fromSeconds(block_size / rate)
        for core in self.cores:
            core.add_random(
                duration,
                period,
                block_size,
                min_addr,
                max_addr,
                rd_perc,
                data_limit,
            )

    def start_traffic(self) -> None:
        for core in self.cores:
            core.start_traffic()
