# Copyright (c) 2021 The Regents of the University of California
# All Rights Reserved.
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

import math

from .....processors.cpu_types import CPUTypes
from .....processors.abstract_core import AbstractCore
from ......isas import ISA

from m5.objects import (
    MessageBuffer,
    MI_example_L1Cache_Controller,
    RubyCache,
    ClockDomain,
)


class L1Cache(MI_example_L1Cache_Controller):

    _version = 0

    @classmethod
    def versionCount(cls):
        cls._version += 1  # Use count for this particular type
        return cls._version - 1

    def __init__(
        self,
        size: str,
        assoc: int,
        network,
        core: AbstractCore,
        cache_line_size,
        target_isa: ISA,
        clk_domain: ClockDomain,
    ):
        super().__init__()
        self.version = self.versionCount()
        self._cache_line_size = cache_line_size
        self.connectQueues(network)

        self.cacheMemory = RubyCache(
            size=size, assoc=assoc, start_index_bit=self.getBlockSizeBits()
        )

        self.clk_domain = clk_domain
        self.send_evictions = self.sendEvicts(core=core, target_isa=target_isa)

    def getBlockSizeBits(self):
        bits = int(math.log(self._cache_line_size, 2))
        if 2 ** bits != self._cache_line_size.value:
            raise Exception("Cache line size not a power of 2!")
        return bits

    def sendEvicts(self, core: AbstractCore, target_isa: ISA):
        """True if the CPU model or ISA requires sending evictions from caches
        to the CPU. Two scenarios warrant forwarding evictions to the CPU:
        1. The O3 model must keep the LSQ coherent with the caches
        2. The x86 mwait instruction is built on top of coherence
        3. The local exclusive monitor in ARM systems
        """
        if core.get_type() is CPUTypes.O3 or target_isa in (ISA.X86, ISA.ARM):
            return True
        return False

    def connectQueues(self, network):
        self.mandatoryQueue = MessageBuffer()
        self.requestFromCache = MessageBuffer(ordered=True)
        self.requestFromCache.out_port = network.in_port
        self.responseFromCache = MessageBuffer(ordered=True)
        self.responseFromCache.out_port = network.in_port
        self.forwardToCache = MessageBuffer(ordered=True)
        self.forwardToCache.in_port = network.out_port
        self.responseToCache = MessageBuffer(ordered=True)
        self.responseToCache.in_port = network.out_port
