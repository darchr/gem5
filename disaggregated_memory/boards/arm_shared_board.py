# Copyright (c) 2023-24 The Regents of the University of California
# All rights reserved.
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

# The goal of this board is to combine the gem5-only and the gem5-SSt boards
# into one single board.
import os
import sys

from typing import (
    List,
    Sequence,
    Tuple,
)

# all the source files are one directory above.
sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
)

from memories.external_remote_memory import ExternalRemoteMemory
from boards.arm_main_board import ArmComposableMemoryBoard

import m5
from m5.objects import (
    AddrRange,
    ArmSystem,
    BadAddr,
    ExternalMemory,
    IOXBar,
    NoncoherentXBar,
    Port,
    SrcClockDomain,
    Terminal,
    VncServer,
    VoltageDomain,
)
from m5.objects.ArmFsWorkload import ArmFsLinux
from m5.objects.ArmSystem import (
    ArmDefaultRelease,
    ArmRelease,
)
from m5.objects.RealView import (
    VExpress_GEM5_Base,
    VExpress_GEM5_Foundation,
)
from m5.util.fdthelper import (
    Fdt,
    FdtNode,
    FdtProperty,
    FdtPropertyStrings,
    FdtPropertyWords,
    FdtState,
)

from gem5.components.boards.arm_board import ArmBoard
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.processors.abstract_processor import AbstractProcessor
from gem5.utils.override import overrides
from m5.util import (
    fatal,
    warn,
)

class ArmSharedMemoryBoard(ArmComposableMemoryBoard):
    """
    A high-level ARM board that can zNUMA-capable systems with a remote
    memories. This board is extended from the ArmBoard from Gem5 standard
    library. This board assumes that you will be booting Linux. This board can
    be used to do disaggregated ARM system research while accelerating the
    simulation using kvm.

    The revised ArmComposableMemoryBoard combines the older boards into one
    single board to make the boards compatible with both gem5 and SST.

    **Limitations**
    * kvm is only supported in a gem5-only setup.

    @params
    :clk_freq: Clock frequency of the board
    :processor: An abstract processor to use with this board.
    :local_memory: An abstract memory system taht starts at 0x80000000
    :remote_memory: An abstract memory system that either starts at the end of
            local memory or at a custom address range defined by the user.
    :cache_hierarchy: An abstract_cache_hierarchy compatible with local and
            remote memories.
    :platform: Arm-specific platform to use with this board.
    :release: Arm-specific extensions to use with this board.
    :remote_memory_access_cycles: Optionally add some latency to access the
            remote memory. If the remote memory is being simulated in SST, then
            pass this as a param on the sst-side runscript.
    :remote_memory_address_range: Use this to force map the remote memory
            address range when using stdlib DRAM/memory interfaces.
    """

    def __init__(
        self,
        clk_freq: str,
        processor: AbstractProcessor,
        local_memory: AbstractMemorySystem,
        remote_memory: AbstractMemorySystem,
        cache_hierarchy: AbstractCacheHierarchy,
        platform: VExpress_GEM5_Base = VExpress_GEM5_Foundation(),
        release: ArmRelease = ArmDefaultRelease(),
        remote_memory_access_cycles: int = 0,
        remote_memory_address_range: AddrRange = None,
    ) -> None:
        super().__init__(
            clk_freq=clk_freq,
            processor=processor,
            local_memory=local_memory,
            remote_memory=remote_memory,
            cache_hierarchy=cache_hierarchy,
            platform=platform,
            release=release,
            remote_memory_access_cycles=remote_memory_access_cycles,
            remote_memory_address_range=remote_memory_address_range
        )
        # We need to make sure NUMA nodes are not created in this board.
        # Instead a memory range is created which has the same physical address
        # backing for all the nodes that we're simulating.

    @overrides(ArmComposableMemoryBoard)
    def generateDeviceTree(self, state):
        # Generate a device tree root node for the system by creating the root
        # node and adding the generated subnodes of all children.
        # When a child needs to add multiple nodes, this is done by also
        # creating a node called '/' which will then be merged with the
        # root instead of appended.

        def generateMemNode(mem_range):
            node = FdtNode(f"memory@{int(mem_range.start):x}")
            node.append(FdtPropertyStrings("device_type", ["memory"]))
            node.append(
                FdtPropertyWords(
                    "reg",
                    state.addrCells(mem_range.start)
                    + state.sizeCells(mem_range.size()),
                )
            )
            # node.append(FdtPropertyWords("numa-node-id", [numa_node_id]))
            return node

        root = FdtNode("/")
        root.append(state.addrCellsProperty())
        root.append(state.sizeCellsProperty())

        # Add memory nodes. There are two memory ranges. One is the primary
        # range the other is the shared memory range, mounted on /dev/uio0
        assert len(self.mem_ranges) == 2
        
        for mem_range in self.mem_ranges:
            root.append(generateMemNode(mem_range))
        
        # Create a UIO node here
        # fix the addresses for now.
        # Can this range be cached? This will become the same as remote ranges.
        base_addr = 0x100000000
        uio_size = 0x80000000
        node = FdtNode(f"uio_device@{hex(base_addr)[2:]}")
        node.append(FdtPropertyStrings("compatible", ["generic-uio"]))
        node.append(
            FdtPropertyWords(
                "reg",
                state.addrCells(base_addr)
                + state.sizeCells(uio_size),
            )
        )
        node.append(FdtPropertyWords("uio,number-of-dynamic-regions", [1]))
        node.append(FdtPropertyWords("uio,dynamic-region-sizes", [0x4000]))
        # TODO: Figure out what these interrupts do.
        node.append(FdtPropertyWords("interrupts", [0, 10, 0]))
        root.append(node)

        for node in self.recurseDeviceTree(state):
            # Merge root nodes instead of adding them (for children
            # that need to add multiple root level nodes)
            if node.get_name() == root.get_name():
                root.merge(node)
            else:
                root.append(node)

        return root

    @overrides(ArmComposableMemoryBoard)
    def get_default_kernel_args(self) -> List[str]:
        # The default kernel string is taken from the devices.py file.
        return [
            "console=ttyAMA0",
            "lpj=19988480",
            "norandmaps",
            # "init=/root/gem5-init.sh",
            "root={root_value}",
            "rw",
            "mem=2G",
            "uio_pdrv_genirq.of_id=generic-uio",    # uio-pci-generic
        ]
