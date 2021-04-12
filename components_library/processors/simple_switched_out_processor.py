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

from .abstract_swapped_out_processor import \
    AbstractAbstractSwitchedOutProcessor
from .simple_processor import SimpleProcessor

import ..utils.override

class SimpleSwitchedOutProcessor(AbstractAbstractSwitchedOutProcessor,
                                  SimpleProcessor):

    def __init__(self, cpu_type: CPUTypes, num_cores: int) -> None:
        super(SimpleSwitchedOutProcessor, self).__init__(
            cpu_type = cpu_type,
            num_cores = num_cores,
        )

    def __init__(self, processor: SimpleProcessor):
        """
        Returns the switched-out form of the current processor.
        """
        super(SimpleSwitchedOutProcessor, self).__init__(
            cpu_type = processor.get_cpu_type(),
            num_cores = processor.get_num_cores(),
        )

    @override(SimpleProcessor)
    def _create_cores(self, cpu_class: BaseCPU, num_cores: int):
        return [cpu_class(cpu_id = i, switched_out = True) 
                for i in range(num_cores)]