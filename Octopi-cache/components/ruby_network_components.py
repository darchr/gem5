from m5.objects import Switch, SimpleIntLink, SimpleExtLink

class RubyNetworkComponent():
    def __init__(self):
        super().__init__()
        self._routers = []
        self._ext_links = []
        self._int_links = []
    def _add_router(self, router):
        self._routers.append(router)
    def _add_ext_link(self, link):
        self._ext_links.append(link)
    def _add_int_link(self, link):
        self._int_links.append(link)
    def get_routers(self):
        return self._routers
    def get_ext_links(self):
        return self._ext_links
    def get_int_links(self):
        return self._int_links
    def incorporate_ruby_subsystem(self, other_ruby_subsystem):
        self._routers.extend(other_ruby_subsystem.get_routers())
        self._ext_links.extend(other_ruby_subsystem.get_ext_links())
        self._int_links.extend(other_ruby_subsystem.get_int_links())

class RubyRouter(Switch):
    _router_id = 0

    @classmethod
    def _get_router_id(cls):
        cls._router_id += 1
        return cls._router_id - 1
    
    def __init__(self, network):
        super().__init__()
        self.router_id = self._get_router_id()
        self.virt_nets = network.number_of_virtual_networks

class RubyExtLink(SimpleExtLink):
    _link_id = 0

    @classmethod
    def _get_link_id(cls):
        cls._link_id += 1
        return cls._link_id - 1

    def __init__(self, ext_node, int_node, bandwidth_factor=16):
        super().__init__()
        self.link_id = self._get_link_id()
        self.ext_node = ext_node
        self.int_node = int_node
        self.bandwidth_factor = bandwidth_factor

class RubyIntLink(SimpleIntLink):
    _link_id = 0

    @classmethod
    def _get_link_id(cls):
        cls._link_id += 1
        return cls._link_id - 1

    @classmethod
    def create_bidirectional_links(cls, node_1, node_2, bandwidth_factor=16):
        return [RubyIntLink(node_1, node_2, bandwidth_factor), RubyIntLink(node_2, node_1, bandwidth_factor)]

    def __init__(self, src_node, dst_node, bandwidth_factor=16):
        super().__init__()
        self.link_id = self._get_link_id()
        self.src_node = src_node
        self.dst_node = dst_node
        self.bandwidth_factor = bandwidth_factor
