# Copyright (c) 2021 The Regents of the University of California
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

from abc import ABCMeta, abstractmethod

from m5.defines import buildEnv
from m5.objects import System, Port, IOXBar, ClockDomain

from .isas import ISA
from .coherence_protocol import CoherenceProtocol

from typing import List


class AbstractBoard(System):
    """The abstract board interface.

    Boards are used as the object which can connect together all other system
    components. This abstract class defines the external interface that other
    boards must provide. Boards can be specialized for different ISAs or system
    designs (e.g., core counts, cache types, memory channels, I/O devices, etc)

    In addition to providing the place that system components are connected,
    the board also exposes an interface for the caches, processor, and memory
    to interact.

    The board also exposes an interface to set up I/O devices which needs to be
    specialized for each ISA and/or platform.
    """

    __metaclass__ = ABCMeta

    def __init__(
        self,
        processor: "AbstractProcessor",
        memory: "AbstractMemory",
        cache_hierarchy: "AbstractCacheHierarchy",
    ) -> None:
        super(AbstractBoard, self).__init__()
        """
        :param processor: The processor for this board.
        :param memory: The memory for this board.
        :param cache_hierarchy: The Cachie Hierarchy for this board.
        """

        self.processor = processor
        self.memory = memory
        self.cache_hierarchy = cache_hierarchy

    def get_processor(self) -> "AbstractProcessor":
        """Get the processor connected to the board.

        :returns: The processor.
        """
        return self.processor

    def get_memory(self) -> "AbstractMemory":
        """Get the memory (RAM) connected to the board.

        :returns: The memory system.
        """
        return self.memory

    def get_cache_hierarchy(self) -> "AbstractCacheHierarchy":
        """Get the cache hierarchy connected to the board.

        :returns: The cache hierarchy.
        """
        return self.cache_hierarchy

    def get_dma_ports(self) -> List[Port]:
        """Get the board's Direct Memory Access ports.
        This abstract method must be implemented within the subclasses if they
        support DMA and/or full system simulation.

        :returns: A List of the Direct Memory Access ports.

        """
        raise NotImplementedError

    def get_io_bus(self) -> IOXBar:
        """Get the board's IO Bus.
        This abstract method must be implemented within the subclasses if they
        support DMA and/or full system simulation.

        The I/O bus is a non-coherent bus (in the classic caches). On the CPU
        side, it accepts requests meant for I/O devices. On the memory side, it
        forwards these requests to the devices (e.g., the interrupt
        controllers on each core).

        :returns: The I/O Bus
        """
        raise NotImplementedError

    @abstractmethod
    def get_clock_domain(self) -> ClockDomain:
        raise NotImplementedError

    @abstractmethod
    def connect_system_port(self, port: Port) -> None:
        raise NotImplementedError

    @abstractmethod
    def connect_things(self) -> None:
        """Connects all the components to the board.

        This should be called after the constructor.

        When implementing this function, derived boards should use this to
        hook up the memory, process, and cache hierarchy as a *second* stage.
        You should use this function to connect things together when you need
        to know that everything has already been constructed.
        """
        raise NotImplementedError

    def get_runtime_isa(self) -> ISA:
        """Gets the target ISA.
        This can be inferred at runtime.

        :returns: The target ISA.
        """
        isa_map = {
            "sparc": ISA.SPARC,
            "mips": ISA.MIPS,
            "null": ISA.NULL,
            "arm": ISA.ARM,
            "x86": ISA.X86,
            "power": ISA.POWER,
            "riscv": ISA.RISCV,
        }

        isa_str = str(buildEnv["TARGET_ISA"]).lower()
        if isa_str not in isa_map.keys():
            raise NotImplementedError(
                "ISA '" + buildEnv["TARGET_ISA"] + "' not recognized."
            )

        return isa_map[isa_str]

    def get_runtime_coherence_protocol(self) -> CoherenceProtocol:
        """Gets the cache coherence protocol.
        This can be inferred at runtime.

        :returns: The cache coherence protocol.
        """
        protocol_map = {
            "mi_example": CoherenceProtocol.MI_EXAMPLE,
            "moesi_hammer": CoherenceProtocol.ARM_MOESI_HAMMER,
            "garnet_standalone": CoherenceProtocol.GARNET_STANDALONE,
            "moesi_cmp_token": CoherenceProtocol.MOESI_CMP_TOKEN,
            "mesi_two_level": CoherenceProtocol.MESI_TWO_LEVEL,
            "moesi_amd_base": CoherenceProtocol.MOESI_AMD_BASE,
            "mesi_three_level_htm": CoherenceProtocol.MESI_THREE_LEVEL_HTM,
            "mesi_three_level": CoherenceProtocol.MESI_THREE_LEVEL,
            "gpu_viper": CoherenceProtocol.GPU_VIPER,
            "chi": CoherenceProtocol.CHI,
        }

        protocol_str = str(buildEnv["PROTOCOL"]).lower()
        if protocol_str not in protocol_map.keys():
            raise NotImplementedError(
                "Protocol '" + buildEnv["PROTOCOL"] + "' not recognized."
            )

        return protocol_map[protocol_str]
