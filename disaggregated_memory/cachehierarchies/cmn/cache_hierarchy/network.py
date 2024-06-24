# Copyright (c) 2022 The Regents of the University of California
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

from abc import abstractmethod
from math import log, sqrt, ceil

from gem5.utils.override import overrides
from m5.objects import SimpleNetwork, Switch, SimpleExtLink, SimpleIntLink


class BaseSystemNetwork(SimpleNetwork):
    def __init__(self, ruby_system, vnets):
        super().__init__()
        self.netifs = []

        # TODO: These should be in a base class
        # https://gem5.atlassian.net/browse/GEM5-1039
        self.ruby_system = ruby_system

        self._routers = []
        self._ext_links = []
        self._int_links = []

        self._has_dma = False

        self.number_of_virtual_networks = vnets

    def connect_core_tiles(self, core_tiles, system_caches):
        assert len(core_tiles) == len(system_caches)
        self.system_routers = [CMNSwitch(self) for _ in range(len(core_tiles))]
        slice_links = []
        for tile, cache_slice, router in zip(
            core_tiles, system_caches, self.system_routers
        ):
            router.ext_routing_latency = 7
            router.int_routing_latency = 10
            slice_links.append(
                ExtLink(cache_slice, router, bandwidth_factor=128)
            )
            rs, els, ils = tile.setup_network(self, router)
            self._routers.extend(rs)
            self._ext_links.extend(els)
            self._int_links.extend(ils)
        self.slice_links = slice_links

        self._routers.extend(self.system_routers)
        self._ext_links.extend(self.slice_links)

    def connect_memory_controllers(self, memory_controllers):
        assert len(memory_controllers) > 0
        self.memory_routers = [
            CMNSwitch(self) for _ in range(len(memory_controllers))
        ]
        memory_links = []
        for ctrl, router in zip(memory_controllers, self.memory_routers):
            router.ext_routing_latency = 10
            router.int_routing_latency = 10
            memory_links.append(ExtLink(ctrl, router, bandwidth_factor=64))
        self.memory_links = memory_links

        self._routers.extend(self.memory_routers)
        self._ext_links.extend(self.memory_links)

    def connect_dma_controllers(self, dma_controllers):
        if not dma_controllers:
            return
        self.dma_routers = [
            CMNSwitch(self) for _ in range(len(dma_controllers))
        ]
        dma_links = []
        for ctrl, router in zip(dma_controllers, self.dma_routers):
            router.ext_routing_latency = 10
            router.int_routing_latency = 10
            dma_links.append(ExtLink(ctrl, router, bandwidth_factor=64))
        self.dma_links = dma_links

        self._routers.extend(self.dma_routers)
        self._ext_links.extend(self.dma_links)
        self._has_dma = True

    @abstractmethod
    def build_system_network(self):
        raise NotImplementedError

    def finalize(self):
        self.routers = self._routers
        self.ext_links = self._ext_links
        self.int_links = self._int_links


class Pt2PtSystemNetwork(BaseSystemNetwork):
    def _get_system_level_routers(self):
        assert len(self.system_routers) > 0
        assert len(self.memory_routers) > 0
        ret = self.system_routers + self.memory_routers
        if self._has_dma:
            assert len(self.dma_routers) > 0
            ret += self.dma_routers
        return ret

    @overrides(BaseSystemNetwork)
    def build_system_network(self):
        system_links = []
        system_level_routers = self._get_system_level_routers()
        for src in system_level_routers:
            for dst in system_level_routers:
                if src == dst:
                    continue
                system_links.append(IntLink(src, dst))
        self.system_links = system_links
        self._int_links.extend(self.system_links)


class MeshSystemNetwork(BaseSystemNetwork):
    def _get_dimensions(self, num_tiles):
        if log(num_tiles, 2) % 2 == 0:
            height = int(sqrt(num_tiles))
        else:
            height = int(sqrt(num_tiles // 2))
        width = num_tiles // height
        return width, height

    def _rotate(self, width, height, i, j):
        return (height - j - 1, i)

    def build_system_network(self):
        width, height = self._get_dimensions(len(self.system_routers))

        links = []
        for y in range(height):
            for x in range(width):
                if x > 0:
                    current_router = self.system_routers[y * width + x]
                    previous_router = self.system_routers[y * width + (x - 1)]
                    links.append(IntLink(current_router, previous_router))
                    links.append(IntLink(previous_router, current_router))
                if y > 0:
                    current_router = self.system_routers[y * width + x]
                    previous_router = self.system_routers[(y - 1) * width + x]
                    links.append(IntLink(current_router, previous_router))
                    links.append(IntLink(previous_router, current_router))

        if self._has_dma:
            routers = self.memory_routers + self.dma_routers
        else:
            routers = self.memory_routers

        i = 0
        j = ceil(height / 2) + 1
        for index, router in enumerate(routers):
            if index % 4 == 0:
                j -= 1
            if j < 0:
                j = height - 1
            system_router = self.system_routers[j * width + i]
            links.append(IntLink(router, system_router, bandwidth_factor=512))
            links.append(IntLink(system_router, router, bandwidth_factor=512))
            i, j = self._rotate(width, height, i, j)

        self.system_links = links
        self._int_links.extend(links)


class CMNSwitch(Switch):
    """Simple switch with auto counting for the id."""

    _version = 0

    @classmethod
    def version_count(cls):
        cls._version += 1  # Use count for this particular type
        return cls._version - 1

    def __init__(self, network):
        super().__init__()
        self.router_id = self.version_count()
        self.virt_nets = network.number_of_virtual_networks


class ExtLink(SimpleExtLink):
    """Simple ext link with auto counting for the id."""

    _version = 0

    @classmethod
    def version_count(cls):
        cls._version += 1  # Use count for this particular type
        return cls._version - 1

    def __init__(self, ext_node, int_node, bandwidth_factor=16):
        super().__init__()
        self.link_id = self.version_count()
        self.ext_node = ext_node
        self.int_node = int_node
        self.bandwidth_factor = bandwidth_factor


class IntLink(SimpleIntLink):
    """Simple int link with auto counting for the id."""

    _version = 0

    @classmethod
    def version_count(cls):
        cls._version += 1  # Use count for this particular type
        return cls._version - 1

    def __init__(self, src_node, dst_node, bandwidth_factor=16):
        super().__init__()
        self.link_id = self.version_count()
        self.src_node = src_node
        self.dst_node = dst_node
        self.bandwidth_factor = bandwidth_factor
