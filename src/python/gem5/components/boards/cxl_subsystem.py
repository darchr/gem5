from typing import List

from m5.objects import (
    CXL_CXLDevice_Controller,
    CXL_CXLHost_Controller,
    CXLHostPort,
    MessageBuffer,
    RubySystem,
    SimpleExtLink,
    SimpleIntLink,
    SimpleNetwork,
    SubSystem,
    Switch,
    SysBridge,
    System,
)

from .abstract_board import AbstractBoard
from ..memory.abstract_memory_system import AbstractMemorySystem


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


class CXLTopology:
    def __init__(self, desc: str) -> None:
        self._desc = desc

    def __str__(self) -> str:
        return self._desc

    def __repr__(self) -> str:
        return self._desc

    def setup_internal_network(
        self, left_side, right_side
    ) -> List[CXLIntLink]:
        raise NotImplementedError


class CXLNetwork(SimpleNetwork):
    def __init__(self, ruby_system: RubySystem, topology: CXLTopology):
        super().__init__()
        # What are netifs?
        self.netifs = []

        # TODO: vnets will change in the future with coherence.
        self.number_of_virtual_networks = 2
        self.ruby_system = ruby_system
        self._topology = topology

    # FIXME: Break this function into smaller functions that focus on connecting
    # the devices and hosts separately. For now this is connecting all hosts to
    # all devices.
    # NOTE: This function will be replaced by self._topology.setup_internal_network
    def connect_controllers(self, controllers):
        assert len(controllers) > 0
        # Creating the switches/routers. Pass the network (self) to the switches.
        routers = [CXLSwitch(self) for _ in range(len(controllers))]
        self.routers = routers
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

    def __init__(
        self, clk_domain, mem_port, mem_range, ruby_system, memory_latency=1
    ):
        super(CXLDevice, self).__init__()

        self.clk_domain = clk_domain
        self.memory_latency = memory_latency
        self.version = CXLDevice.versionCount()
        self.ruby_system = ruby_system
        self.memory_out_port = mem_port
        self.addr_ranges = [mem_range]

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


class CXLSubSystem(SubSystem):
    """A class to implement the CXL Memory system"""

    def _ensure_cache_block_size_conformity(
        boards: List[AbstractBoard], block_size: int
    ) -> None:
        """
        Ensure that all boards have the same cache line size.
        """
        check = all(
            [
                board.get_cache_line_size().getValue() == block_size
                for board in boards
            ]
        )
        if not check:
            raise ValueError(
                "All boards must have the same cache line size as the CXLSubSystem."
            )

    def __init__(
        self,
        boards: List[AbstractBoard],
        memory_system: AbstractMemorySystem,
        block_size: int,
    ) -> None:
        super().__init__()
        CXLSubSystem._ensure_cache_block_size_conformity(boards, block_size)
        self._boards = boards
        self._num_boards = len(boards)
        self.memory_system = memory_system
        self._memory_range = self.memory_system.get_uninterleaved_range()[0]
        self._cache_line_size = block_size

    # NOTE: cxl_system should be created by something like a Cluster and
    # passed to this function. Somehow, CXLSubSystem can not be a
    # `CXLSystem` (inherit from System SimObject).
    def build_cxl_network(self, cxl_system: System) -> None:
        self.ruby_system = RubySystem(number_of_virtual_networks=2)
        self.ruby_system.block_size_bytes = self._cache_line_size
        # NOTE: AFAIK, this parameter is only used by RubyProfiler.
        self.ruby_system.num_of_sequencers = 0
        self.ruby_system.cxl_network = CXLNetwork(
            self.ruby_system,
            CXLTopology("This is a placeholder object for CXLTopology."),
        )

        # TODO: Potentially can be migrated to CXLHost class.
        self.cxl_host_ports = [
            CXLHostPort(
                request_latency=1,
                response_latency=1,
                request_queue_size=-1,
                request_issue_width=-1,
                mem_ranges=[self._memory_range],
                ruby_system=self.ruby_system,
            )
            for _ in range(self._num_boards)
        ]
        self.sys_bridges = [
            SysBridge(
                source=board,
                target=cxl_system,
                target_port=host_port.host_side_port,
            )
            for host_port, board in zip(self.cxl_host_ports, self._boards)
        ]

        # NOTE: self.cxl_host_controllers has to be defined like this because
        # gem5 will throw out an attribute error if we append CXLHosts to
        # self.cxl_host_controllers. If we try to define it by first creating
        # a local variable and then assigning it to self.cxl_host_controllers
        # it will throw an error because it can't find _parent attribute in
        # SimObjectVector. gem5 tries to access _parent when it throws out a
        # warning about self.cxl_host_controllers already having a parent in
        # SimObject.py:add_child(), line 978.
        self.cxl_host_controllers = [
            CXLHost(
                clk_domain=cxl_system.clk_domain,
                host_port=host_port,
                ruby_system=self.ruby_system,
                req_fwd_latency=1,
            )
            for host_port in self.cxl_host_ports
        ]

        # TODO: Replicate this for every memory channel
        # Wrap these channels into a single logical device
        # This device could potentially be split into multiple objects
        # eg CXLDeviceHead and CXLDeviceTail
        self.cxl_device_controllers = [
            CXLDevice(
                clk_domain=cxl_system.clk_domain,
                mem_port=port,
                mem_range=_range,
                ruby_system=self.ruby_system,
                memory_latency=1,
            )
            for _range, port in self.memory_system.get_mem_ports()
        ]

        for controller in (
            self.cxl_host_controllers + self.cxl_device_controllers
        ):
            controller.setup_buffers(self.ruby_system.cxl_network)

        self.ruby_system.cxl_network.connect_controllers(
            self.cxl_host_controllers + self.cxl_device_controllers
        )
        # setup_buffers is inherited from SimpleNetwork
        self.ruby_system.cxl_network.setup_buffers()

        for board, sys_brige in zip(self._boards, self.sys_bridges):
            board.set_remote_memory_ports(
                [(self._memory_range, sys_brige.source_port)]
            )
