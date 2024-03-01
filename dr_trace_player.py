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


from typing import Optional
from pathlib import Path

from m5.objects import DRTraceReader
from gem5.utils.override import overrides

from gem5.components.processors.abstract_generator import AbstractGenerator
from gem5.components.boards.abstract_board import AbstractBoard
from dr_trace_player_core import DRTracePlayerCore


class DRTracePlayerGenerator(AbstractGenerator):
    def __init__(
        self,
        trace_directory: Path,
        num_cores: int,
        max_ipc: int,
        max_outstanding_reqs: int,
        clk_freq: Optional[str] = None,
    ):
        super().__init__(
            cores=[
                DRTracePlayerCore(
                    max_ipc=max_ipc,
                    max_outstanding_reqs=max_outstanding_reqs,
                    clk_freq=clk_freq,
                )
                for _ in range(num_cores)
            ]
        )

        self.reader = DRTraceReader(
            directory=trace_directory, num_players=num_cores
        )

        for core in self.get_cores():
            core.set_reader(self.reader)

    @overrides(AbstractGenerator)
    def start_traffic(self):
        """
        Since DRTracePlayer does not need a call to start_traffic to
        start generation. This function is just pass.
        """
        pass

    @overrides(AbstractGenerator)
    def incorporate_processor(self, board: AbstractBoard) -> None:
        super().incorporate_processor(board)
        for core in self.get_cores():
            core.set_memory_range(board.mem_ranges[0])
