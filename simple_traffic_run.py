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

import m5

from m5.objects import Root

from components_library.boards.simple_board import SimpleBoard
from components_library.cachehierarchies.no_cache import NoCache
from components_library.memory.ddr3_1600_8x8 import DDR3_1600_8x8
from components_library.processors.linear_generator import LinearGenerator
from components_library.processors.random_generator import RandomGenerator
from components_library.processors.complex_generator import ComplexGenerator

cache_hierarchy = NoCache()

memory = DDR3_1600_8x8(size="512MiB")

lingen = LinearGenerator(num_cores=1, rate="100GB/s")
rndgen = RandomGenerator(num_cores=2, block_size = 32, rate = "50")

cmxgen = ComplexGenerator(num_cores=1)
cmxgen.add_linear(rate="100GB/s")
cmxgen.add_random(block_size = 32, rate = "50MB/s")

motherboard = SimpleBoard(
    clk_freq="3GHz",
    processor=cmxgen,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

motherboard.connect_things()


root = Root(full_system=False, system=motherboard)

m5.instantiate()


cmxgen.start_traffic()
print("Beginning simulation!")
exit_event = m5.simulate()
print("Exiting @ tick %i because %s" % (m5.curTick(), exit_event.getCause()))
cmxgen.start_traffic()
print("Resuming simulation!")
exit_event = m5.simulate()
print("Exiting @ tick %i because %s" % (m5.curTick(), exit_event.getCause()))
