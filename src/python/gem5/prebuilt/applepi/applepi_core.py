# Copyright (c) 2022 The Regents of the University of California
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
from gem5.utils.requires import requires
from gem5.components.processors.abstract_core import AbstractCore
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.utils.override import overrides
from m5.objects.O3CPU import ArmO3CPU
from m5.objects import (
    BaseMMU,
    Port,
    BaseCPU,
    Process,
)
from m5.objects.BaseMinorCPU import *
from gem5.isas import ISA

class A72IntFU(IntALU):
    opList[0].opLat = 1


class A72IntMulDivFU(IntMultDiv):
    opList[0].opLat = 1



class A72FloatMulDivFU(FP_MultDiv):
    pass

class A72FloatALUFU(FP_ALU):
    pass


class A72FloatSimdFU(SIMD_Unit):
    pass


class A72PredFU(PredALU):
    opList[0].opLat = 1


class A72MemFU(RdWrPort):
    opList[0].opLat = 4
    opList[1].opLat = 1
    opList[2].opLat = 4
    opList[3].opLat = 1


class A72FUPool(DefaultFUPool):
    funcUnits = [
        A72IntFU(),
        A72IntMulDivFU(),
        A72FloatMulDivFU(),
        A72FloatALUFU(),
        A72FloatSimdFU(),
        A72PredFU(),
        A72MemFU(),
    ]


class A72BP(TournamentBP):
    BTBEntries = 16
    RASSize = 6
    localHistoryTableSize = 4096  # is 3.6 KiB but gem5 requires power of 2

    indirectBranchPred = SimpleIndirectPredictor()
    indirectBranchPred.indirectSets = 8

# this will serve as the parent class to RiscvA72CPU
# in src/arch/arm/ArmCPU.py
class A72CPU(ArmO3CPU):

    fuPool = A72FUPool()
    branchPred = A72BP()


class A72Core(AbstractCore):
    """
        A72Core models the core of the Raspberry Pi 4.
        The core has a single thread.
        Each execution unit has its own pipe:

        - Integer 0 (1 stage/cycle)
        - Integer 1 (1 stage/cycle)
        - Integer multi-cycle (4 to 12 cycles)
        - FP/ASIMD 0 (6 to 18 cycles)
        - FP/ASIMD 1 (6 to 32 cycles)
        - Load L1D cache hit: 4 cycles)
        - Store (1 cycle)
        - Branch (1 cycle)

        The branch predictor is a TournamentBP, based on Section 4.2.5 on page 38.
          - BTBEntries: 16 entries
          - RASSize: 6 entries
          - IndirectSets: 8 sets
          - localHistoryTableSize: 4096 B
        NOTE: The BHT of the HiFive Board is 3.6KiB but gem5 requires a power of 2, so the BHT is 4096B.
    """
    def __init__(
        self,
        core_id,
    ):
        super().__init__(cpu_type=CPUTypes.O3)
        self._isa = ISA.ARM
        requires(isa_required=self._isa)
        self.core = A72CPU(cpu_id=core_id)
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
        self,
        interrupt_requestor: Optional[Port] = None,
        interrupt_responce: Optional[Port] = None,
    ) -> None:
        self.core.createInterruptController()

    @overrides(AbstractCore)
    def get_mmu(self) -> BaseMMU:
        return self.core.mmu
