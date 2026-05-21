from math import log
from typing import (
    List,
    Type,
    Union,
)

from m5.objects import (  # UsefulDataType,
    NULL,
    Addr,
    AddrRange,
    ClockDomain,
    RubyCache,
    RubyController,
    RubyNetwork,
    RubyPortProxy,
    RubySequencer,
    RubySystem,
    SimpleMemory,
)

# from gem5.configs.common import cores
from m5.util.convert import toMemorySize

from gem5.coherence_protocol import CoherenceProtocol
from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.components.cachehierarchies.chi.nodes.abstract_node import (
    AbstractNode,
)
from gem5.components.cachehierarchies.chi.nodes.dma_requestor import (
    DMARequestor,
)
from gem5.components.cachehierarchies.chi.nodes.memory_controller import (
    MemoryController,
)
from gem5.components.cachehierarchies.ruby.abstract_ruby_cache_hierarchy import (
    AbstractRubyCacheHierarchy,
)
from gem5.utils.override import overrides
from gem5.utils.requires import requires

from .host import CHI_Host
from .network import (
    BaseSystemNetwork,
    MultiHostNetwork,
)


class CHI_3_Level_Remote_Dir(AbstractRubyCacheHierarchy):
    def __init__(
        self,
        l1i_size: str = "64KiB",
        l1d_size: str = "64KiB",
        l2_size: str = "1MiB",
        slc_size: str = "32MiB",
        slc_intlv_size: str = "128B",
        num_hosts: int = 1,
        num_hns: int = 2,
        directory_remote_latency: int = 250,
        enable_numa: bool = False,
        pmem_address_range: AddrRange = None,
        # system_network_cls: Type[BaseSystemNetwork] = BaseSystemNetwork,
    ) -> None:
        """ """
        super().__init__()
        # super(AbstractCacheHierarchy, self).__init__()
        self._l1i_size = l1i_size
        self._l1d_size = l1d_size
        self._l2_size = l2_size
        self._slc_size = slc_size  # toMemorySize(slc_size)
        self._slc_intlv_size = slc_intlv_size
        self._num_hosts = num_hosts
        self._num_hns = num_hns
        self._directory_remote_latency = directory_remote_latency
        self._enable_numa = enable_numa
        self._pmem_address_range = pmem_address_range
        # self._system_network_cls = system_network_cls

    def _intlv_memory_for_hosts(
        self,
        start_addr: Addr,
        mem_size: int,
        num_hosts: int,
        intlv_size: Union[int, str],
    ) -> List[AddrRange]:
        if num_hosts == 1:
            return [AddrRange(start=start_addr, size=mem_size)]
        intlv_size = toMemorySize(intlv_size)
        intlv_low_bit = int(log(intlv_size, 2))
        intlv_bits = int(log(num_hosts, 2))
        return [
            AddrRange(
                start=start_addr,
                size=mem_size,
                intlvHighBit=intlv_low_bit + intlv_bits - 1,
                xorHighBit=0,
                intlvBits=intlv_bits,
                intlvMatch=i,
            )
            for i in range(num_hosts)
        ]

    @overrides(AbstractCacheHierarchy)
    def incorporate_cache(self, board: AbstractBoard) -> None:
        requires(coherence_protocol_required=CoherenceProtocol.CHI)

        self.ruby_system = RubySystem(
            block_size_bytes=board.get_cache_line_size()
        )

        # Network configurations
        # virtual networks: 0=request, 1=snoop, 2=response, 3=data
        self.ruby_system.number_of_virtual_networks = 4

        # Ruby's global network.
        self.ruby_system.network = MultiHostNetwork(
            self.ruby_system, self.ruby_system.number_of_virtual_networks
        )

        memory_controllers = []

        for rng, port in board.get_mem_ports():
            print(f"Adding memory controller for range: {rng}")

            memory_controller = MemoryController(
                self.ruby_system.network, [rng], port
            )
            memory_controller.ruby_system = self.ruby_system
            memory_controllers.append(memory_controller)
            # add some of these to remote mem?
            # maybe add a get_remote_mem_ports to the board?

        self.memory_controllers = memory_controllers

        cores = board.get_processor().get_cores()
        assert (
            len(cores) % self._num_hosts
        ) == 0, "Number of cores must be divisible by number of hosts"
        assert (
            self._num_hns >= self._num_hosts
        ), "Number of home nodes must be >= number of hosts"
        assert (
            self._num_hns % self._num_hosts
        ) == 0, "Number of home nodes must be divisible by number of hosts"

        # unsure if this is needed
        cores_per_host = int(len(cores) / self._num_hosts)

        mem_range = board.get_memory().get_uninterleaved_range()[0]

        hosts = []
        sequencers = []
        system_caches = []

        if self._enable_numa:
            hns_per_host = self._num_hns // self._num_hosts
            mem_per_host = mem_range.size() // self._num_hosts
            addr_ranges = []
            for i in range(self._num_hosts):
                host_start = mem_range.start + (i * mem_per_host)
                host_ranges = self._intlv_memory_for_hosts(
                    host_start,
                    mem_per_host,
                    hns_per_host,
                    self._slc_intlv_size,
                )
                addr_ranges.extend(host_ranges)
        else:
            addr_ranges = self._intlv_memory_for_hosts(
                mem_range.start,
                mem_range.size(),
                self._num_hns,
                self._slc_intlv_size,
            )

        for i in range(self._num_hosts):
            cores_in_host = cores[
                i * cores_per_host : (i + 1) * cores_per_host
            ]

            host = CHI_Host(
                cores_in_host,
                board,
                self.ruby_system.network,
                self.ruby_system,
                self._l1i_size,
                self._l1d_size,
                self._l2_size,
                host_id=i,
                pmem_address_range=self._pmem_address_range,
                # clk_domain=board.get_clock_domain(),
            )
            hosts.append(host)
            sequencers.extend(host._sequencers)

        # Divide total SLC size by the number of HNs to maintain constant total L3 capacity
        per_hn_slc_size = toMemorySize(self._slc_size) // self._num_hns
        hns_per_host = self._num_hns // self._num_hosts

        for j in range(self._num_hns):
            # Calculate which host this HN belongs to
            host_id_for_hn = j // hns_per_host

            # Create the system cache (SLC)
            system_cache = SystemLevelCache(
                size=f"{per_hn_slc_size}B",
                assoc=16,
                network=self.ruby_system.network,
                cache_line_size=board.get_cache_line_size(),
                clk_domain=board.get_clock_domain(),
                host_id=host_id_for_hn,  # Pass the computed host_id to the SystemLevelCache
                num_hns=self._num_hns,  # Pass num_hns for resource scaling
                directory_remote_latency=self._directory_remote_latency,
            )
            # WILLCHANGED
            ranges = [addr_ranges[j]]
            if j == 0:
                ranges.append(AddrRange(0, size="4KiB"))
            system_cache.addr_ranges = ranges

            # DEGNAHCLLIW
            system_cache.ruby_system = self.ruby_system
            system_caches.append(system_cache)

        self.hosts = hosts
        self.system_caches = system_caches

        print(f"Number of Hosts:  {len(self.hosts)}")
        print(f"Number of System Caches:  {len(self.system_caches)}")
        print(
            f"System Cache mem ranges:  {self.system_caches[0].addr_ranges[0].start} - {self.system_caches[0].addr_ranges[0].end}"
        )
        print(f"Number of Memory Controllers:  {len(self.memory_controllers)}")

        # Point each System Level Cache to the memory-side controllers
        for slc in self.system_caches:
            slc.downstream_destinations = self.memory_controllers

        for host in self.hosts:
            host.set_downstream_destinations(self.system_caches)

        if board.has_dma_ports():
            self.dma_controllers = self._create_dma_controllers(
                board, self.system_caches
            )
            self.ruby_system.num_of_sequencers = len(cores) * 2 + len(
                self.dma_controllers
            )
        else:
            self.ruby_system.num_of_sequencers = len(cores) * 2

        # connect everything here

        self.ruby_system.network.connect_hosts(self.hosts, self.system_caches)
        self.ruby_system.network.connect_memory_controllers(
            self.memory_controllers
        )

        # added below on 12/8 check chatgpt for why
        for mem_ctrl in self.memory_controllers:
            mem_ctrl.downstream_destinations = []  # or [NULL] if needed

        self.ruby_system.network.connect_dma_controllers(
            self.dma_controllers if board.has_dma_ports() else []
        )

        self.ruby_system.network.build_system_network(self.hosts)
        # Connect system_port for board/system
        # self._system_port = self.ruby_system.network.master
        self.ruby_system.sys_port_proxy = RubyPortProxy(
            ruby_system=self.ruby_system
        )
        # self._system_port = self.ruby_system.sys_port_proxy.in_ports
        self.ruby_system.network.finalize()
        self.ruby_system.network.setup_buffers()

        self.ruby_system.sys_port_proxy = RubyPortProxy(
            ruby_system=self.ruby_system
        )
        board.connect_system_port(self.ruby_system.sys_port_proxy.in_ports)

    # @property
    # def get_system_port(self):
    #     return self._system_port

    def _create_dma_controllers(
        self,
        board: AbstractBoard,
        downstream_destinations: List[RubyController],
    ) -> List[DMARequestor]:

        dma_controllers = []
        for i, port in enumerate(board.get_dma_ports()):
            ctrl = DMARequestor(
                self.ruby_system.network,
                board.get_cache_line_size(),
                board.get_clock_domain(),
            )
            ctrl.cache.size = "1KiB"
            version = len(board.get_processor().get_cores()) + i
            ctrl.sequencer = RubySequencer(
                version=version,
                in_ports=port,
                ruby_system=self.ruby_system,
            )
            ctrl.sequencer.dcache = NULL
            # ctrl.sequencer.icache = NULL

            ctrl.ruby_system = self.ruby_system
            ctrl.sequencer.ruby_system = self.ruby_system

            ctrl.downstream_destinations = downstream_destinations

            dma_controllers.append(ctrl)

        return dma_controllers


class SystemLevelCache(AbstractNode):
    """This cache assumes that it is a shared victim L3.

    This cache also tracks all sharers in caches closer to the CPU.
    """

    def __init__(
        self,
        size: str,
        assoc: int,
        network: RubyNetwork,
        cache_line_size,
        clk_domain: ClockDomain,
        host_id: int = None,  # Added host_id parameter
        num_hns: int = 1,  # Changed to num_hns for resource scaling
        directory_remote_latency: int = 250,
    ):
        super().__init__(network, cache_line_size)
        if host_id is not None:
            self.host_id = host_id

        self.cache = RubyCache(
            size=size, assoc=assoc, start_index_bit=self.getBlockSizeBits()
        )

        # delay_ns = directory_remote_latency.replace("ns", "")
        delay_cycles = directory_remote_latency * 3  # Assuming a 3GHz clock
        self.remote_hit_delay = delay_cycles

        self.clk_domain = clk_domain
        self.prefetcher = NULL
        self.use_prefetcher = False

        # Only used for L1 controllers
        self.send_evictions = False
        self.sequencer = NULL

        self.is_HN = True
        self.enable_DMT = True
        self.enable_DCT = True

        self.allow_SD = True

        self.alloc_on_seq_acc = False  # Does not apply to L3
        self.alloc_on_seq_line_write = False

        self.alloc_on_readshared = (
            False  # I think this should be True for perf
        )
        self.alloc_on_readunique = False
        # NOTE: Based on CHI config in gem5.
        self.alloc_on_readonce = True

        # insert on writeback (victim cache)
        self.alloc_on_writeback = True

        # NOTE: Based on CHI config in gem5.
        self.alloc_on_atomic = True

        ###########################
        # NOTE: If an upstream asks for unique, we should deallocate.
        # It will probably write to it.
        self.dealloc_on_unique = True

        self.dealloc_on_shared = False
        ###########################

        # Allow caches closer to core to keep block even if evicted from L3
        self.dealloc_backinv_unique = False
        self.dealloc_backinv_shared = False

        # Scale resources based on HN count to prevent starvation with fewer HNs
        # Baseline is 8 HNs with 64 TBEs each
        scale = 8 // num_hns
        if scale < 1:
            scale = 1

        # Some reasonable default TBE params
        self.number_of_TBEs = 64
        self.number_of_repl_TBEs = 64
        self.number_of_snoop_TBEs = 8
        self.number_of_DVM_TBEs = 16
        self.number_of_DVM_snoop_TBEs = 4
        self.unify_repl_TBEs = False
        # self.number_of_TBEs = 64 * scale
        # self.number_of_repl_TBEs = 64 * scale
        # self.number_of_snoop_TBEs = 8 * scale
        # self.number_of_DVM_TBEs = 16 * scale
        # self.number_of_DVM_snoop_TBEs = 4 * scale
        # self.unify_repl_TBEs = False
