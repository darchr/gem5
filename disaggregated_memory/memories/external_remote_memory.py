# Copyright (c) 2023-24 The Regents of the University of California
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

"""We need a class that extends the outgoing bridge from gem5. The goal
of this class to have a MemInterface like class in the future, where we'll
append mem_ranges within this interface."""

from typing import (
    List,
    Sequence,
    Tuple,
)

import m5
from m5.objects import (
    AddrRange,
    MemCtrl,
    OutgoingRequestBridge,
    Port,
    Tick,
)
from m5.util import fatal

from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.memory.memory import AbstractMemorySystem
from gem5.utils.override import overrides


class ExternalRemoteMemory(AbstractMemorySystem):
    """ExternalRemoteMemory is an AbstractMemorySystem in gem5 that allows SST
    to be interfaced as a component in the gem5's stdlib.
    """

    def __init__(
        self,
        size: "str" = None,
        addr_range: AddrRange = None,
        link_latency: Tick = None,
    ):
        """This class has to be initialized using either size or memory ranges.

        Args:
            size (str, optional): Size. Defaults to None.
            addr_range (AddrRange, optional): Address Range. Defaults to None.
            link_latency (Tick, optional): Additional latency. Defaults to None
        """
        super().__init__()

        # We will create a non-coherent cross bar if the user wants to simulate
        # latency for the remote memory links.
        self._xbar_required = False

        # We setup the remote memory with size or address range. This allows us
        # to quickly scale the setup with N nodes.
        self._size = None

        # We will either use size or addr range. This variable is used to keep
        # a track of that.
        self._set_using_addr_ranges = False

        # The outtgoingrequestbridge is an AbstractMemory object that connects
        # gem5 to SST as an external memory.
        self._outgoing_req_bridge = OutgoingRequestBridge()

        # The outgoingrequestbridge;s range should not appear in the addr_map.
        # TODO: The range and physical_address_ranges should have the same name
        # to avoid confusion.
        self._outgoing_req_bridge.in_addr_map = False

        # The user needs to provide either the size of the remote memory or the
        # range of the remote memory.
        if size is None and addr_range is None:
            fatal("External memory needs to either have a size or a range!")
        else:
            if addr_range is not None:
                self._outgoing_req_bridge.physical_address_ranges = [
                    addr_range
                ]
                self._size = self._outgoing_req_bridge.physical_address_ranges[
                    0
                ].size()
                self._set_using_addr_ranges = True
            # The size will be setup in the board in case ranges are not given
            # by the user.
            else:
                self._size = size

        # If there is a remote latency specified, create a non_coherent
        # cross_bar.
        if link_latency > 0:
            self._xbar_required = True
            self._link_latency = link_latency

    def get_size(self):
        return self._size

    def is_xbar_required(self):
        # If an XBar is required, it should be added in the connect_things to
        # avoid initializing an orphan node.
        return self._xbar_required

    def get_link_latency(self):
        assert self._xbar_required == True
        return self._link_latency

    def get_set_using_addr_ranges(self):
        return self._set_using_addr_ranges

    @overrides(AbstractMemorySystem)
    def incorporate_memory(self, board: AbstractBoard) -> None:
        # Since the External memory is similar to SimpleMemory in the stdlib,
        # we do not have anything in particular to setup.
        pass

    @overrides(AbstractMemorySystem)
    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        # raise NotImplementedError("This method is not implemented yet.")
        return [
            (self._outgoing_req_bridge.range, self._outgoing_req_bridge.port)
        ]

    @overrides(AbstractMemorySystem)
    def get_memory_controllers(self) -> List[MemCtrl]:
        return [self._outgoing_req_bridge]

    @overrides(AbstractMemorySystem)
    def get_size(self) -> int:
        return self._size

    @overrides(AbstractMemorySystem)
    def set_memory_range(self, ranges: List[AddrRange]) -> None:
        if len(ranges) != 1 or ranges[0].size() != self._size:
            raise Exception(
                "Simple single channel memory controller requires a single "
                "range which matches the memory's size."
            )
        self._outgoing_req_bridge.range = ranges[0]

        # We use two additional variables to restore checkpoints
        self._outgoing_req_bridge.start_range = ranges[0].start
        self._outgoing_req_bridge.end_range = ranges[0].end
