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

from .abstract_core import AbstractCore
from .abstract_generator_core import AbstractGeneratorCore

from ..utils.override import overrides


class ComplexGeneratorCore(AbstractGeneratorCore):
    def __init__(self):
        super(ComplexGeneratorCore, self).__init__()
        """ The linear generator core interface.

        This class defines the interface for a generator core that will create
        a series of different types of traffic. This core uses PyTrafficGen to
        create and inject the synthetic traffic. This generator could be used
        to create more complex traffics that consist of linear and random
        traffic in different phases.
        """
        self.generator = PyTrafficGen()
        self._traffic = []

    @overrides(AbstractCore)
    def connect_dcache(self, port: Port) -> None:
        self.generator.port = port

    def add_linear(
        self,
        duration: int,
        period: int,
        block_size: int,
        min_addr: int,
        max_addr: int,
        rd_perc: int,
        data_limit: int,
    ) -> None:
        """
        This function will add a linear traffic to the list of traffics in this
        generator core. It uses a PyTrafficGen to create the traffic based on
        the specified params below.

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
        self._main_traffic.append(
            self._create_linear_traffic(
                duration,
                period,
                block_size,
                min_addr,
                max_addr,
                rd_perc,
                data_limit,
            )
        )

    def add_random(
        self,
        duration: int,
        period: int,
        block_size: int,
        min_addr: int,
        max_addr: int,
        rd_perc: int,
        data_limit: int,
    ) -> None:
        """
        This function will add a random traffic to the list of traffics in this
        generator core. It uses a PyTrafficGen to create the traffic based on
        the specified params below.

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
        self._main_traffic.append(
            self._create_random_traffic(
                duration,
                period,
                block_size,
                min_addr,
                max_addr,
                rd_perc,
                data_limit,
            )
        )

    def _create_linear_traffic(
        self,
        duration: int,
        period: int,
        block_size: int,
        min_addr: int,
        max_addr: int,
        rd_perc: int,
        data_limit: int,
    ):
        """
        This function yields (creates) a linear traffic based on the input
        params. Then it will yield (create) an exit traffic (exit traffic is
        used to exit the simulation).

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
        min_period = period
        max_period = period
        yield self.main_generator.createLinear(
            duration,
            min_addr,
            max_addr,
            block_size,
            min_period,
            max_period,
            rd_perc,
            data_limit,
        )
        yield self.main_generator.createExit(0)

    def _create_random_traffic(
        self,
        duration: int,
        period: int,
        block_size: int,
        min_addr: int,
        max_addr: int,
        rd_perc: int,
        data_limit: int,
    ):
        """
        This function yields (creates) a random traffic based on the input
        params. Then it will yield (create) an exit traffic (exit traffic is
        used to exit the simulation).

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
        min_period = period
        max_period = period
        yield self.main_generator.createRandom(
            duration,
            min_addr,
            max_addr,
            block_size,
            min_period,
            max_period,
            rd_perc,
            data_limit,
        )
        yield self.main_generator.createExit(0)

    def start_traffic(self) -> None:
        """
        This function starts generating the traffic at the top of the traffic
        list. It also pops the first element in the list so that every time
        this function is called a new traffic is generated, each instance of a
        call to this function should happen before one instance of the call to
        m5.simulate().
        """
        self.main_generator.start(self._main_traffic.pop(0))
