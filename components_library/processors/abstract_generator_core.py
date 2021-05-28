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


from m5.objects import Port
from ..utils.override import overrides

from .cpu_types import CPUTypes
from .abstract_core import AbstractCore


class AbstractGeneratorCore(AbstractCore):
    """The abstract generator core

    Generator cores are cores that can replace the processing cores to allow
    for testing computer systems in gem5. The abstract class
    AbstractGeneratorCore defines the external interface that every generator
    core must implement. Certain generator cores might need to extend this
    interface to fit their requirements.
    """

    def __init__(self):
        """
        Create an AbstractCore with the CPUType of Timing.
        """
        # TODO: Remove the CPU Type parameter
        super(AbstractGeneratorCore, self).__init__(CPUTypes.TIMING)


    @overrides(AbstractCore)
    def connect_icache(self, port: Port) -> None:
        """
        Generator cores only have one request port which we will connect to
        the data cache not the icache. Just pass here.
        """
        pass

    @overrides(AbstractCore)
    def connect_walker_ports(self, port1: Port, port2: Port) -> None:
        """
        Since generator cores are not used in full system mode, no need to
        connect them to walker ports. Just pass here.
        """
        pass

    @overrides(AbstractCore)
    def set_workload(self, process: "Process") -> None:
        """
        Generator cores do not need any workload assigned to them, as they
        generate their own synthetic workload (synthetic traffic). Just pass
        here.

        :param process: The process to execute during simulation.
        """
        pass

    @overrides(AbstractCore)
    def connect_interrupt(
        self, interrupt_requestor: Port, interrupt_responce: Port
    ) -> None:
        """
        Since generator cores are not used in full system mode, no need to
        connect them to walker ports. Just pass here.
        """
        pass

