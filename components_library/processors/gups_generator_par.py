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


from m5.objects import Addr

from ..boards.mem_mode import MEM_MODE

from .gups_generator_core import GUPSGeneratorCore

from .abstract_processor import AbstractProcessor
from ..boards.abstract_board import AbstractBoard


class GUPSGeneratorPAR(AbstractProcessor):
    def __init__(
        self,
        num_cores: int = 1,
        start_addr: Addr = 0,
        mem_size: str = "32MB",
        update_limit: int = 0,
    ):
        """The GUPSGeneratorPAR class
        This class defines the interface for multi core GUPSGenerator, this
        generator could be used in place of a processor. In terms of
        benchmarking this generator implements the parallel (3rd)
        variant of the GUPS benchmark as specified by HPCC website.

        :param start_addr: The start address for allocating the update table.
        :param mem_size: The size of memory to allocate for the update table.
        Should be a power of 2.
        :param update_limit: The number of updates to do before terminating
        simulation. Pass zero to run the benchmark to completion (The amount of
        time it takes to simulate depends on )
        """
        super(GUPSGeneratorPAR, self).__init__(
            cores=self._create_cores(
                num_cores=num_cores,
                start_addr=start_addr,
                mem_size=mem_size,
                update_limit=update_limit,
            )
        )

    """
    Helper function to create cores.
    """
    def _create_cores(
        self,
        num_cores: int,
        start_addr: Addr,
        mem_size: str,
        update_limit: int,
    ):
        return [
            GUPSGeneratorCore(
                start_addr=start_addr,
                mem_size=mem_size,
                update_limit=update_limit / num_cores,
            )
            for i in range(num_cores)
        ]

    def incorporate_processor(self, board: AbstractBoard) -> None:
        board.set_mem_mode(MEM_MODE.TIMING)

    def start_traffic(self):
        pass
