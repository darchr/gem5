# Copyright (c) 2023 The Regents of the University of California
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
    opLat = 4
    issueLat = 2


class RocketIntMulFU(MinorDefaultIntMulFU):
    opLat = 1
    issueLat = 4


class RocketIntDivFU(MinorDefaultIntDivFU):
    opLat = 8
    issueLat = 1


class RocketFloatSimdFU(MinorDefaultFloatSimdFU):
    opLat = 7
    issueLat = 1


class RocketPredFU(MinorDefaultPredFU):
    issueLat = 5


class RocketMemReadFU(MinorDefaultMemFU):
    opClasses = minorMakeOpClassSet(["MemRead", "FloatMemRead"])
    opLat = 4
    issueLat = 1


class RocketMemWriteFU(MinorDefaultMemFU):
    opClasses = minorMakeOpClassSet(["MemWrite", "FloatMemWrite"])
    opLat = 2
    issueLat = 1


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
    BTBEntries = 512
    RASSize = 192
    BTBTagSize = 16
    instShiftAmt = 1
    localHistoryTableSize = 256
    localPredictorSize = 4096
    globalPredictorSize = 4096
    choicePredictorSize = 2048
    localCtrBits = 2
    globalCtrBits = 2
    choiceCtrBits = 2
    indirectBranchPred = SimpleIndirectPredictor()
    indirectBranchPred.indirectSets = 1024
    indirectBranchPred.indirectWays = 1
    indirectBranchPred.indirectPathLength = 7
    indirectBranchPred.indirectTagSize = 32


class RocketCPU(RiscvMinorCPU):
    """
    To be fine-tuned.
    """

    threadPolicy = "RoundRobin"

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
    decodeInputBufferSize = 1
    decodeToExecuteForwardDelay = 1
    decodeInputWidth = 3
    decodeCycleInput = False

    # Execute stage
    executeInputWidth = 2
    executeCycleInput = True
    executeIssueLimit = 4
    executeMemoryIssueLimit = 2
    executeCommitLimit = 2
    executeMemoryCommitLimit = 2
    executeInputBufferSize = 2
    executeMaxAccessesInMemory = 1
    executeLSQMaxStoreBufferStoresPerCycle = 1
    executeLSQRequestsQueueSize = 1
    executeLSQTransfersQueueSize = 1
    executeLSQStoreBufferSize = 1
    executeBranchDelay = 1
    executeSetTraceTimeOnCommit = False
    executeSetTraceTimeOnIssue = False
    executeAllowEarlyMemoryIssue = False
    enableIdling = True

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
