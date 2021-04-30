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
from ..boards.abstract_board import AbstractBoard

from m5.objects import  BaseCPU, BaseXBar, SystemXBar, BadAddr
from m5.params import Port

from typing import Tuple, Optional

from ..utils.override import *

class NoCache(AbstractCacheHierarchy):
    '''
    No cache hierarchy. The CPUs are connected straight to the memory bus.

    By default a SystemXBar of width 64bit is used, though this can be
    configured via the constructor.

    NOTE: At present this does not work with FS. The following error is
    received:

    ```
    ...
    build/X86/mem/snoop_filter.cc:277: panic: panic condition
    (sf_item.requested & req_mask).none() occurred: SF value
    0000000000000000000000000000000000000000000000000000000000000000 ...
    missing the original request
    Memory Usage: 3554472 KBytes
    Program aborted at tick 1668400099164
    --- BEGIN LIBC BACKTRACE ---
    ...
    ```
    '''

    @staticmethod
    def _get_default_membus() -> SystemXBar:
        """
        A method used to obtain the default memory bus of 64 bit in width for
        the NoCache CacheHierarchy.

        :returns: The default memory bus for the NoCache CacheHierarchy.

        :rtype: SystemXBar
        """
        membus = SystemXBar(width = 64)
        membus.badaddr_responder = BadAddr()
        membus.default = membus.badaddr_responder.pio
        return membus

    def __init__(self,
                 membus: Optional[BaseXBar] = \
                     _get_default_membus.__func__()) -> None:
        """
        :param membus: The memory bus for this setup. This parameter is
        optional and will default toa 64 bit width SystemXBar is not specified.

        :type membus: Optional[BaseXBar]
        """
        super(NoCache, self).__init__()
        self.membus = membus


    @overrides(AbstractCacheHierarchy)
    def incorporate_cache(self, board: AbstractBoard) -> None:

        for cpu in board.get_processor().get_cpu_simobjects():
            cpu.icache_port = self.get_membus().cpu_side_ports
            cpu.dcache_port = self.get_membus().cpu_side_ports
            cpu.mmu.connectWalkerPorts(
                self.get_membus().cpu_side_ports,
                self.get_membus().cpu_side_ports
            )
        # Set up the system port for functional access from the simulator.
        board.system_port = self.get_membus().cpu_side_ports

    @overrides(AbstractCacheHierarchy)
    def get_interrupt_ports(self, cpu: BaseCPU) -> Tuple[Port, Port]:
        return self.get_membus().mem_side_ports, \
               self.get_membus().cpu_side_ports

    @overrides(AbstractCacheHierarchy)
    def get_membus(self) -> BaseXBar:
        return self.membus
