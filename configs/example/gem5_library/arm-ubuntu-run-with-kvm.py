# Copyright (c) 2022-2025 The Regents of the University of California
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

"""
This script shows an example of booting an ARM based full system Ubuntu
disk image. This simulation boots the disk image using 2 KVM cores, then
switches to TIMING cores after the second hypercall exit, which occurs after
Ubuntu has successfully booted. The simulation ends soon after, when the
simulation reaches a `gem5-bridge hypercall 3` command in the script the
simulation runs after booting.

Usage
-----

```
scons build/ALL/gem5.opt -j<NUM_CPUS>
./build/ALL/gem5.opt configs/example/gem5_library/arm-ubuntu-run-with-kvm.py
```

"""

from m5.objects import (
    ArmDefaultRelease,
    ArmSystem,
    VExpress_GEM5_V1,
)

from gem5.components.boards.arm_board import ArmBoard
from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import (
    PrivateL1PrivateL2CacheHierarchy,
)
from gem5.components.memory import DualChannelDDR4_2400
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_switchable_processor import (
    SimpleSwitchableProcessor,
)
from gem5.isas import ISA
from gem5.resources.resource import (
    DiskImageResource,
    KernelResource,
    obtain_resource,
)
from gem5.simulate.exit_event import ExitEvent
from gem5.simulate.simulator import Simulator
from gem5.utils.override import overrides
from gem5.utils.requires import requires

# This runs a check to ensure the gem5 binary is compiled for ARM.
requires(isa_required=ISA.ARM)

from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import (
    PrivateL1PrivateL2CacheHierarchy,
)

# Here we setup the parameters of the l1 and l2 caches.
cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
    l1d_size="16KiB", l1i_size="16KiB", l2_size="256KiB"
)

# Memory: Dual Channel DDR4 2400 DRAM device.
memory = DualChannelDDR4_2400(size="2GiB")

# Here we set up the processor. This is a special switchable processor in which
# a starting core type and a switch core type must be specified. Once a
# configuration is instantiated a user may call `processor.switch()` or
# `simulator.switch_processor()` (if switching processors within a hypercall
# exit handler) to switch from the starting core types to the switch core
# types. In this simulation we start with KVM cores to simulate the OS boot,
# then switch to the Timing cores for the command we wish to run after boot.
processor = SimpleSwitchableProcessor(
    starting_core_type=CPUTypes.KVM,
    switch_core_type=CPUTypes.TIMING,
    isa=ISA.ARM,
    num_cores=1,
)

# The ArmBoard requires a `release` to be specified. This adds all the
# extensions or features to the system. We are setting this to for_kvm()
# to enable KVM simulation.
release = ArmDefaultRelease.for_kvm()

# The platform sets up the memory ranges of all the on-chip and off-chip
# devices present on the ARM system. ARM KVM only works with VExpress_GEM5_V1
# on the ArmBoard at the moment.
platform = VExpress_GEM5_V1()


from m5.util.fdthelper import (
    FdtNode,
    FdtProperty,
    FdtPropertyStrings,
    FdtPropertyWords,
)


class MyArmBoard(ArmBoard):
    @overrides(ArmSystem)
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
                    + state.sizeCells(mem_range.size() - 0x4000_0000),
                    # Remove the last 1GB of memory to avoid overlap with pmem
                )
            )
            return node

        root = FdtNode("/")
        root.append(state.addrCellsProperty())
        root.append(state.sizeCellsProperty())

        # Add memory nodes
        for mem_range in self.mem_ranges:
            root.append(generateMemNode(mem_range))

        for node in self.recurseDeviceTree(state):
            # Merge root nodes instead of adding them (for children
            # that need to add multiple root level nodes)
            if node.get_name() == root.get_name():
                root.merge(node)
            else:
                root.append(node)

        node = FdtNode("pmem@140000000")
        node.append(FdtPropertyStrings("compatible", ["pmem-region"]))
        node.append(
            FdtPropertyWords(
                "reg",
                state.addrCells(0x1_4000_0000) + state.sizeCells(0x4000_0000),
                # use the upper 1GB of memory for pmem
            )
        )
        node.append(FdtProperty("volatile"))
        root.append(node)

        return root


# Here we setup the board. The ArmBoard allows for Full-System ARM simulations.
board = MyArmBoard(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
    release=release,
    platform=platform,
)

# Here we set a full system workload. The "arm-ubuntu-24.04-boot-with-systemd"
# boots Ubuntu 24.04.
workload = obtain_resource(
    "arm-ubuntu-24.04-boot-with-systemd", resource_version="3.0.0"
)
board.set_workload(workload)

# Examples of how you can override the default hypercall exit handler
# behaviors.
# Exit handlers don't have to be specified in the config script if you don't
# want to modify/override their default behaviors. Below, we override the
# default after-boot exit handler to switch processors.

# You can inherit from either the class that handles a certain hypercall,
# or inherit directly from ExitHandler and specify a hypercall number.
# See src/python/gem5/simulate/exit_handler.py for more information on which
# handlers map to which hypercalls, and what the default behaviors are.


class CustomKernelBootedExitHandler(ExitHandler, hypercall_num=1):
    @overrides(ExitHandler)
    def _process(self, simulator: "Simulator") -> None:
        print("First exit: kernel booted")

    @overrides(ExitHandler)
    def _exit_simulation(self) -> bool:
        return False


class SwitchProcessorAfterBootExitHandler(AfterBootExitHandler):
    @overrides(AfterBootExitHandler)
    def _process(self, simulator: "Simulator") -> None:
        print("Second exit: Started `after_boot.sh` script")
        print("Switching to Timing CPU")
        simulator.switch_processor()

    @overrides(AfterBootExitHandler)
    def _exit_simulation(self) -> bool:
        return False


class AfterBootScriptExitHandler(ExitHandler, hypercall_num=3):
    @overrides(ExitHandler)
    def _process(self, simulator: "Simulator") -> None:
        print(f"Third exit: {self.get_handler_description()}")

    @overrides(ExitHandler)
    def _exit_simulation(self) -> bool:
        return True


simulator = Simulator(board=board)

simulator.run()
