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

from math import log
from m5.objects import *


def interleave_addresses(plain_range, num_channels, cache_line_size):
    intlv_low_bit = log(cache_line_size, 2)
    intlv_bits = log(num_channels, 2)
    ret = []
    for i in range(num_channels):
        ret.append(
            AddrRange(
                start=plain_range.start,
                size=plain_range.size(),
                intlvHighBit=intlv_low_bit + intlv_bits - 1,
                xorHighBit=0,
                intlvBits=intlv_bits,
                intlvMatch=i,
            )
        )
    return ret


class GPT(SubSystem):
    def __init__(self, register_file_size: int, cache_size: str):
        super().__init__()
        self.wl_engine = WLEngine(
            update_queue_size=64, register_file_size=register_file_size
        )
        self.coalesce_engine = CoalesceEngine(
            attached_memory_atom_size=32,
            cache_size=cache_size,
            max_resp_per_cycle=8,
            pending_pull_limit=64,
            active_buffer_size=80,
            post_push_wb_queue_size=64,
        )
        self.push_engine = PushEngine(
            push_req_queue_size=32,
            attached_memory_atom_size=64,
            resp_queue_size=4096,
            max_propagates_per_cycle=8,
            update_queue_size=32,
        )

        self.vertex_mem_ctrl = SimpleMemory(
            latency="122ns", latency_var="0ns", bandwidth="28GiB/s"
        )
        self.coalesce_engine.mem_port = self.vertex_mem_ctrl.port

        self.mpu = MPU(
            wl_engine=self.wl_engine,
            coalesce_engine=self.coalesce_engine,
            push_engine=self.push_engine,
        )

    def getRespPort(self):
        return self.wl_engine.in_ports

    def setRespPort(self, port):
        self.wl_engine.in_ports = port

    def getReqPort(self):
        return self.push_engine.out_ports

    def setReqPort(self, port):
        self.push_engine.out_ports = port

    def getEdgeMemPort(self):
        return self.push_engine.mem_port

    def setEdgeMemPort(self, port):
        self.push_engine.mem_port = port

    def set_vertex_range(self, vertex_range):
        self.vertex_mem_ctrl.range = vertex_range


class EdgeMemory(SubSystem):
    def __init__(self, size: str):
        super(EdgeMemory, self).__init__()
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = "2.4GHz"
        self.clk_domain.voltage_domain = VoltageDomain()

        self.mem_ctrl = MemCtrl(
            dram=DDR4_2400_8x8(range=AddrRange(size), in_addr_map=False)
        )
        self.xbar = NoncoherentXBar(
            width=64, frontend_latency=1, forward_latency=1, response_latency=1
        )
        self.xbar.mem_side_ports = self.mem_ctrl.port

    def set_image(self, image):
        self.mem_ctrl.dram.image_file = image

    def getPort(self):
        return self.xbar.cpu_side_ports

    def setPort(self, port):
        self.xbar.cpu_side_ports = port


class SEGA(System):
    def __init__(self, num_gpts, num_registers, cache_size, graph_path):
        super(SEGA, self).__init__()
        # num_gpts should be an even power of 2
        assert num_gpts != 0
        assert num_gpts % 2 == 0
        assert (num_gpts & (num_gpts - 1)) == 0

        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = "2GHz"
        self.clk_domain.voltage_domain = VoltageDomain()
        self.cache_line_size = 32
        self.mem_mode = "timing"

        # Building the CenteralController
        self.ctrl = CenteralController(
            vertex_image_file=f"{graph_path}/vertices"
        )
        # Building the EdgeMemories
        edge_mem = []
        for i in range(int(num_gpts / 2)):
            mem = EdgeMemory("16GiB")
            mem.set_image(f"{graph_path}/edgelist_{i}")
            edge_mem.append(mem)
        self.edge_mem = edge_mem
        # Building the GPTs
        vertex_ranges = interleave_addresses(
            AddrRange(start=0, size="4GiB"), num_gpts, 32
        )
        gpts = []
        for i in range(num_gpts):
            gpt = GPT(num_registers, cache_size)
            gpt.set_vertex_range(vertex_ranges[i])
            gpt.setEdgeMemPort(
                self.edge_mem[i % (int(num_gpts / 2))].getPort()
            )
            gpts.append(gpt)
        # Creating the interconnect among mpus
        for gpt_0 in gpts:
            for gpt_1 in gpts:
                gpt_0.setReqPort(gpt_1.getRespPort())
        self.gpts = gpts

        self.ctrl.mpu_vector = [gpt.mpu for gpt in self.gpts]

    def work_count(self):
        return self.ctrl.workCount()

    def set_async_mode(self):
        self.ctrl.setAsyncMode()

    def set_bsp_mode(self):
        self.ctrl.setBSPMode()

    def create_pop_count_directory(self, atoms_per_block):
        self.ctrl.createPopCountDirectory(atoms_per_block)

    def create_bfs_workload(self, init_addr, init_value):
        self.ctrl.createBFSWorkload(init_addr, init_value)

    def create_bfs_visited_workload(self, init_addr, init_value):
        self.ctrl.createBFSVisitedWorkload(init_addr, init_value)

    def create_sssp_workload(self, init_addr, init_value):
        self.ctrl.createSSSPWorkload(init_addr, init_value)

    def create_cc_workload(self):
        self.ctrl.createCCWorkload()

    def create_pr_workload(self, alpha):
        self.ctrl.createPRWorkload(alpha)

    def create_bc_workload(self, init_addr, init_value):
        self.ctrl.createBCWorkload(init_addr, init_value)

    def print_answer(self):
        self.ctrl.printAnswerToHostSimout()
