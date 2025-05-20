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
import math


class SagaNetwork(SimpleNetwork):
    """A network which resembles the hierarchy of an AMD Epyc

    This assumes that there are private L1 I/D with a private L2 in a cluster
    of 8 cores per L3. The directory is on a separate "I/O die" and is split
    one per memory controller.
    """

    def __init__(self, ruby_system, vnets,
                 racks_per_board,
                 chassis_per_rack,
                 cluster_per_chassis,
                 cores_per_cluster):
        super().__init__()
        self.netifs = []

        # TODO: These should be in a base class
        # https://gem5.atlassian.net/browse/GEM5-1039
        self.ruby_system = ruby_system
        self._racks_per_board = racks_per_board
        self._chassis_per_rack = chassis_per_rack
        self._cluster_per_chassis = cluster_per_chassis
        self._cores_per_cluster = cores_per_cluster

        self._routers = []
        self._ext_links = []
        self._int_links = []

        self.number_of_virtual_networks = vnets

        self._L1_router = SagaSwitch(self)
        self._routers.append(self._L1_router)

        self._L2_routers = []
        for _ in range(self._racks_per_board // 2):
            l2_router = SagaSwitch(self)
            self._L2_routers.append(l2_router)
            self._routers.append(l2_router)
                
        self._TOR_routers = []
        for _ in range(self._racks_per_board):
            tor_router = SagaSwitch(self)
            self._TOR_routers.append(tor_router)
            self._routers.append(tor_router)

        self._dir_routers = []
        for _ in range(self._chassis_per_rack*self._racks_per_board):
            dir_router = SagaSwitch(self)
            self._dir_routers.append(dir_router)
            self._routers.append(dir_router)

    def connect_ccx(self, core_complexes): 
        tot_chassis_num = self._racks_per_board * self._chassis_per_rack
        for i, ccx in enumerate(core_complexes):
            chassis_index = i // (len(core_complexes) // tot_chassis_num)
            print(f"Connecting CCX {i} to chassis index {chassis_index}")
            rs, els, ils = ccx.setup_network(self._dir_routers[chassis_index], self)
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
        tot_chassis_num = self._racks_per_board * self._chassis_per_rack
        if tot_chassis_num > len(memory_controllers):
            raise ValueError("tot_chassis_num is greater than available memory controllers, causing division error.")

        md_links = []
        dm_links = []

        ## real workloads in ARM board
        for i, router in enumerate(self.mem_routers):
            if i == 0: # Skip the first one in ARM board, is for kernel/io
                continue
            else : 
                chassis_index = (i-1) // (len(self.mem_routers) // tot_chassis_num)
                assert router is not None, f"Router at index {i} is None"
                assert chassis_index < len(self._dir_routers), f"Invalid chassis index {chassis_index}"
                assert self._dir_routers[chassis_index] is not None, f"Dir router at {chassis_index} is None"
                md_link = SagaIntLink(router, self._dir_routers[chassis_index], bandwidth_factor=64)
                dm_link = SagaIntLink(self._dir_routers[chassis_index], router, bandwidth_factor=64)
                md_links.append(md_link)
                dm_links.append(dm_link)
        self.md_links = md_links
        self.dm_links = dm_links
        self._int_links.extend(md_links)
        self._int_links.extend(dm_links)

        ## traffic generator
        # for i, router in enumerate(self.mem_routers):
        #     chassis_index = (i) // (len(self.mem_routers) // tot_chassis_num)
        #     assert router is not None, f"Router at index {i} is None"
        #     assert chassis_index < len(self._dir_routers), f"Invalid chassis index {chassis_index}"
        #     assert self._dir_routers[chassis_index] is not None, f"Dir router at {chassis_index} is None"
        #     md_link = SagaIntLink(router, self._dir_routers[chassis_index], bandwidth_factor=64)
        #     dm_link = SagaIntLink(self._dir_routers[chassis_index], router, bandwidth_factor=64)
        #     md_links.append(md_link)
        #     dm_links.append(dm_link)
        # self.md_links = md_links
        # self.dm_links = dm_links
        # self._int_links.extend(md_links)
        # self._int_links.extend(dm_links)
        


    def connect_dma(self, dma_ctrls):
        if not dma_ctrls:
            return
        dma_links = []
        for ctrl in dma_ctrls:
            dma_links.append(SagaExtLink(ctrl, self._dir_routers[0]))
        self.dma_links = dma_links
        self._ext_links.extend(dma_links)    

    def finalize(self):
        ## Connect the TOR routers to the directory routers
        chassis_tor_links = []
        tor_chassis_links = []
        for i, dir_router in enumerate(self._dir_routers):
            rack_index = i // self._chassis_per_rack
            print(f"{len(self._TOR_routers)}, {self._racks_per_board}, {len(self._dir_routers)}: Connecting directory router {i} to TOR router at rack index {rack_index}")
            chassis_tor_links.append(SagaIntLink(dir_router, self._TOR_routers[rack_index]))
            tor_chassis_links.append(SagaIntLink(self._TOR_routers[rack_index], dir_router))
        self.chassis_tor_links = chassis_tor_links
        self.tor_chassis_links = tor_chassis_links
        self._int_links.extend(chassis_tor_links)
        self._int_links.extend(tor_chassis_links)

        ## Connect the TOR routers to the L2 routers
        tor_l2_links = []
        l2_tor_links = []
        for i, tor_router in enumerate(self._TOR_routers):
            l2_router_index = i // len(self._L2_routers)
            tor_l2_links.append(SagaIntLink(tor_router, self._L2_routers[l2_router_index]))
            l2_tor_links.append(SagaIntLink(self._L2_routers[l2_router_index], tor_router))
        self.tor_l2_links = tor_l2_links
        self.l2_tor_links = l2_tor_links
        self._int_links.extend(tor_l2_links)
        self._int_links.extend(l2_tor_links)

        ## Connect the L2 routers to the L1 routers
        ## all to all connections
        l2_l1_links = []
        l1_l2_links = []
        for l2_router in (self._L2_routers):
            l2_l1_links.append(SagaIntLink(l2_router, self._L1_router))
            l1_l2_links.append(SagaIntLink(self._L1_router, l2_router))
        self.l2_l1_links = l2_l1_links
        self.l1_l2_links = l1_l2_links
        self._int_links.extend(l2_l1_links)
        self._int_links.extend(l1_l2_links)

        self.routers = self._routers
        self.ext_links = self._ext_links
        self.int_links = self._int_links
        
        print("Network finalized with the following components:")
        print(f"Total memory routers: {len(self.mem_routers)}")
        # for router in self._routers:
        #     print(f"Router name: {router._name}")

        # # Print names of all external links
        # for ext_link in self._ext_links:
        #     print(f"External link name: {ext_link._name}")

        # # Print names of all internal links
        # for int_link in self._int_links:
        #     print(f"Internal link name: {int_link._name}")
 

        # # for i, router in enumerate(self.routers):
        # #     print(f"Router {i}: {router}")
        # # for i, link in enumerate(self.int_links):
        # #     print(f"Internal Link {i}: {link}")
        # # for i, link in enumerate(self.ext_links):
        # #     print(f"External Link {i}: {link}")

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
