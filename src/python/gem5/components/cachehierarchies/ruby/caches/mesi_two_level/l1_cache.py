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

from .....processors.abstract_core import AbstractCore
from .....processors.cpu_types import CPUTypes
from ......isas import ISA

from m5.objects import (
    MESI_Two_Level_L1Cache_Controller,
    MessageBuffer,
    RubyPrefetcher,
    RubyCache,
    ClockDomain,
)

import math


class L1Cache(MESI_Two_Level_L1Cache_Controller):

    _version = 0

    @classmethod
    def versionCount(cls):
        cls._version += 1  # Use count for this particular type
        return cls._version - 1

    def __init__(
        self,
        l1i_size,
        l1i_assoc,
        l1d_size,
        l1d_assoc,
        network,
        core: AbstractCore,
        num_l2Caches,
        cache_line_size,
        target_isa: ISA,
        clk_domain: ClockDomain,
    ):
        """Creating L1 cache controller. Consist of both instruction
        and data cache.
        """
        super().__init__()

        self.version = self.versionCount()
        self._cache_line_size = cache_line_size
        self.connectQueues(network)

        # This is the cache memory object that stores the cache data and tags
        self.L1Icache = RubyCache(
            size=l1i_size,
            assoc=l1i_assoc,
            start_index_bit=self.getBlockSizeBits(),
            is_icache=True,
        )
        self.L1Dcache = RubyCache(
            size=l1d_size,
            assoc=l1d_assoc,
            start_index_bit=self.getBlockSizeBits(),
            is_icache=False,
        )
        self.l2_select_num_bits = int(math.log(num_l2Caches, 2))
        self.clk_domain = clk_domain
        self.prefetcher = RubyPrefetcher()
        self.send_evictions = self.sendEvicts(core=core, target_isa=target_isa)
        self.transitions_per_cycle = 4
        self.enable_prefetch = False

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
        self.requestFromL1Cache = MessageBuffer()
        self.requestFromL1Cache.out_port = network.in_port
        self.responseFromL1Cache = MessageBuffer()
        self.responseFromL1Cache.out_port = network.in_port
        self.unblockFromL1Cache = MessageBuffer()
        self.unblockFromL1Cache.out_port = network.in_port

        self.optionalQueue = MessageBuffer()

        self.requestToL1Cache = MessageBuffer()
        self.requestToL1Cache.in_port = network.out_port
        self.responseToL1Cache = MessageBuffer()
        self.responseToL1Cache.in_port = network.out_port
