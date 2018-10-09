# Copyright (c) 2018 The Regents of The University of California
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
#
# Authors: Bradley Wang


from BaseCPU import BaseCPU
from BranchPredictor import *
from m5.params import *
from m5.proxy import *

class SimpleDataflowCPU(BaseCPU):
    type = 'SimpleDataflowCPU'
    cxx_header = 'cpu/flexcpu/simple_dataflow_cpu.hh'

    # formatted camelCase to use same parameter name as other CPU models.
    branchPred = Param.BranchPredictor(TournamentBP(
        numThreads = Parent.numThreads), "Branch predictor")
    branch_pred_max_depth = Param.Unsigned(20,
                                           "How many branches deep the "
                                           "predictor is allowed to explore "
                                           "at any given point in time. Set "
                                           "to 0 for infinite.")

    clocked_cpu = Param.Bool(False, "Ties stages of the cpu to clock edges")

    # Tying requests to clock edges basically delays any events that would
    # otherwise happen immediately to the earliest tick on a clock edge (May be
    # the same tick if already on a clock edge).
    clocked_dtb_translation = Param.Bool(Self.clocked_cpu, "Ties requests for "
                                         "data addresses to clock edges.")
    clocked_inst_fetch = Param.Bool(Self.clocked_cpu, "Ties requests for "
                                    "instruction data to clock edges.")
    clocked_itb_translation = Param.Bool(Self.clocked_cpu, "Ties requests for "
                                         "instruction addresses to "
                                         "clock edges.")
    clocked_memory_request = Param.Bool(Self.clocked_cpu, "Ties requests for "
                                        "sending packets to memory to clock "
                                        "edges")

    clocked_execution = Param.Bool(Self.clocked_cpu, "Ties requests for "
                                   "instruction execution to clock edges.")
    execution_latency = Param.Cycles(1, "Number of cycles for each "
                                         "instruction to execute")
    execution_bandwidth = Param.Int(0, "Number of executions each cycle. "
                                       "Assumes a fully-pipelined unit and 0 "
                                       "implies infinite bandwidth.")

    fetch_buffer_size = Param.Unsigned(Parent.cache_line_size,
                                       "Size of fetch buffer in bytes. "
                                       "Also determines size of fetch "
                                       "requests. Should not be larger than a "
                                       "cache line.")

    in_order_begin_execute = Param.Bool(False, "Serialize dependent "
                                               "instruction execution. Allows "
                                               "parallel execution of "
                                               "sequential and independent "
                                               "instructions")
    in_order_execute = Param.Bool(False, "Serialize all instruction "
                                         "execution.")

    instruction_buffer_size = Param.Unsigned(0, "Size of the dynamic "
                                             "instruction buffer. This buffer "
                                             "is used for maintaining the "
                                             "commit order of instructions. "
                                             "Limiting this limits the number "
                                             "of instructions that can be "
                                             "handled at once before commit "
                                             "(out of order). 0 implies an "
                                             "infinitely large buffer.")

    clocked_issue = Param.Bool(Self.clocked_cpu, "Ties requests for issuing "
                                                 "instructions to the clock")
    issue_latency = Param.Cycles(0, "Number of cycles each instruction takes "
                                    "to issue.")
    issue_bandwidth = Param.Int(0, "Number of instructions/micro-ops that can "
                                   "be issued each cycle.")


    strict_serialization = Param.Bool(True, "Controls behavior of serializing "
                                            "flags on instructions.")

    zero_time_microop_execution = Param.Bool(False, "Makes all microops "
                                             "except the last for a macroop "
                                             "take zero time.")

    @classmethod
    def memory_mode(cls):
        return 'timing'
