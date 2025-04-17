# Copyright (c) 2025 The Regents of the University of California
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

from m5.objects.ClockedObject import ClockedObject
from m5.params import *
from m5.util.pybind import PyBindMethod


class Layer(ClockedObject):
    type = "Layer"
    cxx_header = "network/layer.hh"
    cxx_class = "gem5::Layer"

    # Vector of data cells in the network
    data_cells = VectorParam.DataCell("Data cells in the layer")

    #   max_packets: 0 means not provided
    max_packets = Param.Int(
        -1, "Maximum number of packets to schedule; -1 means infinite"
    )

    #   schedule_path: empty string means not provided
    schedule_path = Param.String(
        "", "File path for schedule (empty means not provided)"
    )

    crosspoint_delay = Param.Float(-1, "Crosspoint delay in picoseconds")
    merger_delay = Param.Float(-1, "Merger delay in picoseconds")
    splitter_delay = Param.Float(-1, "Splitter delay in picoseconds")
    circuit_variability = Param.Float(-1, "Circuit variability in picoseconds")
    variability_counting_network = Param.Float(
        -1, "Variability in counting network in picoseconds"
    )
    crosspoint_setup_time = Param.Float(
        -1, "Crosspoint setup time in picoseconds"
    )
    hold_time = Param.Float(-1, "Hold time in picoseconds")

    cxx_exports = [
        PyBindMethod("setRandomTrafficMode"),
        PyBindMethod("setHotspotTrafficMode"),
        PyBindMethod("setAllToAllTrafficMode"),
        PyBindMethod("setTornadoTrafficMode"),
        PyBindMethod("setShuffle"),
    ]
