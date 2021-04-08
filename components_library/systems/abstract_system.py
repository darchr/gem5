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

from abc import ABCMeta, abstractmethod

from ..motherboards.abstract_motherboard import AbstractMotherboard
from ..memory.abstract_memory import AbstractMemory
from ..processors.abstract_processor import AbstractProcessor

import m5
from m5.objects import Root

class AbstractFullSystem(metaclass=ABCMeta):

    @property
    @abstractmethod
    def motherboard(self) -> AbstractMotherboard:
        pass

    @property
    @abstractmethod
    def processor(self) -> AbstractProcessor:
        pass

    @property
    @abstractmethod
    def memory(self) -> AbstractMemory:
        pass

    def set_workload(self, kernel_path: str, disk_path: str, command: str):

        self.motherboard.set_workload(kernel_path, disk_path, command)

    def instantiate(self) -> None:

        # Create the required root instance
        self.root = Root(full_system = True)

        # Make sure the gem5 system is a child of root.
        self.root.system = self.motherboard.get_system_simobject()
        m5.instantiate()

