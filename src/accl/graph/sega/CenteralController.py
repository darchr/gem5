# -*- coding: utf-8 -*-
# Copyright (c) 2017 Jason Lowe-Power
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
from m5.proxy import *
from m5.util.pybind import PyBindMethod
from m5.objects.AbstractMemory import AbstractMemory
from m5.objects.BaseMemoryEngine import BaseMemoryEngine


class CenteralController(BaseMemoryEngine):
    type = "CenteralController"
    cxx_header = "accl/graph/sega/centeral_controller.hh"
    cxx_class = "gem5::CenteralController"

    mirrors_map_mem = RequestPort("Port to a memory storing mirrors map file.")

    choose_best = Param.Bool(
        "Whether to prefer the best update "
        "value for choosing the next slice"
    )

    vertex_image_file = Param.String("Path to the vertex image file.")

    mirrors_mem = Param.SimpleMemory("Memory to store the vertex mirrors.")

    mpu_vector = VectorParam.MPU("All mpus in the system.")

    edge_image_file = Param.String("Path to the edge image file.")
    
    abstract_mem_vector = VectorParam.AbstractMemory(
        "Abstract Memories to be intialized by edge_image_file."
    )
    abstract_mem_atom_size = Param.Int(
        64, "burst size of the abstract memories."
    )
    
    edge_base = Param.UInt64("Addr of base address range")
    
    

    cxx_exports = [
        PyBindMethod("setAsyncMode"),
        PyBindMethod("setBSPMode"),
        PyBindMethod("setPGMode"),
        PyBindMethod("createPopCountDirectory"),
        PyBindMethod("createBFSWorkload"),
        PyBindMethod("createBFSVisitedWorkload"),
        PyBindMethod("createSSSPWorkload"),
        PyBindMethod("createCCWorkload"),
        PyBindMethod("createAsyncPRWorkload"),
        PyBindMethod("createPRWorkload"),
        PyBindMethod("createBCWorkload"),
        PyBindMethod("workCount"),
        PyBindMethod("getPRError"),
        PyBindMethod("printAnswerToHostSimout"),
    ]
