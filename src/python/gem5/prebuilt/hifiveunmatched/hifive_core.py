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
from typing import Optional
from ...utils.requires import requires
from gem5.components.processors.abstract_core import AbstractCore
from gem5.components.processors.cpu_types import CPUTypes
from ...isas import ISA
from ...runtime import get_runtime_isa
from ...utils.override import overrides
from m5.objects import (
    BaseMMU,
    Port,
    BaseCPU,
    Process,
)
from m5.objects.BaseMinorCPU import *
from gem5.isas import ISA

class U74IntFU(MinorDefaultIntFU):
    opLat = 1


class U74IntMulFU(MinorDefaultIntMulFU):
    opLat = 3


class U74IntDivFU(MinorDefaultIntDivFU):
    opLat = 6  # not implemented 6 or 68 cycles
    # microops or another method to implement division?
    pass


class U74FloatSimdFU(MinorDefaultFloatSimdFU):
    pass


class U74PredFU(MinorDefaultPredFU):
    pass


class U74MemFU(MinorDefaultMemFU):
    opLat = 3


class U74MiscFU(MinorDefaultMiscFU):
    pass


class U74FUPool(MinorFUPool):
    funcUnits = [
        U74IntFU(),
        U74IntFU(),
        U74IntMulFU(),
        U74IntDivFU(),
        U74FloatSimdFU(),
        U74PredFU(),
        U74MemFU(),
        U74MiscFU(),
    ]


class U74BP(TournamentBP):
    BTBEntries = 16
    RASSize = 6
    localHistoryTableSize = 4096  # is 3.6 KiB but gem5 requires power of 2

    indirectBranchPred = SimpleIndirectPredictor()
    indirectBranchPred.indirectSets = 8

# this will serve as the parent class to RiscvU74CPU
# in src/arch/riscv/RiscvCPU.py
class U74CPU(BaseMinorCPU):
    fetch1FetchLimit = 2
    fetch1ToFetch2BackwardDelay = 0
    fetch2InputBufferSize = 1
    decodeInputBufferSize = 1
    executeInputBufferSize = 1
    decodeToExecuteForwardDelay = 2
    executeFuncUnits = U74FUPool()
    branchPred = U74BP()



class U74Core(AbstractCore):
    def __init__(
        self,
        isa: Optional[ISA]= None
    ):
        super().__init__(cpu_type=CPUTypes.MINOR)
        if isa:
            requires(isa_required=isa)
            self._isa = isa
        else:
            self._isa = get_runtime_isa()
        self.core = AbstractCore.MinorCPU(fetch1FetchLimit = 2,
            fetch1ToFetch2BackwardDelay = 0,
            fetch2InputBufferSize = 1,
            decodeInputBufferSize = 1,
            executeInputBufferSize = 1,
            decodeToExecuteForwardDelay = 2,
            executeFuncUnits = U74FUPool(),
            branchPred = U74BP())
        self.core.createThreads()
    def get_simobject(self) -> BaseCPU:
        return self.core
    @overrides(AbstractCore)
    def get_isa(self) -> ISA:
        return self._isa
    @overrides(AbstractCore)
    def connect_icache(self, port: Port) -> None:
        self.core.icache_port = port
    @overrides(AbstractCore)
    def connect_dcache(self, port: Port) -> None:
        self.core.dcache_port = port
    @overrides(AbstractCore)
    def connect_walker_ports(self, port1: Port, port2: Port) -> None:
        self.core.mmu.connectWalkerPorts(port1, port2)
    @overrides(AbstractCore)
    def set_workload(self, process: Process) -> None:
        self.core.workload = process
    @overrides(AbstractCore)
    def set_switched_out(self, value: bool) -> None:
        self.core.switched_out = value
    @overrides(AbstractCore)
    def connect_interrupt(
        self, interrupt_requestor: Optional[Port] = None,
        interrupt_responce: Optional[Port] = None
    ) -> None:
        # TODO: This model assumes that we will only create an interrupt
        # controller as we require it. Not sure how true this is in all cases.
        self.core.createInterruptController()
    @overrides(AbstractCore)
    def get_mmu(self) -> BaseMMU:
        return self.core.mmu