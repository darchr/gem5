from abc import abstractmethod
from math import (
    ceil,
    log,
    sqrt,
)

from m5.objects import (
    SimpleExtLink,
    SimpleIntLink,
    SimpleNetwork,
    Switch,
)

from gem5.utils.override import overrides


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

        # adding for dax support

    #     self.dax_ports = []

    # def get_dax_ports():
    #     return self.dax_ports

    # def append_dax_ports(new_port):
    #     self.dax_ports.append(new_port)

    def connect_hosts(self, hosts, system_caches):
        # create a router for each host that will connect to that hosts' L3 and othere hosts
        self.system_routers = [CHISwitch(self) for _ in range(len(hosts))]
        slice_links = []

        # Connect hosts to their respective routers
        for host, router in zip(hosts, self.system_routers):
            # connects the necessary routers within the host
            rs, els, ils = host.setup_network(self, router)
            self._routers.extend(rs)
            self._ext_links.extend(els)
            self._int_links.extend(ils)

        # Connect system caches (HNs) to the correct host router
        for cache_slice in system_caches:
            router = self.system_routers[cache_slice.host_id]
            router.ext_routing_latency = 1
            router.int_routing_latency = 1
            slice_links.append(
                ExtLink(cache_slice, router, bandwidth_factor=128)
            )

        self.slice_links = slice_links

        self._routers.extend(self.system_routers)
        self._ext_links.extend(self.slice_links)

        # self._routers.extend(self.system_routers)
        # self._ext_links.extend(self.slice_links)

    def connect_memory_controllers(self, memory_controllers):
        assert len(memory_controllers) > 0
        self.memory_routers = [
            CHISwitch(self) for _ in range(len(memory_controllers))
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
            CHISwitch(self) for _ in range(len(dma_controllers))
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
        for link in self.ext_links:
            print(
                f"Ext Link {link.link_id}: {link.ext_node} <-> {link.int_node}"
            )
        for link in self.int_links:
            print(
                f"Int Link {link.link_id}: {link.src_node} <-> {link.dst_node}"
            )


class MultiHostNetwork(BaseSystemNetwork):
    def __init__(self, ruby_system, vnets):
        super().__init__(ruby_system, vnets)

    def build_system_network(self, hosts):
        system_links = []

        # (optional) host-to-host mesh among system_routers
        for src in self.system_routers:
            for dst in self.system_routers:
                if src is dst:
                    continue
                system_links.append(IntLink(src, dst))

        # REQUIRED: connect every system router to every memory router, both ways
        for s in self.system_routers:
            for m in self.memory_routers:
                system_links.append(IntLink(s, m))
                system_links.append(IntLink(m, s))

        self.system_links = system_links
        self._int_links.extend(self.system_links)
        print(f"Number of System(host/memory) Links: {len(self.system_links)}")

    # # TODO: Need to implement this still
    # def build_system_network(self, hosts):
    #     # assert len(hosts) == len(system_caches)
    #     # self.host_routers = [CHISwitch(self) for _ in range(len(hosts))]
    #     system_links = []

    #     for src in self.system_routers:
    #         for dst in self.system_routers:
    #             if src == dst:
    #                 continue
    #             system_links.append(IntLink(src, dst))

    #     self.system_links = system_links
    #     self._int_links.extend(self.system_links)
    #     print(f"Number of System(host-to-host) Links:  {len(self.system_links)}")


class CHISwitch(Switch):
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
