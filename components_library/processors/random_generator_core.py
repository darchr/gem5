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
from m5.objects import PyTrafficGen, Port

from .abstract_core import AbstractCore
from .abstract_generator_core import AbstractGeneratorCore

from ..utils.override import overrides


class RandomGeneratorCore(AbstractGeneratorCore):
    def __init__(
        self,
        duration: int,
        period: int,
        block_size: int,
        min_addr: int,
        max_addr: int,
        rd_perc: int,
        data_limit: int,
    ):
        super(RandomGeneratorCore, self).__init__()
        """ The linear generator core interface.

        This class defines the interface for a generator core that will create
        a random traffic specific to the parameters below. This core
        uses PyTrafficGen to create and inject the synthetic traffic.

        :param duration: The number of ticks for the generator core to generate
        traffic.
        :param period: The number of ticks that separates two consecutive
        requests.
        :param block_size: The number of bytes to be read/written with each
        request.
        :param min_addr: The lower bound of the address range the generator
        will read/write from/to.
        :param max_addr: The upper bound of the address range the generator
        will read/write from/to.
        :param rd_perc: The percentage of read requests among all the generated
        requests. The write percentage would be equal to 100 - rd_perc.
        :param data_limit: The amount of data in bytes to read/write by the
        generator before stopping generation.
        """
        self.generator = PyTrafficGen()
        self._duration = duration
        self._period = period
        self._block_size = block_size
        self._min_addr = min_addr
        self._max_addr = max_addr
        self._rd_perc = rd_perc
        self._data_limit = data_limit
        self._set_traffic()

    @overrides(AbstractCore)
    def connect_dcache(self, port: Port) -> None:
        self.generator.port = port

    def _set_traffic(self) -> None:
        """
        This private function will set the traffic to be generated.
        """
        self._traffic = self._create_traffic()

    def _create_traffic(self):
        """
        A python generator that yields (creates) a random traffic with the
        specified params in the generator core and then yields (creates) an
        exit traffic.

        :rtype: ???
        """
        self._min_period = self._period
        self._max_period = self._period
        yield self.generator.createRandom(
            self._duration,
            self._min_addr,
            self._max_addr,
            self._block_size,
            self._min_period,
            self._max_period,
            self._rd_perc,
            self._data_limit,
        )
        yield self.generator.createExit(0)

    def start_traffic(self) -> None:
        """
        A call to this function will start generating the traffic, this call
        should happen before m5.simulate()
        """
        print("rtype: ", type(self._traffic))
        self.generator.start(self._traffic)
