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
from typing import Sequence,Tuple

# all the source files are one directory above.
sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
)

from memories.external_remote_memory import ExternalRemoteMemory
from cachehierarchies.dm_caches import ClassicPrivateL1PrivateL2SharedL3DMCache
from cachehierarchies.saga.cache_hierarchy import SagaCacheHierarchy
from cachehierarchies.cmn.cache_hierarchy import *

from gem5.components.memory import SingleChannelDDR4_2400
from gem5.isas import ISA
from gem5.components.boards.arm_board import ArmBoard
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.utils.override import overrides

from m5.objects.ArmFsWorkload import ArmFsLinux
from m5.objects.ArmSystem import ArmDefaultRelease
from m5.util.fdthelper import (
    FdtNode,
    FdtPropertyStrings,
    FdtPropertyWords,
)
from m5.objects import (
    AddrRange,
    ArmDefaultRelease,
    ArmFsLinux,
    ArmSystem,
    BadAddr,
    Bridge,
    GenericTimer,
    IOXBar,
    Port,
    SimObject,
    SrcClockDomain,
    Terminal,
    VExpress_GEM5_V1,
    VncServer,
    VoltageDomain,
)
class ArmComposableMemoryBoard(ArmBoard):
    """
    A high-level ARM board that is zNUMA-capable systems with a remote
    memories. This board is extended from the ArmBoard from Gem5 standard
    library. This board assumes full-system simulation booting Linux.

    **Limitations**
    * kvm is only supported in a gem5-only setup.

    @params
    :use_sst: This is a boolean to indicate if the external memory is simulated in SST or in gem5.
              When taking a checkpoint, everything is simulated in gem5.
              When restoring a checkpoint, the simulation is done in SST and gem5.
    :remote_memory_address_range: Use this to force map the remote memory
              address range when using stdlib DRAM/memory interfaces.
    :local_memory_size: Size of the local memory. Default is 8GiB.
              Depending on the memory allocation policy and the experiment,
              the size may change. Thus, we have a parameter to set the size.
    """

    def __init__(
        self,
        use_sst: bool = False,
        remote_memory_address_range: AddrRange = None,
        local_memory_size: str = "8GiB",
    ) -> None:

        self._remoteMemoryAddressRange = remote_memory_address_range
        self._remote_memory = ExternalRemoteMemory(
            addr_range=remote_memory_address_range, use_sst_sim=use_sst
        )
        
        # We use KVM CPU for taking checkpoint in a gem5-only setup.
        # We use O3 CPU for restoring a checkpoint and run the actual
        # simulation in a gem5-SST setup.
        if use_sst == True:
            self._cpu_type = CPUTypes.O3
        else:
            self._cpu_type = CPUTypes.KVM

        # Initialize the ArmBoard with the following parameters.
        super().__init__(
            clk_freq="4GHz",
            processor=SimpleProcessor(cpu_type=self._cpu_type, isa=ISA.ARM, num_cores=8),
            memory=SingleChannelDDR4_2400(size=local_memory_size),
            cache_hierarchy=SagaCacheHierarchy(),
            platform=VExpress_GEM5_V1(),
            release=ArmDefaultRelease.for_kvm(),
        )
        # The board has a memory child. We take it as the local memory.
        self.local_memory = self.memory
        # Defining the remote memory instantiate above as a child of the board.
        self.remote_memory = self._remote_memory
        self._set_remote_memory_ranges()
        self.mem_ranges.append(self.get_remote_memory_addr_range())

    @overrides(ArmBoard)
    def get_memory(self) -> "AbstractMemorySystem":
        """Get the memory (RAM) connected to the board.

        :returns: The memory system.
        """
        raise NotImplementedError

    @overrides(ArmBoard)
    def get_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        mem_range_ports = []
        mem_range_ports.append(self.get_bootmem_ports())
        mem_range_ports.append(self.get_local_memory_ports())
        mem_range_ports.append(self.get_remote_memory_ports())
        return mem_range_ports

    def get_local_memory(self) -> "AbstractMemorySystem":
        """Get the memory (RAM) connected to the board.
        :returns: The local memory system.
        """
        # get local memory is called at init phase.
        return self.memory

    def get_remote_memory(self) -> "AbstractMemorySystem":
        """Get the memory (RAM) connected to the board.
            This has to be implemeted by the child class as we don't know if
            this board is simulating Gem5 memory or some external simulator
            memory.
        :returns: The remote memory system.
        """
        return self._remote_memory

    def get_remote_memory_size(self) -> "str":
        """Get the remote memory size to setup the NUMA nodes. Since the remote
            memory is an abstract memory system, we should be able to call its
            standard methods.
        :returns: The size of the remote memory system.
        """
        return self.get_remory_memory().get_size()

    def get_local_memory_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return self.get_local_memory().get_mem_ports()
    
    def get_remote_memory_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return self.get_remote_memory().get_mem_ports()
    
    def get_bootmem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        return [
            (self.realview.bootmem.range, self.realview.bootmem.port)
        ]
    
    def get_remote_mem_ports(self) -> Sequence[Tuple[AddrRange, Port]]:
        """Get the memory (RAM) ports connected to the board.
            This has to be implemeted by the child class as we don't know if
            this board is simulating Gem5 memory or some external simulator
            memory.
        :returns: A tuple of mem_ports.
        """
        return self.get_remote_memory().get_mem_ports()

    def get_remote_memory_addr_range(self):
        """Get the range of the remote memory. This can be omitted in the
            future iteration of the board.
        :returns: AddrRange of the remote memory
        """
        # Although this is hardcoded to return the first element, this is
        # always valid. This is how the standard library returns
        # get_mem_ports().
        if self._remoteMemoryAddressRange is None:
            return self.get_remote_mem_ports()[0][0]
        else:
            return self._remoteMemoryAddressRange

    def get_monolithic_range():
        """Get the range of the monolithic memory. This can be omitted in the
            future iteration of the board.
        :returns: AddrRange of the monolithic memory
        """
        size = self.get_local_memory().get_size() + self.get_remote_memory().get_size()
        return AddrRange(start=0, size=size)

    @overrides(ArmBoard)
    def _setup_io_devices(self) -> None:
        """
        This method first sets up the platform. ARM uses ``realview`` platform.
        Most of the on-chip and off-chip devices are setup by the realview
        platform. Once realview is setup, we connect the I/O devices to the
        I/O bus.
        """

        # Currently, the ArmBoard supports VExpress_GEM5_V1,
        # VExpress_GEM5_V1_HDLcd and VExpress_GEM5_Foundation.
        # VExpress_GEM5_V2 and VExpress_GEM5_V2_HDLcd are not supported by the
        # ArmBoard.
        self.realview = self._platform

        # We need to setup the global interrupt controller (GIC) addr for the
        # realview system.
        if hasattr(self.realview.gic, "cpu_addr"):
            self.gic_cpu_addr = self.realview.gic.cpu_addr

        # For KVM cpus, we need to simulate the GIC.
        if any(core.is_kvm_core() for core in self.processor.get_cores()):
            # The following is taken from
            # `tests/fs/linux/arm/configs/arm_generic.py`:
            # Arm KVM regressions will use a simulated GIC. This means that in
            # order to work we need to remove the system interface of the
            # generic timer from the DTB and we need to inform the MuxingKvmGic
            # class to use the gem5 GIC instead of relying on the host one
            GenericTimer.generateDeviceTree = SimObject.generateDeviceTree
            self.realview.gic.simulate_gic = True

        # IO devices has to setup before incorporating the caches in the case
        # of ruby caches. Otherwise the DMA controllers are incorrectly
        # created. The IO device has to be attached first. This is done in the
        # realview class.
        if self.get_cache_hierarchy().is_ruby():
            # All the on-chip devices are attached in this method.
            # mem_ports = []
            # mem_ports.append(self.get_bootmem_ports()[0])
            # mem_ports.append(self.get_local_memory_ports()[0])
            # mem_ports.append(self.get_remote_memory_ports()[0])
            self.realview.attachOnChipIO(
                self.iobus,
                dma_ports=self.get_dma_ports(),
                mem_ports= self.get_mem_ports(),
            )
            self.realview.attachIO(self.iobus, dma_ports=self.get_dma_ports())

        else:
            # We either have iocache or dmabridge depending upon the
            # cache_hierarchy. If we have "NoCache", then we use the dmabridge.
            # Otherwise, we use the iocache on the board.

            # We setup the iobridge for the ARM Board. The default
            # cache_hierarchy's NoCache class has an iobridge has a latency
            # of 10. We are using an iobridge with latency = 50ns, taken
            # from the configs/example/arm/devices.py.
            self.iobridge = Bridge(delay="50ns")
            self.iobridge.mem_side_port = self.iobus.cpu_side_ports
            self.iobridge.cpu_side_port = (
                self.cache_hierarchy.get_mem_side_port()
            )

            if isinstance(self.cache_hierarchy, NoCache) is True:
                # This corresponds to a machine without caches. We have a DMA
                # bridge in this case. Parameters of this bridge are also taken
                # from the common/example/arm/devices.py file.
                self.dmabridge = Bridge(delay="50ns", ranges=self.mem_ranges)
                self.dmabridge.mem_side_port = (
                    self.cache_hierarchy.get_cpu_side_port()
                )
                self.dmabridge.cpu_side_port = self.iobus.mem_side_ports

            # The classic caches are setup in the  _setup_io_cache() method
            # defined under the cachehierarchy class. Verified it with both
            # PrivateL1PrivateL2CacheHierarchy and PrivateL1CacheHierarchy
            # classes.
            self.realview.attachOnChipIO(
                self.cache_hierarchy.membus, self.iobridge
            )
            self.realview.attachIO(self.iobus)

    def _set_remote_memory_ranges(self):
        self.get_remote_memory().set_memory_range(
            [self.get_remote_memory_addr_range()]
        )

    @overrides(ArmBoard)
    def _setup_board(self) -> None:
        # This board is expected to run full-system simulation.
        # Loading ArmFsLinux() from `src/arch/arm/ArmFsWorkload.py`
        self.workload = ArmFsLinux()

        # We are fixing the following variable for the ArmSystem to work. The
        # security extension is checked while generating the dtb file in
        # realview. This board does not have security extension enabled.
        self._have_psci = False

        # highest_el_is_64 is set to True. True if the register width of the
        # highest implemented exception level is 64 bits.
        self.highest_el_is_64 = True

        # Setting up the voltage and the clock domain here for the ARM board.
        # The ArmSystem/RealView expects voltage_domain to be a parameter.
        # The voltage and the clock frequency are taken from the devices.py
        # file from configs/example/arm. We set the clock to the same frequency
        # as the user specified in the config script.
        self.voltage_domain = VoltageDomain(voltage="1.0V")
        self.clk_domain = SrcClockDomain(
            clock=self._clk_freq, voltage_domain=self.voltage_domain
        )

        # The ARM board supports both Terminal and VncServer.
        self.terminal = Terminal()
        self.vncserver = VncServer()

        # Incoherent I/O Bus
        self.iobus = IOXBar()
        self.iobus.badaddr_responder = BadAddr()
        self.iobus.default = self.iobus.badaddr_responder.pio

        # We now need to setup the dma_ports.
        self._dma_ports = None

        # RealView sets up most of the on-chip and off-chip devices and GIC
        # for the ARM board. These devices' information is also used to
        # generate the dtb file. We then connect the I/O devices to the
        # I/O bus.
        self._setup_io_devices()

        # Once the realview is setup, we can continue setting up the memory
        # ranges. ArmBoard's memory can only be setup once realview is
        # initialized.
        local_memory = self.get_local_memory()
        mem_size = local_memory.get_size()

        # The following code is taken from configs/example/arm/devices.py. It
        # sets up all the memory ranges for the board.
        self.mem_ranges = []
        success = False
        # self.mem_ranges.append(self.get_remote_memory_addr_range())
        for mem_range in self.realview._mem_regions:
            size_in_range = min(mem_size, mem_range.size())
            self.mem_ranges.append(
                AddrRange(start=mem_range.start, size=size_in_range)
            )
            mem_size -= size_in_range

            if mem_size == 0:
                success = True
                break

        if success:
            local_memory.set_memory_range(self.mem_ranges)
        else:
            raise ValueError("Memory size too big for platform capabilities")
        

        # The PCI Devices. PCI devices can be added via the `_add_pci_device`
        # function.
        self._pci_devices = []

    @overrides(ArmSystem)
    def generateDeviceTree(self, state):
        # Generate a device tree root node for the system by creating the root
        # node and adding the generated subnodes of all children.
        # When a child needs to add multiple nodes, this is done by also
        # creating a node called '/' which will then be merged with the
        # root instead of appended.

        def generateMemNode(numa_node_id, mem_range):
            node = FdtNode(f"memory@{int(mem_range.start):x}")
            node.append(FdtPropertyStrings("device_type", ["memory"]))
            node.append(
                FdtPropertyWords(
                    "reg",
                    state.addrCells(mem_range.start)
                    + state.sizeCells(mem_range.size()),
                )
            )
            node.append(FdtPropertyWords("numa-node-id", [numa_node_id]))
            return node

        root = FdtNode("/")
        root.append(state.addrCellsProperty())
        root.append(state.sizeCellsProperty())

        # Add memory nodes
        for mem_range in self.mem_ranges:
            root.append(generateMemNode(0, mem_range))
        root.append(generateMemNode(1, self.get_remote_memory_addr_range()))

        for node in self.recurseDeviceTree(state):
            # Merge root nodes instead of adding them (for children
            # that need to add multiple root level nodes)
            if node.get_name() == root.get_name():
                root.merge(node)
            else:
                root.append(node)

        return root

    @overrides(ArmBoard)
    def get_default_kernel_args(self) -> List[str]:
        # The default kernel string is taken from the devices.py file.
        return [
            "console=ttyAMA0",
            "lpj=19988480",
            "norandmaps",
            "root={root_value}",
            "rw",
        ]

    @overrides(ArmBoard)
    def _connect_things(self) -> None:
        """Connects all the components to the board.

        The order of this board is always:

        1. Connect the memory.
        2. Connect the cache hierarchy.
        3. Connect the processor.

        Developers may build upon this assumption when creating components.

        Notes
        -----

        * The processor is incorporated after the cache hierarchy due to a bug
        noted here: https://gem5.atlassian.net/browse/GEM5-1113. Until this
        bug is fixed, this ordering must be maintained.
        * Once this function is called `_connect_things_called` *must* be set
        to `True`.
        """

        if self._connect_things_called:
            raise Exception(
                "The `_connect_things` function has already been called."
            )

        # Incorporate the memory into the motherboard.
        self.get_local_memory().incorporate_memory(self)
        self.get_remote_memory().incorporate_memory(self)

        # Incorporate the cache hierarchy for the motherboard.
        self.get_cache_hierarchy().incorporate_cache(self)
            
        # Incorporate the processor into the motherboard.
        self.get_processor().incorporate_processor(self)
        # self.get_cache_hierarchy().l3.snoop_filter.max_capacity = "32MiB"

        self._connect_things_called = True

    @overrides(ArmBoard)
    def _post_instantiate(self):
        """Called to set up anything needed after m5.instantiate. The memory
        has been replaced with local and remote memories in this board."""
        self.get_processor()._post_instantiate()
        if self.get_cache_hierarchy():
            self.get_cache_hierarchy()._post_instantiate()
        self.get_local_memory()._post_instantiate()
        self.get_remote_memory()._post_instantiate()