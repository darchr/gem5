# Copyright (c) 2022 The Regents of the University of California.
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
#

from m5.params import *
from m5.proxy import *

from m5.objects.ClockedObject import ClockedObject
from m5.objects.SimObject import SimObject


class DRTraceReader(SimObject):
    """This is a trace reader which is shared between possibly many different
    trace players.
    """

    type = "DRTraceReader"
    cxx_class = "gem5::DRTraceReader"
    cxx_header = "cpu/testers/dr_trace_player/trace_reader.hh"

    directory = Param.String("Directory which contains the memtrace files")

    num_players = Param.Int("Total number of players connected to this reader")

    # Since this is the thing which sees the timestamps, this is what will do
    # the "context switches." So, if we want to add different context switch
    # options, this is the place.


class DRTracePlayer(ClockedObject):
    """This is a trace player object. One of these represents a "core."

    You can limit the amount of ILP by using the `max_ipc` and/or the
    `max_outstanding_reqs` parameters. If these are set to 0, then there is no
    limit.

    This model assumes that instructions are executed in order, except for
    memory instructions. I.e., instructions are executed sequentially, the
    memory instructions are sent to memory sequentially, but other
    instructions can execute (including memory instructions) while prior
    instructions are waiting for memory.

    When used with caches, `send_data` should be true.

    The addresses are virtual-ish addresses in the trace. Currently, there is
    no address translation. Instead, if the backing address space (i.e., main
    memory) has a limited range you should set the `compress_address_range` to
    be the backing memory's address range.
    You can set memory range to be much larger when using `is_null=True` on
    the abstract memory.
    """

    type = "DRTracePlayer"
    cxx_class = "gem5::DRTracePlayer"
    cxx_header = "cpu/testers/dr_trace_player/trace_player.hh"

    # Port used for sending requests and receiving responses
    port = RequestPort("This port sends requests and receives responses")

    # System used to determine the mode of the memory system
    system = Param.System(Parent.any, "System this generator is part of")

    reader = Param.DRTraceReader("The reader for this player")

    max_ipc = Param.Int(
        1,
        "Max number of instructions per cycle. Zero means no limit.",
    )

    max_outstanding_reqs = Param.Int(
        16,
        "Max number of memory instructions outstanding. Zero means no limit.",
    )

    send_data = Param.Bool(
        False,
        "If true, this player will send dummy data on writes and make space "
        "for reads. If false, system.memory.null should be true.",
    )

    compress_address_range = Param.AddrRange(
        0, "Compress the addresses into the given range if valid."
    )
