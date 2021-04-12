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

from abc import ABC, abstractmethod
from ..processors.abstract_switched_out_processor import \
    AbstractSwitchedOutProcessor
from ..processors.abstract_processor import AbstractProcessor

from typing import Dict, Any, List

import m5
from m5.objects import System

class AbstractwitchableProcessors(ABC):
    """
    An interface for motherboard to inherit from. This allows the motherboard's
    processors to be swapped for a different type at a specified point within
    a simulation.

    This is useful when needing to jump between a "fast" CPU type (e.g., KVM)
    and a "detailed" CPU type (e.g., Timing) during different regions of
    interest.
    """

    def __init__(self, current_processor: AbstractProcessor):
        self._current_processor = current_processor

    def swap_processor_to(self, to_swap: AbstractSwitchedOutProcessor):
        if to_swap == self._current_processor:
            # TODO: Probably should be a warning here. There's no point
            # swapping a processor out with itself.
            return

        if to_swap.get_num_cores() != self._current_processor.get_num_cores():
            # TODO: See if there is a more appropriate Error type we can raise.
            raise AssertionError("The number of cores in the processor to "
                                 "swap in is not the current processor. This "
                                 "is not allowed.")
        
        m5.switchCpus(self, list(zip(
            self._current_processor.get_cpu_simobjects,
            to_swap.get_cpu_simobjects()
                                    )
                                )
                      )
        self._current_processor = to_swap
