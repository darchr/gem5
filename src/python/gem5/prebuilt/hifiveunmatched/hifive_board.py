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

from gem5.components.memory.dram_interfaces.ddr4 import DDR4_2400_8x8
from gem5.components.memory.memory import ChanneledMemory
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.utils.requires import requires
from gem5.isas import ISA
from gem5.components.boards.simple_board import SimpleBoard
from python.gem5.prebuilt.hifiveunmatched.hifive_cache import (
    HiFiveCacheHierarchy,
)
from python.gem5.prebuilt.hifiveunmatched.hifive_proc import U74Processor
from m5.objects import AddrRange

class HiFiveUnmatchedBoard(SimpleBoard):
    def __init__(self) -> None:
        requires(isa_required=ISA.RISCV)

        cache_hierarchy = HiFiveCacheHierarchy(
            l2_size="2MB"
        )

        memory = ChanneledMemory(DDR4_2400_8x8, 1, 64, "16GB")
        memory.set_memory_range(
            [AddrRange(start=0x80000000, size=memory.get_size())]
        )

        processor = U74Processor()

        super().__init__(
            clk_freq="1.2GHz",  # real system is 1.0 to 1.5 GHz
            processor=processor,
            memory=memory,
            cache_hierarchy=cache_hierarchy,
        )
