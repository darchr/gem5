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

from typing import (
    List,
    Optional,
)

from m5.objects import (
    AddrRange,
    IOXBar,
    Port,
    RubySystem,
)

from ...utils.override import overrides
from ..cachehierarchies.abstract_cache_hierarchy import AbstractCacheHierarchy
from ..memory.abstract_memory_system import AbstractMemorySystem
from ..memory.cxl_memory import CXLMemory
from ..processors.abstract_generator import AbstractGenerator
from .abstract_board import AbstractBoard
from .abstract_system_board import AbstractSystemBoard
from .test_board import TestBoard


class CXLTestBoard(TestBoard):
    """This is a Testing Board used to run traffic generators on a simple
    architecture.

    To work as a traffic generator board, pass a generator as a processor.

    This board does not require a cache hierarchy (it can be ``none``) in which
    case the processor (generator) will be directly connected to the memory.
    The clock frequency is only used if there is a cache hierarchy or when
    using the GUPS generators.
    """

    def __init__(
        self,
        clk_freq: str,
        generator: AbstractGenerator,
        cache_hierarchy: Optional[AbstractCacheHierarchy],
        cxl_memory: CXLMemory,
    ):

        super().__init__(
            clk_freq=clk_freq,  # Only used if cache hierarchy or GUPS-gen
            generator=generator,
            memory=cxl_memory,
            cache_hierarchy=cache_hierarchy,
        )
