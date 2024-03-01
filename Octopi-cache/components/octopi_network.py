from .ruby_network_components import RubyNetworkComponent, RubyExtLink, RubyIntLink, RubyRouter

from m5.objects import SimpleNetwork

# . The Network owns all routers, all int links and all ext links that are not in CCD's.
# . The CCD subsystems are not of type RubyNetwork, so we need to copy the references of
# routers and links to OctopiNetwork._routers, ._int_links, and ._ext_links; which will
# be, in turns, copied to RubyNetwork.routers, .int_links, and .ext_links respectively.
#
# Terms: "connect" -> create int links
#        "incorporate" -> copy references of routers and links, create routers/links if necessary
class OctopiNetwork(SimpleNetwork, RubyNetworkComponent):
    def __init__(self, ruby_system):
        SimpleNetwork.__init__(self=self)
        RubyNetworkComponent.__init__(self=self)
        self.netifs = []
        self.ruby_system = ruby_system
        self.number_of_virtual_networks = ruby_system.number_of_virtual_networks

        self.cross_ccd_router = RubyRouter(self)
        self._add_router(self.cross_ccd_router)

    def connect_ccd_routers_to_cross_ccd_router(self, ccds):
        for ccd in ccds:
            int_link_1, int_link_2 = RubyIntLink.create_bidirectional_links(self.cross_ccd_router, ccd.get_main_router(), bandwidth_factor=64)
            ccd.to_cross_ccd_router_link = int_link_1
            ccd.from_cross_ccd_router_link = int_link_2
            self._add_int_link(int_link_1)
            self._add_int_link(int_link_2)

    def incorporate_ccds(self, ccds):
        for ccd in ccds:
            self.incorporate_ruby_subsystem(ccd)
        self.connect_ccd_routers_to_cross_ccd_router(ccds)
