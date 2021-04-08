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
βχ: Another simple x86 system.

This system uses the two level MOESI_hammer cache hierarchy.

It has 4 cores, 3GHz frequency.

It exposes the L2 size and the coherence mechanisms as parameters.
"""

from ..cachehierarchies.abstract_cache_hierarchy import AbstractCacheHierarchy
from ..motherboards.x86_motherboard import X86Motherboard

from ..cachehierarchies.moesi_hammer \
    import MOESIHammerCacheHierarchy
from ..memory.ddr3_1600_8x8 import DDR3_1600_8x8
from ..processors import SingleCycleProcessor

class BetaChiSystem:
    """
    This is a long description of this system.
    """

    cache_hierarchy = MOESIHammerCacheHierarchy(
        l1d_size = "32kB",
        l1i_size = "32kB",
        l2_size = "256kB",
    )
    memory = DDR3_1600_8x8(size="3GB")
    processor = SingleCycleProcessor(num_cores=4)

    def __init__(self, l2_size: str):
        self.cache_hierarchy.l2_size = l2_size

        self.motherboard = X86Motherboard(clk_freq="3GHz",
                                        processor=self.processor,
                                        memory=self.memory,
                                        cache_hierarchy=self.cache_hierarchy,
                                    )

        super(BetaChiSystem).__init__(self)

class AlphaBetaChiSystem:
    """
    This is a description. This system is the same as the BetaChi system
    except that it is a broadcast-based design.
    """

    cache_hierarchy = MOESIHammerCacheHierarchy(
        l1d_size = "32kB",
        l1i_size = "32kB",
        l2_size = "256kB",
        broadcast = True
    )

class BetaBetaChiSystem:
    """
    This is a description. This system is the same as the BetaChi system
    except that it is a probe filter design.
    """

    cache_hierarchy = MOESIHammerCacheHierarchy(
        l1d_size = "32kB",
        l1i_size = "32kB",
        l2_size = "256kB",
        broadcast = False,
        probe_filter_size = "2MB",
    )

    def __init__(self, l2_size: str, probe_filter_reach: str):
        self.cache_hierarchy.probe_filter_size = probe_filter_reach
        super(BetaBetaChiSystem).__init__(self, l2_size)

class GammaBetaChiSystem:
    """
    This is a description. This system is the same as the BetaChi system
    except that it has a full directory.
    """

    cache_hierarchy = MOESIHammerCacheHierarchy(
        l1d_size = "32kB",
        l1i_size = "32kB",
        l2_size = "256kB",
        broadcast = False,
        probe_filter_size = "2MB",
        full_directory = True
    )

    def __init__(self, l2_size: str, probe_filter_reach: str):
        self.cache_hierarchy.probe_filter_size = probe_filter_reach
        super(BetaBetaChiSystem).__init__(self, l2_size)


