from typing import (
    List,
    Optional,
    Sequence,
    Tuple,
)

from m5.objects import (
    AddrRange,
    CXL_CXLDevice_Controller,
    CXL_CXLHost_Controller,
    CXLHostPort,
    DDR4_2400_8x8,
    MemCtrl,
    MemInterface,
    MessageBuffer,
    Port,
    RubySystem,
    SimpleExtLink,
    SimpleIntLink,
    SimpleNetwork,
    SrcClockDomain,
    Switch,
    VoltageDomain,
)
from m5.util.convert import toMemorySize

from .abstract_memory_system import AbstractMemorySystem
from ..boards.abstract_board import AbstractBoard
from ...utils.override import overrides


class CXLSwitch(Switch):
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


class CXLExtLink(SimpleExtLink):
    """Simple ext link with auto counting for the id."""

    _version = 0

    @classmethod
    def version_count(cls):
        cls._version += 1
        return cls._version - 1

    def __init__(self, ext_node, int_node, bandwidth_factor=16):
        super().__init__()
        self.link_id = self.version_count()
        self.ext_node = ext_node
        self.int_node = int_node
        self.bandwidth_factor = bandwidth_factor


class CXLIntLink(SimpleIntLink):
    """Simple int link with auto counting for the id."""

    _version = 0

    @classmethod
    def version_count(cls):
        cls._version += 1
        return cls._version - 1

    def __init__(self, src_node, dst_node, bandwidth_factor=16):
        super().__init__()
        self.link_id = self.version_count()
        self.src_node = src_node
        self.dst_node = dst_node
        self.bandwidth_factor = bandwidth_factor


class CXLNetwork(SimpleNetwork):
    def __init__(self, ruby_system):
        super().__init__()
        # What are netifs?
        self.netifs = []

        # TODO: This should change in the future.
        self.number_of_virtual_networks = 2
        self.ruby_system = ruby_system

    # FIXME: Break this function into smaller functions that focus on connecting
    # the devices and hosts separately. For now this is connecting all hosts to
    # all devices.
    def connect_controllers(self, controllers):
        assert len(controllers) > 0
        print(f"Connecting {len(controllers)} controllers to the network")
        # Creating the switches/routers. Pass the network (self) to the switches.
        routers = [CXLSwitch(self) for _ in range(len(controllers))]
        self.routers = routers
        print(f"Created {len(self.routers)} routers")
        # Creating the external links. Pass the controller and router to the
        # external links. The external link is the link between the controller
        # and the router.
        # FIXME: This may break in the future because we may have more controllers
        # than routers.
        ext_links = [
            CXLExtLink(ctrl, router)
            for ctrl, router in zip(controllers, self.routers)
        ]
        self.ext_links = ext_links

        # Simple P2P network.

        # Creating the internal links. Pass the router to the internal links.
        # The internal link is the link between two routers
        int_links = [
            CXLIntLink(src, dst)
            for src in self.routers
            for dst in self.routers
            if src != dst
        ]
        self.int_links = int_links


class CXLHost(CXL_CXLHost_Controller):
    _version = 0

    @classmethod
    def versionCount(cls):
        cls._version += 1
        return cls._version - 1

    def __init__(self, clk_domain, host_port, ruby_system, req_fwd_latency=1):
        super(CXLHost, self).__init__()

        self.clk_domain = clk_domain
        self.req_fwd_latency = req_fwd_latency
        self.version = CXLHost.versionCount()
        self.ruby_system = ruby_system
        self.hostPort = host_port

    def setup_buffers(self, network):
        self.mandatoryQueue = MessageBuffer()
        self.reqOutToDevice = MessageBuffer()
        self.respInFromDevice = MessageBuffer()

        self.reqOutToDevice.out_port = network.in_port
        self.respInFromDevice.in_port = network.out_port


class CXLDevice(CXL_CXLDevice_Controller):
    _version = 0

    @classmethod
    def versionCount(cls):
        cls._version += 1
        return cls._version - 1

    def __init__(self, clk_domain, mem_port, ruby_system, memory_latency=1):
        super(CXLDevice, self).__init__()

        self.clk_domain = clk_domain
        self.memory_latency = memory_latency
        self.version = CXLDevice.versionCount()
        self.ruby_system = ruby_system
        self.memory_out_port = mem_port

    def setup_buffers(self, network):
        self.reqInFromHost = MessageBuffer()
        self.respOutToHost = MessageBuffer()
        self.requestToMemory = MessageBuffer(
            randomization="disabled", ordered=True
        )
        self.responseFromMemory = MessageBuffer(
            randomization="disabled", ordered=True
        )

        self.reqInFromHost.in_port = network.out_port
        self.respOutToHost.out_port = network.in_port


class CXLMemory(AbstractMemorySystem):
    """A class to implement the CXL Memory system"""

    def __init__(
        self, clk_freq: Optional[str] = "1GHz", size: Optional[str] = "4GiB"
    ) -> None:
        super().__init__()
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = clk_freq
        self.clk_domain.voltage_domain = VoltageDomain()
        self._size = toMemorySize(size)

        self.cxl_host_port = CXLHostPort(
            request_latency=1,
            response_latency=1,
            request_queue_size=-1,
            request_issue_width=-1,
        )

    @overrides(AbstractMemorySystem)
    def incorporate_memory(self, board: AbstractBoard) -> None:
        self.ruby_system = RubySystem(number_of_virtual_networks=2)
        self.ruby_system.block_size_bytes = board.get_cache_line_size()
        # NOTE: AFAIK, this parameter is only used by RubyProfiler.
        self.ruby_system.num_of_sequencers = 0

        self.cxl_host_controller = CXLHost(
            clk_domain=self.clk_domain,
            host_port=self.cxl_host_port,
            ruby_system=self.ruby_system,
            req_fwd_latency=1,
        )
        self.cxl_host_port.ruby_system = self.ruby_system
        self.cxl_host_port.controller = self.cxl_host_controller

        self.mem_ctrl = MemCtrl(dram=DDR4_2400_8x8())
        self.cxl_device_controller = CXLDevice(
            clk_domain=self.clk_domain,
            mem_port=self.mem_ctrl.port,
            ruby_system=self.ruby_system,
            memory_latency=1,
        )

        self.ruby_system.cxl_network = CXLNetwork(self.ruby_system)

        self.cxl_device_controller.setup_buffers(self.ruby_system.cxl_network)
        self.cxl_host_controller.setup_buffers(self.ruby_system.cxl_network)

        self.ruby_system.cxl_network.connect_controllers(
            [self.cxl_host_controller, self.cxl_device_controller]
        )
        self.ruby_system.cxl_network.setup_buffers()

    @overrides(AbstractMemorySystem)
    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return [
            (
                self.cxl_host_port.mem_ranges[0],
                self.cxl_host_port.host_side_port,
            )
        ]

    @overrides(AbstractMemorySystem)
    def get_memory_controllers(self) -> List[MemCtrl]:
        return [self.cxl_host_port]

    @overrides(AbstractMemorySystem)
    def get_mem_interfaces(self) -> List[MemInterface]:
        """Get all memory interfaces in this memory system.
        Useful when creating physical memory objects."""
        raise Exception("No memory in the CXLHostPort")

    @overrides(AbstractMemorySystem)
    def get_size(self) -> int:
        return self._size

    @overrides(AbstractMemorySystem)
    def set_memory_range(self, ranges: List[AddrRange]) -> None:
        print(f"Setting memory range to {ranges} in CXLMemory system")
        # We check this because of the assumption in get_size
        # that there is only one range. This is temporary
        if len(ranges) != 1:
            raise RuntimeError(
                "CXLMemory only supports a single range. "
                f"Got {len(ranges)} ranges."
            )
        self.cxl_host_port.mem_ranges = ranges

    @overrides(AbstractMemorySystem)
    def get_uninterleaved_range(self) -> List[AddrRange]:
        """Returns the range of the memory system without interleaving.
        This is useful when other components in the system want to interleave
        the memory range different to how the memory has interleaved them.
        """
        return self.cxl_host_port.mem_ranges
