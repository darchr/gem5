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

""" Channeled "generic" DDR memory controllers
"""

import m5
from gem5.utils.override import overrides
from m5.objects import AddrRange, DRAMInterface, MemCtrl, Port
from m5.objects.XBar import NoncoherentXBar
from typing import Type, Sequence, Tuple, Optional, Union

from gem5.components.memory.memory import ChanneledMemory


class RemoteChanneledMemory(ChanneledMemory):
    def __init__(
        self,
        dram_interface_class: Type[DRAMInterface],
        num_channels: Union[int, str],
        interleaving_size: Union[int, str],
        size: Optional[str] = None,
        addr_mapping: Optional[str] = None,
        remote_offset_latency: Union[int, str] = 0,
    ) -> None:
        self._remote_latency = remote_offset_latency
        super().__init__(
            dram_interface_class,
            num_channels,
            interleaving_size,
            size,
            addr_mapping,
        )

    @overrides(ChanneledMemory)
    def _create_mem_interfaces_controller(self):
        self._dram = [
            self._dram_class(addr_mapping=self._addr_mapping)
            for _ in range(self._num_channels)
        ]
        self.remote_links = [
            NoncoherentXBar(
                frontend_latency=self._remote_latency,
                forward_latency=0,
                response_latency=0,
                width=8,
            )
            for _ in range(self._num_channels)
        ]
        self.mem_ctrl = [
            MemCtrl(
                dram=self._dram[i], port=self.remote_links[i].mem_side_ports
            )
            for i in range(self._num_channels)
        ]

    @overrides(ChanneledMemory)
    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return [
            (self.mem_ctrl[i].dram.range, self.remote_links[i].cpu_side_ports)
            for i in range(self._num_channels)
        ]

    @overrides(ChanneledMemory)
    def get_memory_controllers(self):
        return [
            (self.remote_links[i].cpu_side_ports)
            for i in range(self._num_channels)
        ]
