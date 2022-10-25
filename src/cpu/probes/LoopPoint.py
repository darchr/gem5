# Copyright (c) 2022 The Regents of the University of California
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
from m5.objects.Probe import ProbeListenerObject
from m5.objects import SimObject


class LoopPointManager(SimObject):

    type = "LoopPointManager"
    cxx_header = "cpu/probes/looppointmanager.hh"
    cxx_class = "gem5::LoopPointManager"

    target_count = VectorParam.Int("the target PC count")
    target_pc = VectorParam.Addr("the target PC")
    region_id = VectorParam.Int("the simulation region id")
    relative_pc = VectorParam.Addr("the relative PC of the target PC")
    relative_count = VectorParam.Int("the relative PC count of the target PC")


class LoopPoint(ProbeListenerObject):

    type = "LoopPoint"
    cxx_header = "cpu/probes/looppoint.hh"
    cxx_class = "gem5::LoopPoint"

    target_pc = VectorParam.Addr("the target PC")
    core = Param.BaseCPU("the connected cpu")
    lpmanager = Param.LoopPointManager("the looppoint manager")
