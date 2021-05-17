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


from m5.objects import (
    AtomicSimpleCPU,
    DerivO3CPU,
    TimingSimpleCPU,
    BaseCPU,
    X86KvmCPU,
    KvmVM,
)

from .abstract_processor import AbstractProcessor
from .cpu_types import CPUTypes
from ..boards.abstract_board import AbstractBoard
from ..boards.isas import ISA

from typing import List


class SimpleProcessor(AbstractProcessor):
    def __init__(self, cpu_type: CPUTypes, num_cores: int) -> None:
        super(SimpleProcessor, self).__init__(
            cpu_type=cpu_type, num_cores=num_cores
        )

        if self.get_cpu_type() == CPUTypes.ATOMIC:
            self.cpus = self._create_cores(
                cpu_class=AtomicSimpleCPU, num_cores=num_cores
            )
        elif self.get_cpu_type() == CPUTypes.O3:
            self.cpus = self._create_cores(
                cpu_class=DerivO3CPU, num_cores=num_cores
            )
        elif self.get_cpu_type() == CPUTypes.TIMING:
            self.cpus = self._create_cores(
                cpu_class=TimingSimpleCPU, num_cores=num_cores
            )
        elif self.get_cpu_type() == CPUTypes.KVM:
            self.cpus = self._create_cores(
                cpu_class=X86KvmCPU, num_cores=num_cores
            )
            self.kvm_vm = KvmVM()

        else:
            raise NotADirectoryError(
                f"SimpleProcessor does not currently support cpu type"
                f" {self.get_cpu_type().name}."
            )

        for cpu in self.cpus:
            cpu.createThreads()
            cpu.createInterruptController()

        if self.get_cpu_type() == CPUTypes.KVM:
            # To get the KVM CPUs to run on different host CPUs
            # Specify a different event queue for each CPU
            for i, cpu in enumerate(self.cpus):
                for obj in cpu.descendants():
                    obj.eventq_index = 0
                cpu.eventq_index = i + 1

    def _create_cores(self, cpu_class: BaseCPU, num_cores: int):
        return [cpu_class(cpu_id=i) for i in range(num_cores)]

    def incorporate_processor(self, board: AbstractBoard) -> None:

        if self.get_cpu_type() == CPUTypes.KVM:
            board.kvm_vm = self.kvm_vm

    def get_cpu_simobjects(self) -> List[BaseCPU]:
        return self.cpus
