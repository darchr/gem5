from typing import Type

from m5.objects.Prefetcher import BasePrefetcher
from m5.objects.SimObject import SimObject
from m5.params import (
    Addr,
    NullSimObject,
)
from m5.util import inform

from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.cachehierarchies.chi.nodes.abstract_node import (
    AbstractNode,
)
from gem5.utils.override import overrides

from ..modifier import Modifier
from .cmn import CoherentMeshNetwork


class CMNModifier(Modifier):
    def __init__(self, description):
        super().__init__(description)

    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        raise NotImplementedError

    @overrides(Modifier)
    def _get_simobjects(self, board: AbstractBoard):
        cache_hierarchy = board.get_cache_hierarchy()
        if not isinstance(cache_hierarchy, CoherentMeshNetwork):
            raise ValueError(
                f"This modifier ({self.__class__.__name__}) is "
                "only applicable to CoherentMeshNetwork."
            )
        return self._get_simobjects_from_cache_hierarchy(cache_hierarchy)


class CMNDataSeqModifier(CMNModifier):
    def __init__(self, description: str):
        super().__init__(description)

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return [
            cluster.dcache.sequencer
            for core_tile in cache_hierarchy.core_tiles
            for cluster in core_tile.core_clusters
        ]


class CMNAccessNameRegister(CMNDataSeqModifier):
    def __init__(self):
        super().__init__("Ties program counters to names in high-level code.")
        self._pc_name_map = dict()

    def register_mem_access(self, pc: Addr, name: str):
        self._pc_name_map[pc] = name

    @overrides(CMNModifier)
    def _do_modification(self, sim_object: SimObject):
        if not hasattr(sim_object, "register_mem_access"):
            raise ValueError(
                f"{sim_object.__class__.__name__} does not support "
                "registering memory accesses."
            )
        for pc, name in sim_object._pc_name_map.items():
            sim_object.registerMemAccessName(pc, name)


class CMNSeqTypeSetterModifier(CMNModifier):
    def __init__(self, seq_type: str):
        super().__init__(
            f"Sets seq_type to {seq_type} for the right sequencers in CMN."
        )
        self._seq_type = seq_type

    @overrides(CMNModifier)
    def _do_modification(self, sim_object):
        sim_object.seq_type = self._seq_type


class CMNDataSeqTypeModifier(CMNSeqTypeSetterModifier):
    def __init__(self):
        super().__init__("Data")

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return [
            cluster.dcache.sequencer
            for core_tile in cache_hierarchy.core_tiles
            for cluster in core_tile.core_clusters
        ]


class CMNInstSeqTypeModifier(CMNSeqTypeSetterModifier):
    def __init__(self):
        super().__init__("Inst")

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return [
            cluster.icache.sequencer
            for core_tile in cache_hierarchy.core_tiles
            for cluster in core_tile.core_clusters
        ]


class CMNDMASeqTypeModifier(CMNSeqTypeSetterModifier):
    def __init__(self):
        super().__init__("DMA")

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return (
            [ctrl.sequencer for ctrl in cache_hierarchy.dma_controllers]
            if hasattr(cache_hierarchy, "dma_controllers")
            else []
        )


class CMNSysSeqTypeModifier(CMNSeqTypeSetterModifier):
    def __init__(self):
        super().__init__("Sys")

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return [cache_hierarchy.ruby_system.sys_port_proxy]


class CHICacheConfigModifier(CMNModifier):
    def __init__(self, description, **params):
        super().__init__(description)
        self._params = params.copy()

    @overrides(Modifier)
    def _do_modification(self, sim_object: SimObject):
        for pname, pvalue in self._params.items():
            setattr(sim_object, pname, pvalue)


class CMNL1CacheConfigModifier(CHICacheConfigModifier):
    def __init__(self, **params):
        super().__init__(
            f"Sets {params} params of L1 cache in CMN.",
            **params,
        )

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return [
            cluster.dcache
            for core_tile in cache_hierarchy.core_tiles
            for cluster in core_tile.core_clusters
        ] + [
            cluster.icache
            for core_tile in cache_hierarchy.core_tiles
            for cluster in core_tile.core_clusters
        ]


class CMNL2CacheConfigModifier(CHICacheConfigModifier):
    def __init__(self, **params):
        super().__init__(
            f"Sets {params} params of L2 cache in CMN.",
            **params,
        )

    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return [
            cluster.l2cache
            for core_tile in cache_hierarchy.core_tiles
            for cluster in core_tile.core_clusters
        ]


class CMNSystemCacheConfigModifier(CHICacheConfigModifier):
    def __init__(self, **params):
        super().__init__(
            f"Sets {params} params of system cache in CMN.",
            **params,
        )

    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return [system_cache for system_cache in cache_hierarchy.system_caches]


class CMNDMARequestorModifier(CHICacheConfigModifier):
    def __init__(self, **params):
        super().__init__(
            f"Sets {params} params of DMA requestor in CMN.", **params
        )

    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        if not hasattr(cache_hierarchy, "dma_controllers"):
            inform(
                f"{self.__class__.__name__} did not find any DMA requestors. "
                "Skipping the modification."
            )
            return []
        return [
            dma_controller
            for dma_controller in cache_hierarchy.dma_controllers
        ]


class CMNPrefetcherModifier(CMNModifier):
    def __init__(
        self,
        description,
        prefetcher_cls: Type[BasePrefetcher],
        **params,
    ):
        super().__init__(description)
        self._prefetcher_cls = prefetcher_cls
        self._params = params.copy()

    @overrides(Modifier)
    def _do_modification(self, sim_object: SimObject):
        sim_object.prefetcher = self._prefetcher_cls(**self._params)
        sim_object.use_prefetcher = not isinstance(
            sim_object.prefetcher, NullSimObject
        )


class CMNL1DPrefetcherModifier(CMNPrefetcherModifier):
    def __init__(
        self,
        prefetcher_cls: Type[BasePrefetcher],
        **params,
    ):
        super().__init__(
            f"Sets dcache prefetcher to {prefetcher_cls} "
            f"with the following parameters {params}",
            prefetcher_cls,
            **params,
        )

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return [
            cluster.dcache
            for core_tile in cache_hierarchy.core_tiles
            for cluster in core_tile.core_clusters
        ]


class CMNL1IPrefetcherModifier(CMNPrefetcherModifier):
    def __init__(
        self,
        prefetcher_cls: Type[BasePrefetcher],
        **params,
    ):
        super().__init__(
            f"Sets icache prefetcher to {prefetcher_cls} "
            f"with the following parameters {params}",
            prefetcher_cls,
            **params,
        )

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return [
            cluster.icache
            for core_tile in cache_hierarchy.core_tiles
            for cluster in core_tile.core_clusters
        ]


class CMNL2PrefetcherModifier(CMNPrefetcherModifier):
    def __init__(
        self,
        prefetcher_cls: Type[BasePrefetcher],
        **params,
    ):
        super().__init__(
            f"Sets l2cache prefetcher to {prefetcher_cls} "
            f"with the following parameters {params}",
            prefetcher_cls,
            **params,
        )

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return [
            cluster.l2cache
            for core_tile in cache_hierarchy.core_tiles
            for cluster in core_tile.core_clusters
        ]


class CMNClusterLatModifier(CMNModifier):
    def __init__(self, ext_latency: int, int_latency: int):
        description = (
            f"Sets ext_routing_latency for cluster routers to {ext_latency} "
            f"and int_routing_latency for cluster routers to {int_latency}."
        )
        if not (ext_latency > 0 and int_latency > 0):
            raise ValueError(
                "ext_latency and int_latency should be greater than 0."
            )
        super().__init__(description)
        self._ext_latency = ext_latency
        self._int_latency = int_latency

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return [
            cluster.router
            for core_tile in cache_hierarchy.core_tiles
            for cluster in core_tile.core_clusters
        ]

    @overrides(Modifier)
    def _do_modification(self, sim_object: SimObject):
        sim_object.ext_routing_latency = self._ext_latency
        sim_object.int_routing_latency = self._int_latency


class CMNSystemLatModifier(CMNModifier):
    def __init__(self, ext_latency: int, int_latency: int):
        description = (
            f"Sets ext_routing_latency for system routers to {ext_latency} "
            f"and int_routing_latency for system routers to {int_latency}."
        )
        if not (ext_latency > 0 and int_latency > 0):
            raise ValueError(
                "ext_latency and int_latency should be greater than 0."
            )
        super().__init__(description)
        self._ext_latency = ext_latency
        self._int_latency = int_latency

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return [
            router
            for router in cache_hierarchy.ruby_system.network.system_routers
        ]

    @overrides(Modifier)
    def _do_modification(self, sim_object: SimObject):
        sim_object.ext_routing_latency = self._ext_latency
        sim_object.int_routing_latency = self._int_latency


class CMNMemoryLatModifier(CMNModifier):
    def __init__(self, ext_latency: int, int_latency: int):
        description = (
            f"Sets ext_routing_latency for memory routers to {ext_latency} "
            f"and int_routing_latency for memory routers to {int_latency}."
        )
        if not (ext_latency > 0 and int_latency > 0):
            raise ValueError(
                "ext_latency and int_latency should be greater than 0."
            )
        super().__init__(description)
        self._ext_latency = ext_latency
        self._int_latency = int_latency

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return [
            router
            for router in cache_hierarchy.ruby_system.network.memory_routers
        ]

    @overrides(Modifier)
    def _do_modification(self, sim_object: SimObject):
        sim_object.ext_routing_latency = self._ext_latency
        sim_object.int_routing_latency = self._int_latency


class CMNDMALatModifier(CMNModifier):
    def __init__(self, ext_latency: int, int_latency: int):
        description = (
            f"Sets ext_routing_latency for DMA routers to {ext_latency} "
            f"and int_routing_latency for DMA routers to {int_latency}."
        )
        if not (ext_latency > 0 and int_latency > 0):
            raise ValueError(
                "ext_latency and int_latency should be greater than 0."
            )
        super().__init__(description)
        self._ext_latency = ext_latency
        self._int_latency = int_latency

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        if not hasattr(cache_hierarchy.ruby_system.network, "dma_routers"):
            inform(
                f"{self.__class__.__name__} did not find any DMA routers. "
                "Skipping the modification."
            )
            return []
        return [
            router
            for router in cache_hierarchy.ruby_system.network.dma_routers
        ]

    @overrides(Modifier)
    def _do_modification(self, sim_object: SimObject):
        sim_object.ext_routing_latency = self._ext_latency
        sim_object.int_routing_latency = self._int_latency


class CMNReplPolModifier(CMNModifier):
    def __init__(self, repl_policy_cls, **params):
        description = (
            f"Sets replacement policy to {repl_policy_cls} "
            f"with the following parameters {params}."
        )
        super().__init__(description)
        self._repl_policy_cls = repl_policy_cls
        self._params = params.copy()

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return (
            [
                cluster.dcache.cache
                for core_tile in cache_hierarchy.core_tiles
                for cluster in core_tile.core_clusters
            ]
            + [
                cluster.icache.cache
                for core_tile in cache_hierarchy.core_tiles
                for cluster in core_tile.core_clusters
            ]
            + [
                cluster.l2cache.cache
                for core_tile in cache_hierarchy.core_tiles
                for cluster in core_tile.core_clusters
            ]
            + [
                system_cache.cache
                for system_cache in cache_hierarchy.system_caches
            ]
        )

    @overrides(Modifier)
    def _do_modification(self, sim_object: SimObject):
        sim_object.replacement_policy = self._repl_policy_cls(**self._params)


class CMNSparseAccessSizeModifier(CMNModifier):
    def __init__(self, sparse_access_size: int):
        description = f"Sets sparse access size to {sparse_access_size}."
        super().__init__(description)
        self._sparse_access_size = sparse_access_size

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        tail = (
            [
                dma_controller.cache
                for dma_controller in cache_hierarchy.dma_controllers
            ]
            if hasattr(cache_hierarchy, "dma_controllers")
            else []
        )
        return (
            [
                cluster.dcache.cache
                for core_tile in cache_hierarchy.core_tiles
                for cluster in core_tile.core_clusters
            ]
            + [
                cluster.icache.cache
                for core_tile in cache_hierarchy.core_tiles
                for cluster in core_tile.core_clusters
            ]
            + [
                cluster.l2cache.cache
                for core_tile in cache_hierarchy.core_tiles
                for cluster in core_tile.core_clusters
            ]
            + [
                system_cache.cache
                for system_cache in cache_hierarchy.system_caches
            ]
            + tail
        )

    @overrides(Modifier)
    def _do_modification(self, sim_object: SimObject):
        sim_object.sparse_access_size = self._sparse_access_size


class CMNDataChannelSizeModifier(CMNModifier):
    def __init__(
        self, cache_line_size: int, transfers_per_cache_block: int = 1
    ):
        description = f"Sets data channel size to {cache_line_size}."
        super().__init__(description)
        self._cache_line_size = cache_line_size
        self._transfers_per_cache_block = transfers_per_cache_block

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        tail = (
            [
                dma_controller
                for dma_controller in cache_hierarchy.dma_controllers
            ]
            if hasattr(cache_hierarchy, "dma_controllers")
            else []
        )
        return (
            [
                cluster.dcache
                for core_tile in cache_hierarchy.core_tiles
                for cluster in core_tile.core_clusters
            ]
            + [
                cluster.icache
                for core_tile in cache_hierarchy.core_tiles
                for cluster in core_tile.core_clusters
            ]
            + [
                cluster.l2cache
                for core_tile in cache_hierarchy.core_tiles
                for cluster in core_tile.core_clusters
            ]
            + [system_cache for system_cache in cache_hierarchy.system_caches]
            + [
                memory_controller
                for memory_controller in cache_hierarchy.memory_controllers
            ]
            + tail
        )

    @overrides(Modifier)
    def _do_modification(self, sim_object):
        sim_object.data_channel_size = (
            self._cache_line_size // self._transfers_per_cache_block
        )


class CMNRubySystemBlockSizeModifier(CMNModifier):
    def __init__(self, cache_line_size: int):
        description = f"Sets block size to {cache_line_size}."
        super().__init__(description)
        self._cachel_line_size = cache_line_size

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return cache_hierarchy.ruby_system

    @overrides(Modifier)
    def _do_modification(self, sim_object: SimObject):
        sim_object.block_size_bytes = self._cachel_line_size


class CMNPrefetcherBlockSizeModifier(CMNModifier):
    def __init__(self, cache_line_size: int):
        description = f"Sets block size to {cache_line_size}."
        super().__init__(description)
        self._cache_line_size = cache_line_size

    @overrides(CMNModifier)
    def _get_simobjects_from_cache_hierarchy(self, cache_hierarchy):
        return (
            [
                cluster.dcache.prefetcher
                for core_tile in cache_hierarchy.core_tiles
                for cluster in core_tile.core_clusters
            ]
            + [
                cluster.icache.prefetcher
                for core_tile in cache_hierarchy.core_tiles
                for cluster in core_tile.core_clusters
            ]
            + [
                cluster.l2cache.prefetcher
                for core_tile in cache_hierarchy.core_tiles
                for cluster in core_tile.core_clusters
            ]
        )

    @overrides(Modifier)
    def _do_modification(self, sim_object):
        sim_object.block_size = self._cache_line_size
