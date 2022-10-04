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

import m5
import argparse

from math import log
from m5.objects import *

def interleave_addresses(plain_range, num_channels, cache_line_size):
        intlv_low_bit = log(cache_line_size, 2)
        intlv_bits = log(num_channels, 2)
        ret = []
        for i in range(num_channels):
            ret.append(AddrRange(
                start=plain_range.start,
                size=plain_range.size(),
                intlvHighBit=intlv_low_bit + intlv_bits - 1,
                xorHighBit=0,
                intlvBits=intlv_bits,
                intlvMatch=i))
        return ret

class GPT(SubSystem):
    def __init__(self, edge_memory_size: str, cache_size: str):
        super().__init__()
        self.wl_engine = WLEngine(
                                update_queue_size=2,
                                register_file_size=2
                                )
        self.coalesce_engine = CoalesceEngine(
                                            attached_memory_atom_size=32,
                                            cache_size=cache_size,
                                            num_mshr_entry=32,
                                            num_tgts_per_mshr=32,
                                            max_resp_per_cycle=4
                                            )
        self.push_engine = PushEngine(
                                    push_req_queue_size=32,
                                    attached_memory_atom_size=64,
                                    resp_queue_size=64,
                                    update_queue_size=2
                                    )

        self.vertex_mem_ctrl = MemCtrl(dram=HBM_1000_4H_1x128(burst_length=2))

        self.edge_mem_ctrl = MemCtrl(dram=DDR4_2400_8x8(
                                            range=AddrRange(edge_memory_size),
                                            in_addr_map=False
                                                    )
                                    )

        self.coalesce_engine.mem_port = self.vertex_mem_ctrl.port
        self.push_engine.mem_port = self.edge_mem_ctrl.port

        self.mpu = MPU(
                    wl_engine=self.wl_engine,
                    coalesce_engine=self.coalesce_engine,
                    push_engine=self.push_engine
                    )

    def getRespPort(self):
        return self.wl_engine.in_ports
    def setRespPort(self, port):
        self.wl_engine.in_ports = port

    def getReqPort(self):
        return self.push_engine.out_ports
    def setReqPort(self, port):
        self.push_engine.out_ports = port

    def set_vertex_range(self, vertex_range):
        self.vertex_mem_ctrl.dram.range = vertex_range
    def set_edge_image(self, edge_image):
        self.edge_mem_ctrl.dram.image_file = edge_image

class SEGA(System):
    def __init__(self, num_mpus, cache_size, graph_path):
        super(SEGA, self).__init__()
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = '1GHz'
        self.clk_domain.voltage_domain = VoltageDomain()
        self.cache_line_size = 32
        self.mem_mode = "timing"

        self.ctrl = CenteralController(image_file=f"{graph_path}/vertices")

        vertex_ranges = interleave_addresses(
                                        AddrRange(start=0, size="4GiB"),
                                        num_mpus,
                                        32
                                        )

        gpts = []
        for i in range(num_mpus):
            gpt = GPT("8GiB", cache_size)
            gpt.set_vertex_range(vertex_ranges[i])
            gpt.set_edge_image(f"{graph_path}/edgelist_{i}")
            gpts.append(gpt)
        # Creating the interconnect among mpus
        for gpt_0 in gpts:
            for gpt_1 in gpts:
                gpt_0.setReqPort(gpt_1.getRespPort())
        self.gpts = gpts

        self.ctrl.mpu_vector = [gpt.mpu for gpt in self.gpts]

    def create_initial_bfs_update(self, init_addr, init_value):
        self.ctrl.createInitialBFSUpdate(init_addr, init_value)

def get_inputs():
    argparser = argparse.ArgumentParser()
    argparser.add_argument("num_gpts", type=int)
    argparser.add_argument("cache_size", type=str)
    argparser.add_argument("graph", type=str)
    argparser.add_argument("init_addr", type=int)
    argparser.add_argument("init_value", type=int)

    args = argparser.parse_args()

    return args.num_gpts, args.cache_size, \
        args.graph, args.init_addr, args.init_value

if __name__ == "__m5_main__":
    num_gpts, cache_size, graph, init_addr, init_value = get_inputs()

    system = SEGA(num_gpts, cache_size, graph)
    root = Root(full_system = False, system = system)

    m5.instantiate()

    system.create_initial_bfs_update(init_addr, init_value)
    exit_event = m5.simulate()
    print(f"Exited simulation at tick {m5.curTick()} " + \
            f"because {exit_event.getCause()}")
