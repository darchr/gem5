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

from m5.objects import PyTrafficGen, Port

from m5.util.convert import toLatency, toMemoryBandwidth

from .abstract_generator_core import AbstractGeneratorCore

from ..utils.override import overrides

class RandomGeneratorCore(AbstractGeneratorCore):
    def __init__(
        self,
        duration: str,
        rate: str,
        block_size: int,
        min_addr: int,
        max_addr: int,
        rd_perc: int,
        data_limit: int,
    ):
        super(RandomGeneratorCore, self).__init__()
        self.main_generator = PyTrafficGen()
        self._duration = int(toLatency(duration) * 1e12)
        self._rate = toMemoryBandwidth(rate)
        self._block_size = block_size
        self._min_addr = min_addr
        self._max_addr = max_addr
        self._rd_perc = rd_perc
        self._data_limit = data_limit
        self._set_traffic()

    @overrides(AbstractGeneratorCore)
    def connect_dcache(self, port: Port) -> None:
        self.main_generator.port = port

    def _create_traffic(self):
        period = int(float(self._block_size * 1e12) / self._rate)
        self._min_period = period
        self._max_period = period
        yield self.main_generator.createRandom(
            self._duration,
            self._min_addr,
            self._max_addr,
            self._block_size,
            self._min_period,
            self._max_period,
            self._rd_perc,
            self._data_limit,
        )
        yield self.main_generator.createExit(0)

    @overrides(AbstractGeneratorCore)
    def _set_traffic(self) -> None:
        self._main_traffic = self.create_traffic()

    @overrides(AbstractGeneratorCore)
    def start_traffic(self) -> None:
        self.main_generator.start(self._main_traffic)
