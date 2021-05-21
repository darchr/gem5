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

from .complex_generator_core import ComplexGeneratorCore

from .abstract_processor import AbstractProcessor
from ..boards.abstract_board import AbstractBoard

from typing import List


class ComplexGenerator(AbstractProcessor):
    """
    A SimpeProcessor contains a number of cores of a a single CPUType.
    """

    def __init__(
        self,
        num_cores: int,
    ) -> None:
        super(ComplexGenerator, self).__init__(
            cores=self._create_cores(
                num_cores=num_cores,
            )
        )

    def _create_cores(
        self,
        num_cores,
    ):
        return [ComplexGeneratorCore() for i in range(num_cores)]

    def incorporate_processor(self, board: AbstractBoard) -> None:
        # TODO: Shouldn't we do this with a setter function?
        board.mem_mode = "timing"

    def add_linear(
        self,
        duration="1ms",
        rate="100GB/s",
        block_size=64,
        min_addr=0,
        max_addr=32768,
        rd_perc=100,
        data_limit=0,
    ):
        for core in self.cores:
            core.add_linear(
                duration,
                rate,
                block_size,
                min_addr,
                max_addr,
                rd_perc,
                data_limit,
            )

    def add_random(
        self,
        duration="1ms",
        rate="100GB/s",
        block_size=64,
        min_addr=0,
        max_addr=32768,
        rd_perc=100,
        data_limit=0,
    ):
        for core in self.cores:
            core.add_random(
                duration,
                rate,
                block_size,
                min_addr,
                max_addr,
                rd_perc,
                data_limit,
            )

    def start_traffic(self):
        for core in self.cores:
            core.start_traffic()
