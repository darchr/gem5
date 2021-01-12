# Copyright (c) 2020 UC regents
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

from m5.params import *
from m5.SimObject import SimObject
from m5.objects.ClockedObject import ClockedObject

class MemScheduler(ClockedObject):
    type = 'MemScheduler'
    cxx_header = "mem/mem_scheduler.hh"

    cpu_side = VectorResponsePort("CPU side port, receives requests")
    mem_side = VectorRequestPort("Ports on the memory "
                                        "controllers side")
    read_buffer_size = Param.Int(1, "Size of the internal read buffer")
    write_buffer_size = Param.Int(1, "Size of the internal "
                                    "write buffer")
    resp_buffer_size = Param.Int(8, "Size of the internal "
                                    "response buffer")
    service_write_threshold = Param.Percent(60, "Threshold to force writes")
    unified_queue = Param.Bool(True, "Use same queue for reads and "
                                     "write")
