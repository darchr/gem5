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

from .abstract_cache_hierarchy import AbstractCacheHierarchy
from ..motherboards.abstract_motherboard import AbstractMotherboard

from m5.objects import L2XBar

class NoCache(AbstractCacheHierarchy):
    '''
    No cache hierarchy. The CPUs are connected straight to the memory bus.
    '''

    def __init__(self) -> None:
        super(NoCache, self).__init__()

    def incorporate_cache(self, motherboard: AbstractMotherboard) -> None:
        for cpu in motherboard.get_processor().get_cpu_simobjects():
            cpu.icache_port = motherboard.get_membus().cpu_side_ports
            cpu.dcache_port = motherboard.get_membus().cpu_side_ports
            cpu.mmu.connectWalkerPorts(
                motherboard.get_membus().cpu_side_ports,
                motherboard.get_membus().cpu_side_ports
            )
