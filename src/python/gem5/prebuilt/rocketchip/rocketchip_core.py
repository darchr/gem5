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
from gem5.components.processors.base_cpu_core import BaseCPUCore
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.utils.override import overrides
from m5.objects.RiscvCPU import RiscvMinorCPU
from m5.objects import (
    BaseMMU,
    Port,
    BaseCPU,
    Process,
)
from m5.objects.BaseMinorCPU import *
from gem5.isas import ISA


class RocketIntFU(MinorDefaultIntFU):
    opLat = 1


class RocketIntMulFU(MinorDefaultIntMulFU):
    opLat = 3


class RocketIntDivFU(MinorDefaultIntDivFU):
    opLat = 6


class RocketFloatSimdFU(MinorDefaultFloatSimdFU):
    pass


class RocketPredFU(MinorDefaultPredFU):
    pass


class RocketMemReadFU(MinorDefaultMemFU):
    opClasses = minorMakeOpClassSet(["MemRead", "FloatMemRead"])
    opLat = 2


class RocketMemWriteFU(MinorDefaultMemFU):
    opClasses = minorMakeOpClassSet(["MemWrite", "FloatMemWrite"])
    opLat = 2


class RocketMiscFU(MinorDefaultMiscFU):
    pass


class RocketVecFU(MinorDefaultVecFU):
    pass


class RocketFUPool(MinorFUPool):
    funcUnits = [
        RocketIntFU(),
        RocketIntFU(),
        RocketIntMulFU(),
        RocketIntDivFU(),
        RocketFloatSimdFU(),
        RocketPredFU(),
        RocketMemReadFU(),
        RocketMemWriteFU(),
        RocketMiscFU(),
        RocketVecFU(),
    ]


class RocketBP(TournamentBP):
    BTBEntries = 32
    RASSize = 6
    localHistoryTableSize = 256
    localPredictorSize = 1024
    globalPredictorSize = 1024
    choicePredictorSize = 1024
    localCtrBits = 1
    globalCtrBits = 1
    choiceCtrBits = 2
    indirectBranchPred = SimpleIndirectPredictor()
    indirectBranchPred.indirectSets = 8


class RocketCPU(RiscvMinorCPU):
    """
    To be fine-tuned.
    """

    threadPolicy = "SingleThreaded"

    # Fetch1 stage
    fetch1LineSnapWidth = 0
    fetch1LineWidth = 0
    fetch1FetchLimit = 1
    fetch1ToFetch2ForwardDelay = 1
    fetch1ToFetch2BackwardDelay = 0

    # Fetch2 stage
    fetch2InputBufferSize = 1
    fetch2ToDecodeForwardDelay = 1
    fetch2CycleInput = True

    # Decode stage
    decodeInputBufferSize = 2
    decodeToExecuteForwardDelay = 1
    decodeInputWidth = 2
    decodeCycleInput = True

    # Execute stage
    executeInputWidth = 2
    executeCycleInput = True
    executeIssueLimit = 2
    executeMemoryIssueLimit = 1
    executeCommitLimit = 2
    executeMemoryCommitLimit = 1
    executeInputBufferSize = 4
    executeMaxAccessesInMemory = 1
    executeLSQMaxStoreBufferStoresPerCycle = 2
    executeLSQRequestsQueueSize = 1
    executeLSQTransfersQueueSize = 2
    executeLSQStoreBufferSize = 3
    executeBranchDelay = 2
    executeSetTraceTimeOnCommit = True
    executeSetTraceTimeOnIssue = False
    executeAllowEarlyMemoryIssue = True
    enableIdling = False

    # Functional Units and Branch Prediction
    executeFuncUnits = RocketFUPool()
    branchPred = RocketBP()


class RocketCore(BaseCPUCore):
    """
    RocketCore models the core of Rocket Chip.
    """

    def __init__(
        self,
        core_id,
    ):
        super().__init__(core=RocketCPU(cpu_id=core_id), isa=ISA.RISCV)
