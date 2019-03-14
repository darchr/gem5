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

class FlexCPU(BaseCPU):
    type = 'FlexCPU'
    cxx_header = 'cpu/flexcpu/flexcpu.hh'

    # formatted camelCase to use same parameter name as other CPU models.
    branchPred = Param.BranchPredictor(TournamentBP(
        numThreads = Parent.numThreads), "Branch predictor")
    branch_pred_max_depth = Param.Unsigned(20,
                                           "How many branches deep the "
                                           "predictor is allowed to explore "
                                           "at any given point in time. Set "
                                           "to 0 for infinite.")

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

    op_buffer_size = Param.Unsigned(0, "Size of the InflightInst buffer. This "
                                       "buffer stores decoded (possibly "
                                       "microcoded) instructions used for "
                                       "maintaining the commit order of "
                                       "instructions and microops. Limiting "
                                       "this limits the number of "
                                       "instructions AND microops that can be "
                                       "handled at once before commit (out of "
                                       "order). 0 implies an infinitely large "
                                       "buffer.")

    ignore_microops = Param.Bool(False, "When true, assume microops take 0 "
                                 "time and treat max_inflight_insts as "
                                 "counting the number of instructions, not "
                                 "microops.")

    issue_latency = Param.Cycles(0, "Number of cycles each instruction takes "
                                    "to issue.")
    issue_bandwidth = Param.Int(0, "Number of instructions/micro-ops that can "
                                   "be issued each cycle.")

    stld_forward_enabled = Param.Bool(True,
                                    "Whether stores which contain a superset "
                                    "of the data for a future load should "
                                    "forward that data to the load, bypassing "
                                    "memory.")

    strict_serialization = Param.Bool(True, "Controls behavior of serializing "
                                            "flags on instructions. As of the "
                                            "development of this CPU model, "
                                            "gem5 defines flags for "
                                            "serialization only in specific "
                                            "directions, but doesn't seem to "
                                            "use them consistently, so we "
                                            "default to forcing serialization "
                                            "in both directions when the "
                                            "flags are present.")

    @classmethod
    def memory_mode(cls):
        return 'timing'
