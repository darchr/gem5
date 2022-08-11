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

from gem5.utils.override import overrides
from gem5.components.boards.mem_mode import MemMode
from gem5.components.processors.simple_core import SimpleCore

from m5.util import warn

from gem5.components.processors.abstract_processor import AbstractProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.components.boards.abstract_board import AbstractBoard
from gem5.prebuilt.hifiveunmatched.hifive_core import U74Core

from typing import Optional


class U74Processor(AbstractProcessor):
    """
    A U74Processor contains a number of cores of U74Core.
    """

    def __init__(
        self,
        isa: Optional[ISA] = None,
    ) -> None:
        """
        param cpu_type: The CPU type for each type in the processor.
:
        :param num_cores: The number of CPU cores in the processor.
        :param isa: The ISA of the processor. This argument is optional. If not
        set the `runtime.get_runtime_isa` is used to determine the ISA at
        runtime. **WARNING**: This functionality is deprecated. It is
        recommended you explicitly set your ISA via SimpleProcessor
        construction.
        """
        super().__init__(
            cores=self._create_cores(
                isa = isa,
            )
        )

    def _create_cores(
        self,
        isa: Optional[ISA]
    ):
        return U74Core()
        

    @overrides(AbstractProcessor)
    def incorporate_processor(self, board: AbstractBoard) -> None:
        if self._cpu_type == CPUTypes.KVM:
            board.kvm_vm = self.kvm_vm

        # Set the memory mode.
        if self._cpu_type in (CPUTypes.TIMING, CPUTypes.O3, CPUTypes.MINOR):
            board.set_mem_mode(MemMode.TIMING)
        elif self._cpu_type == CPUTypes.KVM:
            board.set_mem_mode(MemMode.ATOMIC_NONCACHING)
        elif self._cpu_type == CPUTypes.ATOMIC:
            if board.get_cache_hierarchy().is_ruby():
                warn(
                    "Using an atomic core with Ruby will result in "
                    "'atomic_noncaching' memory mode. This will skip caching "
                    "completely."
                )
            else:
                board.set_mem_mode(MemMode.ATOMIC)
        else:
            raise NotImplementedError

        if self._cpu_type == CPUTypes.KVM:
            # To get the KVM CPUs to run on different host CPUs
            # Specify a different event queue for each CPU
            for i, core in enumerate(self.cores):
                for obj in core.get_simobject().descendants():
                    obj.eventq_index = 0
                core.get_simobject().eventq_index = i + 1