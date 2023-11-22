# Copyright (c) 2023 The Regents of the University of California
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

import m5
from m5.util import fatal
from m5.objects.XBar import NoncoherentXBar
from m5.objects import OutgoingRequestBridge, AddrRange, Tick


class ExternalRemoteMemoryInterface:
    def __init__(
        self,
        size: "str" = None,
        addr_range: AddrRange = None,
        remote_memory_latency: Tick = None,
    ):
        # We will create a non-coherent cross bar if the user wants to simulate
        # latency for the remote memory links.
        self._xbar_required = False
        # We setup the remote memory with size or address range. This allows us
        # to quickly scale the setup with N nodes.
        self._size = None
        self._set_using_addr_ranges = False
        self.remote_memory = OutgoingRequestBridge()
        # The user needs to provide either the size of the remote memory or the
        # range of the remote memory.
        if size is None and addr_range is None:
            fatal("External memory needs to either have a size or a range!")
        else:
            if addr_range is not None:
                self.remote_memory.physical_address_ranges = [addr_range]
                self._size = self.remote_memory.physical_address_ranges[
                    0
                ].size()
                self._set_using_addr_ranges = True
            # The size will be setup in the board in case ranges are not given
            # by the user.
            else:
                self._size = size

        # If there is a remote latency specified, create a non_coherent
        # cross_bar.
        if remote_memory_latency is not None:
            self._xbar_required = True
            self._remote_memory_latency = remote_memory_latency

    def get_size(self):
        return self._size

    # def set_size(self):
    #     self._size = self.remote_memory.physical_addr_ranges[0].size()

    def is_xbar_required(self):
        # If an XBar is required, it should be added in the connect_things to
        # avoid initializing an orphan node.
        return self._xbar_required

    def get_set_using_addr_ranges(self):
        return self._set_using_addr_ranges
