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

from abc import abstractmethod
from m5.objects import Port, Process, PyTrafficGen

from components_library.processors.cpu_types import CPUTypes
from components_library.processors.abstract_core import AbstractCore

class AbstractGeneratorCore(AbstractCore):
    def __init__(self):
        super(AbstractGeneratorCore, self).__init__(CPUTypes.GEN)
        self.setup_dummy_generator()

    def connect_icache(self, port: Port) -> None:
        self.dummy_generator.port = port

    @abstractmethod
    def connect_dcache(self, port: Port) -> None:
        raise NotImplementedError

    def connect_walker_ports(self, port1: Port, port2: Port) -> None:
        pass

    def set_workload(self, process: Process) -> None:
        pass

    def connect_interrupt(self,
        interrupt_requestor: Port,
        interrupt_responce: Port
    ) -> None:
        pass

    def setup_dummy_generator(self):
        self.dummy_generator = PyTrafficGen()
        self._dummy_traffic = self.dummy_generator.createIdle(0)

    @abstractmethod
    def set_traffic(self, traffic = None):
        raise NotImplementedError

    @abstractmethod
    def start_traffic(self):
        raise NotImplementedError