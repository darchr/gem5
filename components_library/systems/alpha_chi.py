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

"""
αχ: A simple x86 system.
"""

from components_library.motherboards.x86_motherboard import X86Motherboard

from components_library.cachehierarchies.private_l1_private_2_cache_hierarchy \
    import PrivateL1PrivateL2CacheHierarchy
from components_library.memory.ddr3_1600_8x8 import DDR3_1600_8x8
from components_library.processors.simple_processor import SimpleProcessor
from components_library.processors.cpu_types import CPUTypes

class AlphaChiSystem:

    def __init__(self, kernel_path: str, disk_path: str):

        self.cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
            l1d_size = "32kB",
            l1i_size = "32kB",
            l2_size = "256kB",
            inc_mmu = True,
        )

        # For an even simpler setup, have no cache at all!
        #cache_hierarchy = NoCache()

        self.memory = DDR3_1600_8x8(size="3GB")

        self.processor = SimpleProcessor(cpu_type = CPUTypes.TIMING,
            num_cores=1)

        self.motherboard = X86Motherboard(clk_freq="3GHz",
                                        processor=self.processor,
                                        memory=self.memory,
                                        cache_hierarchy=self.cache_hierarchy,
                                    )

        self.kernel_path = kernel_path
        self.disk_path = disk_path

    def set_workload_command(self, command: str):

        self.motherboard.set_workload(self.kernel_path, self.disk_path,
            command)
