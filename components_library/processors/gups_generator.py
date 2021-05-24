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


from m5.util.convert import toMemorySize

from .gups_generator_core import GUPSGeneratorCore

from .abstract_processor import AbstractProcessor
from ..boards.abstract_board import AbstractBoard

from typing import List


class GUPSGenerator(AbstractProcessor):
    def __init__(
        self,
        num_cores=1,
        mode="ep",
        start_addr=0,
        mem_size="32MB",
        update_limit=0,
    ):
        super(GUPSGenerator, self).__init__(
            cores=self._create_cores(
                num_cores, mode, start_addr, mem_size, update_limit
            )
        )

    def _create_cores(
        self, num_cores, mode, start_addr, mem_size, update_limit
    ):
        self._mem_size = toMemorySize(mem_size)
        self._table_size = mem_size / num_cores
        self._start_addr = start_addr
        if mode == "ep":
            return [
                GUPSGeneratorCore(
                    start_addr=self._start_addr + i * self._table_size,
                    mem_size=self._table_size,
                    update_limit=update_limit,
                )
                for i in range(num_cores)
            ]
        elif mode == 'par':
            raise NotImplementedError('This mode is currently not supported')
        else:
            raise ValueError('No such mode for GUPSGen')
