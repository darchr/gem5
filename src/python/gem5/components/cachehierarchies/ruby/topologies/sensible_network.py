from m5.objects import (
    SimpleExtLink,
    SimpleIntLink,
    SimpleNetwork,
    Switch,
)

from ....processors.abstract_core import AbstractCore


class SensibleNetwork(SimpleNetwork):
    """A simple point-to-point network. This doesn't not use garnet."""

    _next_router_id = 0
    _next_ext_link_id = 0
    _next_int_link_id = 0

    @classmethod
    def next_router_id(cls):
        cls._next_router_id += 1
        return cls._next_router_id - 1

    @classmethod
    def next_ext_link_id(cls):
        cls._next_ext_link_id += 1
        return cls._next_ext_link_id - 1

    @classmethod
    def next_int_link_id(cls):
        cls._next_int_link_id += 1
        return cls._next_int_link_id - 1

    def __init__(self, ruby_system):
        super().__init__()
        self.netifs = []

        # TODO: These should be in a base class
        # https://gem5.atlassian.net/browse/GEM5-1039
        self.ruby_system = ruby_system

        self._routers = []
        self._ext_links = []
        self._int_links = []

    def add_core_cluster(self, l1cache, l2cache):
        """Add a core cluster to the network. This is used to add a core
        cluster to the network.
        """
        self._routers.append(
            Switch(router_id=SensibleNetwork.next_router_id())
        )
        self._ext_links.append(
            SimpleExtLink(
                link_id=SensibleNetwork.next_ext_link_id(),
                ext_node=l1cache,
                int_node=self._routers[-1],
            )
        )
        self._ext_links.append(
            SimpleExtLink(
                link_id=SensibleNetwork.next_ext_link_id(),
                ext_node=l2cache,
                int_node=self._routers[-1],
            )
        )
        return self._routers[-1]

    def add_uncore_cluster(self, uncore_controllers):
        if len(uncore_controllers) == 0:
            return None

        self._routers.append(
            Switch(router_id=SensibleNetwork.next_router_id())
        )
        for uncore_controller in uncore_controllers:
            self._ext_links.append(
                SimpleExtLink(
                    link_id=SensibleNetwork.next_ext_link_id(),
                    ext_node=uncore_controller,
                    int_node=self._routers[-1],
                )
            )
        return self._routers[-1]

    def make_dance_hall(self, left_side, right_side):
        for left in left_side:
            for right in right_side:
                self._int_links.append(
                    SimpleIntLink(
                        link_id=SensibleNetwork.next_int_link_id(),
                        src_node=left,
                        dst_node=right,
                    )
                )
                self._int_links.append(
                    SimpleIntLink(
                        link_id=SensibleNetwork.next_int_link_id(),
                        src_node=right,
                        dst_node=left,
                    )
                )

    def make_all_to_all(self, controllers):
        for i in range(len(controllers)):
            for j in range(i + 1, len(controllers)):
                if i == j:
                    continue
                self._int_links.append(
                    SimpleIntLink(
                        link_id=SensibleNetwork.next_int_link_id(),
                        src_node=controllers[i],
                        dst_node=controllers[j],
                    )
                )
                self._int_links.append(
                    SimpleIntLink(
                        link_id=SensibleNetwork.next_int_link_id(),
                        src_node=controllers[j],
                        dst_node=controllers[i],
                    )
                )

    def finalize(self):
        self.routers = self._routers
        self.ext_links = self._ext_links
        self.int_links = self._int_links
