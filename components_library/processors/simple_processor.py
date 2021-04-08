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

from components_library.motherboards.isas import ISA
from m5.objects import AtomicSimpleCPU, DerivO3CPU, TimingSimpleCPU, BaseCPU,\
                       X86KvmCPU, KvmVM

from .abstract_processor import AbstractProcessor
from .cpu_types import CPUTypes
from ..motherboards.abstract_motherboard import AbstractMotherboard

from typing import List

class SimpleProcessor(AbstractProcessor):

    def __init__(self, cpu_type: CPUTypes, num_cores: int) -> None:
        super(SimpleProcessor, self).__init__(cpu_type = cpu_type,
                                              num_cores = num_cores)

        if self.get_cpu_type() == CPUTypes.ATOMIC:
            self._cpus = self._create_cores(cpu_class = AtomicSimpleCPU,
                                            num_cores = num_cores)
        elif self.get_cpu_type() == CPUTypes.O3:
            self._cpus = self._create_cores(cpu_class = DerivO3CPU,
                                            num_cores = num_cores)
        elif self.get_cpu_type() == CPUTypes.TIMING:
            self._cpus = self._create_cores(cpu_class = TimingSimpleCPU,
                                            num_cores = num_cores)
        elif self.get_cpu_type() == CPUTypes.KVM:
            self._cpus = self._create_cores(cpu_class = X86KvmCPU,
                                            num_cores = num_cores)
            self.kvm_vm = KvmVM()
        else:
            raise NotADirectoryError("SimpleProcessor does not currently " +
                                     "support cpu type '" +
                                     self.get_cpu_type().name + "'")

        for cpu in self._cpus:
            cpu.createThreads()

    def _create_cores(self, cpu_class: BaseCPU, num_cores: int):
        return [cpu_class(cpu_id = i) for i in range(num_cores)]

    def incorporate_processor(self, motherboard: AbstractMotherboard) -> None:

        motherboard.get_system_simobject().detailedCPU = \
            self.get_cpu_simobjects()
        for cpu in self.get_cpu_simobjects():
            # create the interrupt controller CPU and connect to the membus
            cpu.createInterruptController()

            if motherboard.get_runtime_isa() == ISA.X86 :
                cpu.interrupts[0].pio = motherboard.get_membus().mem_side_ports
                cpu.interrupts[0].int_requestor = \
                    motherboard.get_membus().cpu_side_ports
                cpu.interrupts[0].int_responder = \
                    motherboard.get_membus().mem_side_ports

    def get_cpu_simobjects(self) -> List[BaseCPU]:
        return self._cpus
