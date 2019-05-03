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
from m5.params import *
from m5.proxy import *

class FlexCPU(BaseCPU):
    type = 'FlexCPU'
    cxx_header = 'cpu/flexcpu/flexcpu.hh'

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

    instruction_buffer_size = Param.Unsigned(0, "Size of the dynamic "
                                             "instruction buffer. This buffer "
                                             "is used for maintaining the "
                                             "commit order of instructions. "
                                             "Limiting this limits the number "
                                             "of instructions that can be "
                                             "handled at once before commit "
                                             "(out of order). 0 implies an "
                                             "infinitely large buffer.")

    issue_latency = Param.Cycles(0, "Number of cycles each instruction takes "
                                    "to issue.")
    issue_bandwidth = Param.Int(0, "Number of instructions/micro-ops that can "
                                   "be issued each cycle.")


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
