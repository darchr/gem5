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


import m5
from m5.objects import BaseCPU

from typing import Dict, Any, List

from .abstract_processor import AbstractProcessor
from ..boards.abstract_board import AbstractBoard
from ..utils.override import *


class SwitchableProcessor(AbstractProcessor):
    """
    This class can be used to setup a switchable processor/processors on a
    system.

    In practise this "processor" functions as a wrapper around the processors
    that are to exist at the start of an simulation, and those that are to be
    switched in later.

    Though this class can be used directly, it is best inherited from. See
    "SimpleSwitchableCPU" for an example of this.
    """

    def __init__(
        self,
        switchable_processors: Dict[Any, AbstractProcessor],
        starting_processor_key: Any,
    ) -> None:

        if starting_processor_key not in switchable_processors.keys():
            raise AssertionError(
                f"Key {starting_processor_key} cannot be found in the"
                " switchable_processors dictionary."
            )

        self._current_processor = switchable_processors[starting_processor_key]
        self._switchable_processors = switchable_processors

        # Set the processor's "switched_out" status.
        for cpu in self._current_processor.get_cpu_simobjects():
            cpu.switched_out = False

        for proc in self._switchable_processors.values():
            if proc is not self._current_processor:
                for cpu in proc.get_cpu_simobjects():
                    cpu.switched_out = True

        super(SwitchableProcessor, self).__init__(
            cpu_type=self._current_processor.get_cpu_type(),
            num_cores=self._current_processor.get_num_cores(),
        )

    @overrides(AbstractProcessor)
    def get_cpu_simobjects(self) -> List[BaseCPU]:
        return self._current_processor.get_cpu_simobjects()

    @overrides(AbstractProcessor)
    def incorporate_processor(self, board: AbstractBoard) -> None:
        self._current_processor.incorporate_processor(board=board)

        # TODO: This these swichable processors should be set as a child of
        # this class. Though, for some reason, gem5 doesn't like this
        # "SubSystem within a SubSystem" design. Therefore, for the meantime,
        # the switchable processors are children of the board (System).
        index = 0
        for proc in self._switchable_processors.values():
            setattr(board, "switchable_proc" + str(index), proc)
            index += 1

        # This is a bit of a hack. The `m5.switchCpus` function, used in the
        # "switch_to_processor" function, requires the System simobject as an
        # argument. We therefore need to store the motherboard when
        # incorporating the procsesor
        self._board = board

    def switch_to_processor(self, switchable_processor_key: Any):

        # Run various checks.
        if not hasattr(self, "_board"):
            raise AssertionError("The processor has not been incorporated.")

        if switchable_processor_key not in self._switchable_processors.keys():
            raise AssertionError(
                f"Key {switchable_processor_key} is not a key in the"
                " switchable_processor dictionary."
            )

        # Select the correct processor to switch to.
        to_switch = self._switchable_processors[switchable_processor_key]

        # Run more checks.
        if to_switch == self._current_processor:
            raise AssertionError(
                "Cannot swap current processor with current processor"
            )

        if (
            to_switch.get_num_cores()
            != self._current_processor.get_num_cores()
        ):
            raise AssertionError(
                "The number of cores in the processor to swap in is not the"
                " current processor. This is not allowed."
            )

        for cpu in to_switch.get_cpu_simobjects():
            if not cpu.switchedOut():
                raise AssertionError(
                    "The cpu is not switched out. It therefore cannot be"
                    " switched in."
                )

        # Switch the CPUs
        m5.switchCpus(
            self._board,
            list(
                zip(
                    self._current_processor.get_cpu_simobjects(),
                    to_switch.get_cpu_simobjects(),
                )
            ),
        )

        # Ensure the current processor is updated.
        self._current_processor = to_switch
        self._cpu_type = to_switch.get_cpu_type()
        self._num_cores = to_switch.get_num_cores()
