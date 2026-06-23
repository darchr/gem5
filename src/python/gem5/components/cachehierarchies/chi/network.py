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

    def connect_hosts(self, compute_hosts, system_caches, hosts_wrapper):
        # create a router for each host (compute AND 0-core)
        self.system_routers = [
            CHISwitch(self) for _ in range(hosts_wrapper.num_hosts)
        ]
        slice_links = []

        # Connect compute hosts to their respective routers
        for host in compute_hosts:
            router = self.system_routers[host._host_id]
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
    def __init__(
        self,
        ruby_system,
        vnets,
        hosts_wrapper,
        topology="default",
        num_stars=1,
        star_switch_latency=10,
        star_link_bandwidth=16,
    ):
        super().__init__(ruby_system, vnets)
        self._hosts_wrapper = hosts_wrapper
        self._topology = topology
        self._num_stars = num_stars
        self._star_switch_latency = star_switch_latency
        self._star_link_bandwidth = star_link_bandwidth

    def build_system_network(self, compute_hosts):
        if self._topology == "multi-star":
            self._build_multi_star_network(compute_hosts)
        else:
            self._build_default_network(compute_hosts)

    def _build_default_network(self, compute_hosts):
        """Default fully-connected mesh: all system routers connected
        to each other and to all memory routers."""
        system_links = []

        # host-to-host mesh among system_routers
        for src in self.system_routers:
            for dst in self.system_routers:
                if src is dst:
                    continue
                system_links.append(IntLink(src, dst))

        # connect every system router to every memory router, both ways
        for s in self.system_routers:
            for m in self.memory_routers:
                system_links.append(IntLink(s, m))
                system_links.append(IntLink(m, s))

        self.system_links = system_links
        self._int_links.extend(self.system_links)
        print(
            f"[default] Number of System(host/memory) Links: "
            f"{len(self.system_links)}"
        )

    def _build_multi_star_network(self, compute_hosts):
        """Multi-Star topology (CXL 3.0 Unified Switch Model):
        We use the 0-core hosts (memory pools) as the central star switches.
        All compute host system_routers connect ONLY through these memory pool routers.
        """
        system_links = []

        # Identify star routers (routers of 0-core hosts)
        pool_hosts = self._hosts_wrapper.memory_pools
        if not pool_hosts:
            print(
                "WARNING: No 0-core memory pool hosts found! Falling back to compute hosts as stars."
            )
            self._star_routers = self.system_routers[: self._num_stars]
        else:
            self._star_routers = [
                self.system_routers[p.host_id] for p in pool_hosts
            ]

        # The star routers are already in self._routers via connect_hosts
        # We just adjust their latencies
        for star in self._star_routers:
            star.int_routing_latency = self._star_switch_latency
            star.ext_routing_latency = self._star_switch_latency

        compute_host_routers = [
            self.system_routers[c.host_id]
            for c in self._hosts_wrapper.compute_hosts
        ]

        # Connect every compute host router to every star, bidirectionally
        for i, s in enumerate(compute_host_routers):
            preferred_star_idx = i % len(self._star_routers)
            for j, star in enumerate(self._star_routers):
                link_weight = 1 if j == preferred_star_idx else 10
                system_links.append(
                    IntLink(
                        s,
                        star,
                        bandwidth_factor=self._star_link_bandwidth,
                        weight=link_weight,
                    )
                )
                system_links.append(
                    IntLink(
                        star,
                        s,
                        bandwidth_factor=self._star_link_bandwidth,
                        weight=link_weight,
                    )
                )

        # Connect every memory_router to every star, bidirectionally
        for m in self.memory_routers:
            for star in self._star_routers:
                system_links.append(
                    IntLink(
                        m,
                        star,
                        bandwidth_factor=self._star_link_bandwidth,
                        weight=1,
                    )
                )
                system_links.append(
                    IntLink(
                        star,
                        m,
                        bandwidth_factor=self._star_link_bandwidth,
                        weight=1,
                    )
                )

        # Connect DMA routers to every star, bidirectionally
        if self._has_dma:
            for i, d in enumerate(self.dma_routers):
                preferred_star_idx = i % len(self._star_routers)
                for j, star in enumerate(self._star_routers):
                    link_weight = 1 if j == preferred_star_idx else 10
                    system_links.append(
                        IntLink(
                            d,
                            star,
                            bandwidth_factor=self._star_link_bandwidth,
                            weight=link_weight,
                        )
                    )
                    system_links.append(
                        IntLink(
                            star,
                            d,
                            bandwidth_factor=self._star_link_bandwidth,
                            weight=link_weight,
                        )
                    )

        self.system_links = system_links
        self._int_links.extend(self.system_links)

        num_hosts = len(self.system_routers)
        num_mem = len(self.memory_routers)
        print(
            f"[multi-star] Stars: {self._num_stars}, "
            f"Hosts: {num_hosts}, MemCtrls: {num_mem}"
        )
        print(
            f"[multi-star] Star switch latency: "
            f"{self._star_switch_latency} cycles"
        )
        print(
            f"[multi-star] Star link bandwidth factor: "
            f"{self._star_link_bandwidth}"
        )
        print(f"[multi-star] Total system links: " f"{len(self.system_links)}")


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

    def __init__(self, src_node, dst_node, bandwidth_factor=16, weight=1):
        super().__init__()
        self.link_id = self.version_count()
        self.src_node = src_node
        self.dst_node = dst_node
        self.bandwidth_factor = bandwidth_factor
        self.weight = weight
