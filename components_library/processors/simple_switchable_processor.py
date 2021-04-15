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

from .switchable_processor import SwitchableProcessor
from .simple_processor import SimpleProcessor

from ..utils.override import *

class SimpleSwitchableProcessor(SwitchableProcessor):
    """
    A Simplified implementation of SwitchableProcessor where there is one
    processor at the start of the simuation, and another that can be switched
    to via the "switch" function later in the simulation. This is good for
    fast/detailed CPU setups.
    """

    def __init__(self, starting_processor: SimpleProcessor,
                       switchable_processor: SimpleProcessor) -> None:
        self._start_key = "start"
        self._switch_key = "switch"

        self._current_processor_is_start = True

        super(SimpleSwitchableProcessor, self).__init__(
            switchable_processors={self._switch_key : switchable_processor,
                                   self._start_key : starting_processor},
            starting_processor_key=self._start_key,
        )

        if starting_processor.get_num_cores() != \
            switchable_processor.get_num_cores():

            # TODO: This shouldn't be an assertion error. Figure out what this
            # should be.
            raise AssertionError("The starting_processor does not have the "
                                 "same number of CPUs as the "
                                 "switchable_processor")

    def switch(self):
        if self._current_processor_is_start:
            self.switch_to_processor(self._switch_key)
        else:
            self.switch_to_processor(self._start_key)

        self._current_processor_is_start = not self._current_processor_is_start

