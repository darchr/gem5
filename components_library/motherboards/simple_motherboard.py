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

from components_library.processors.cpu_types import CPUTypes
import m5
from m5.objects import SystemXBar, AddrRange, SrcClockDomain, VoltageDomain,\
                       Addr, Process, SEWorkload

from .abstract_motherboard import AbstractMotherboard
from ..processors.abstract_processor import AbstractProcessor
from ..memory.abstract_memory import AbstractMemory
from ..cachehierarchies.abstract_classic_cache_hierarchy import \
                                    AbstractClassicCacheHierarchy


from typing import List, Optional


class SimpleMotherboard(AbstractMotherboard):
    """
    This is an incredibly simple system. It contains no I/O, and will work to
    work with a classic cache hierarchy setup.

    You can run a bare-metal executable via the `set_workload` function (SE
    mode only!).
    """
    def __init__(self, clk_freq: str,
                 processor: AbstractProcessor,
                 memory: AbstractMemory,
                 cache_hierarchy: AbstractClassicCacheHierarchy,
                 xbar_width: Optional[int] = 64,
                ) -> None:
        super(SimpleMotherboard, self).__init__(
                                        processor=processor,
                                        memory=memory,
                                        cache_hierarchy=cache_hierarchy,
                                        membus=SystemXBar(width = xbar_width)
                                            )

        # TODO: On second thought, a lot of thise could be put safely in the
        # AbstractMotherboard base class.

        # Set up the clock domain and the voltage domain.
        self.get_system_simobject().clk_domain = SrcClockDomain()
        self.get_system_simobject().clk_domain.clock = clk_freq
        self.get_system_simobject().clk_domain.voltage_domain = VoltageDomain()

        # Set the memory mode.
        if self.get_processor().get_cpu_type() == CPUTypes.TIMING \
            or self.get_processor().get_cpu_type() == CPUTypes.O3 :
            self.get_system_simobject().mem_mode = 'timing'
        elif self.get_processor().get_cpu_type() == CPUTypes.KVM :
            self.get_system_simobject().mem_mode = "atomic_noncaching"
        elif self.get_processor().get_cpu_type() == CPUTypes.ATOMIC :
            self.get_system_simobject().mem_mode = "atomic"
        else:
            raise NotImplementedError

        self.get_system_simobject().mem_ranges = \
            [AddrRange(Addr(memory.get_size_str()))]

        # Incorporate the cache heirarchy for the motherboard.
        self.get_cache_hierarchy().incorporate_cache(self)

        # Incorporate the processor into the motherboard.
        self.get_processor().incorporate_processor(self)

        # Incorporate the memory into the motherboard.
        self.get_memory().incorporate_memory(self)

    def set_workload(self, binary:str) ->None:
        # This is very limited, single binary, putting on a single Core.

        self.get_system_simobject().workload = \
            SEWorkload.init_compatible(binary)

        process = Process()
        process.cmd = [binary]
        self.get_processor().get_cpu_simobjects()[0].workload = process
        self.get_processor().get_cpu_simobjects()[0].createThreads()
