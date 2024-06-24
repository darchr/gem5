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

from m5.objects import SimpleNetwork, Switch, SimpleExtLink, SimpleIntLink


class SagaNetwork(SimpleNetwork):
    """A network which resembles the hierarchy of an AMD Epyc

    This assumes that there are private L1 I/D with a private L2 in a cluster
    of 8 cores per L3. The directory is on a separate "I/O die" and is split
    one per memory controller.
    """

    def __init__(self, ruby_system, vnets):
        super().__init__()
        self.netifs = []

        # TODO: These should be in a base class
        # https://gem5.atlassian.net/browse/GEM5-1039
        self.ruby_system = ruby_system

        self._routers = []
        self._ext_links = []
        self._int_links = []

        self.number_of_virtual_networks = vnets

        self.dir_router = SagaSwitch(self)  # The xbar on the I/O Chip
        self._routers.append(self.dir_router)

    def connect_ccx(self, core_complexes):
        for ccx in core_complexes:
            rs, els, ils = ccx.setup_network(self.dir_router, self)
            self._routers.extend(rs)
            self._ext_links.extend(els)
            self._int_links.extend(ils)

    def connect_directory_memory(self, directories, memory_controllers):
        self.mem_routers = [SagaSwitch(self) for _ in memory_controllers]
        self._routers.extend(self.mem_routers)

        # connect memory and directory to the memory router
        for directory, ctrl, router in zip(
            directories, memory_controllers, self.mem_routers
        ):
            router.dm_link = SagaExtLink(
                directory, router, bandwidth_factor=32
            )
            router.md_link = SagaExtLink(ctrl, router, bandwidth_factor=32)
            self._ext_links.extend([router.dm_link, router.md_link])

        # Connect the memory routers to the dir_router
        self.dir_links = [
            SagaIntLink(router, self.dir_router, bandwidth_factor=64)
            for router in self.mem_routers
        ] + [
            SagaIntLink(self.dir_router, router, bandwidth_factor=64)
            for router in self.mem_routers
        ]
        self._int_links.extend(self.dir_links)

    def connect_dma(self, dma_ctrls):
        if not dma_ctrls:
            return
        self.dma_links = [
            SagaExtLink(ctrl, self.dir_router) for ctrl in dma_ctrls
        ]
        self._ext_links.extend(self.dma_links)

    def finalize(self):
        self.routers = self._routers
        self.ext_links = self._ext_links
        self.int_links = self._int_links


class SagaSwitch(Switch):
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


class SagaExtLink(SimpleExtLink):
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


class SagaIntLink(SimpleIntLink):
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
