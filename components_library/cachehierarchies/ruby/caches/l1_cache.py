from components_library.processors.abstract_core import AbstractCore
from components_library.isas import ISA
from .abstract_l1_cache import AbstractL1Cache

from m5.objects import (
    MessageBuffer,
    RubyPrefetcher,
    RubyCache,
    ClockDomain,
)

import math


class L1Cache(AbstractL1Cache):
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
        super(L1Cache, self).__init__(network, cache_line_size)

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
        self.send_evictions = self.sendEvicts(
            core=core, target_isa = target_isa)
        self.transitions_per_cycle = 4
        self.enable_prefetch = False

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
