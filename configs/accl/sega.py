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
import os
import argparse
import subprocess

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
        self.wl_engine = WLEngine(update_queue_size=32,
                                register_file_size=32)
        self.coalesce_engine = CoalesceEngine(attached_memory_atom_size=32,
                                            cache_size=cache_size,
                                            num_mshr_entry=32,
                                            num_tgts_per_mshr=32,
                                            max_resp_per_cycle=4)
        self.push_engine = PushEngine(push_req_queue_size=32,
                                    attached_memory_atom_size=64,
                                    resp_queue_size=64)

        self.vertex_mem_ctrl = MemCtrl(dram=HBM_1000_4H_1x128(burst_length=2))

        self.edge_mem_ctrl = MemCtrl(dram=DDR4_2400_8x8(
                                            range=AddrRange(edge_memory_size),
                                            in_addr_map=False))

        self.coalesce_engine.mem_port = self.vertex_mem_ctrl.port
        self.push_engine.mem_port = self.edge_mem_ctrl.port

        self.mpu = MPU(wl_engine=self.wl_engine,
                    coalesce_engine=self.coalesce_engine,
                    push_engine=self.push_engine)

    def getRespPort(self):
        return self.mpu.in_port
    def setRespPort(self, port):
        self.mpu.in_port = port

    def getReqPort(self):
        return self.mpu.out_port
    def setReqPort(self, port):
        self.mpu.out_port = port

    def set_vertex_range(self, vertex_range):
        self.vertex_mem_ctrl.dram.range = vertex_range
    def set_edge_image(self, edge_image):
        self.edge_mem_ctrl.dram.image_file = edge_image

class SEGA(System):
    def __init__(self,
                num_mpus,
                cache_size,
                graph_path,
                first_addr,
                first_value):
        super(SEGA, self).__init__()
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = '1GHz'
        self.clk_domain.voltage_domain = VoltageDomain()
        self.cache_line_size = 32
        self.mem_mode = "timing"

        self.interconnect = NoncoherentXBar(frontend_latency=1,
                                            forward_latency=1,
                                            response_latency=1,
                                            width=64)

        self.ctrl = CenteralController(addr=first_addr, value=first_value,
                                    image_file=f"{graph_path}/vertices")
        self.ctrl.req_port = self.interconnect.cpu_side_ports

        vertex_ranges = interleave_addresses(
                            AddrRange(start=0, size="4GiB"),
                            num_mpus,
                            32)

        gpts = []
        for i in range(num_mpus):
            gpt = GPT("8GiB", cache_size)
            gpt.set_vertex_range(vertex_ranges[i])
            gpt.set_edge_image(f"{graph_path}/edgelist_{i}")
            gpt.setReqPort(self.interconnect.cpu_side_ports)
            gpt.setRespPort(self.interconnect.mem_side_ports)
            gpts.append(gpt)
        self.gpts = gpts

        self.ctrl.mpu_vector = [gpt.mpu for gpt in self.gpts]

def get_inputs():
    argparser = argparse.ArgumentParser()
    argparser.add_argument("num_gpts", type=int)
    argparser.add_argument("cache_size", type=str)
    argparser.add_argument("vertex_cache_line_size", type=int)
    argparser.add_argument("synthetic", type=bool)
    argparser.add_argument("--scale", type=int)
    argparser.add_argument("--deg", type=int)
    argparser.add_argument("--graph", type=str)
    argparser.add_argument("init_addr", type=int)
    argparser.add_argument("init_value", type=int)

    args = argparser.parse_args()

    if args.synthetic:
        if (args.scale is None) or (args.deg is None):
            raise ValueError("If synthetic is true, you should specify the"
                        "scale of the graph by --scale [scale] and the average"
                        "degree of the graph by --deg [average degree].")
    else:
        if args.graph is None:
            raise ValueError("If synthetic is false, you should specify the "
                        "path to graph binaries by --graph [path to graph].")
    return args

if __name__ == "__m5_main__":
    input_args = get_inputs()

    image_path = None
    if input_args.synthetic:
        base_dir = os.environ.get("GRAPH_DIR", default="/tmp")
        graph_gen = os.path.abspath(os.environ.get("GRAPH_GEN"))
        graph_reader = os.environ.get("GRAPH_READER")
        graph_sorter = os.environ.get("GRAPH_SORTER")
        if graph_gen is None:
            raise ValueError(f"No value for $GRAPH_GEN.")
        if graph_reader is None:
            raise ValueError(f"No value for $GRAPH_READER.")
        if graph_sorter is None:
            raise ValueError(f"No value for $GRAPH_SORTER")

        graph_path = os.path.join(base_dir, f"graph_{input_args.scale}_{input_args.deg}")
        if not os.path.exists(graph_path):
            print(f"{graph_path} does not exist already.")
            os.mkdir(graph_path)
            print(f"Created {graph_path}")

        if not "graph.txt" in os.listdir(graph_path):
            print(f"graph.txt not found in {graph_path}")
            subprocess.run([f"{graph_gen}",
                            f"{input_args.scale}",
                            f"{input_args.deg}",
                            f"{graph_path}/graph_unordered.txt"])
            print(f"Generated a graph with scale "
                f"{input_args.scale} and deg {input_args.deg}")
            subprocess.run(["python",
                            f"{graph_sorter}",
                            f"{graph_path}/graph_unordered.txt",
                            f"{graph_path}/graph.txt"])
            print(f"Sorted the graph here {graph_path}/graph_unordered.txt"
                                    f" and saved in {graph_path}/graph.txt")
            subprocess.run(["rm", f"{graph_path}/graph_unordered.txt"])
            print(f"Deleted {graph_path}/graph_unordered.txt")

        if not "binaries" in os.listdir(graph_path):
            print(f"binaries directory not found in {graph_path}")
            os.mkdir(f"{graph_path}/binaries")
            print(f"Created {graph_path}/binaries")

        if not f"gpts_{input_args.num_gpts}" in os.listdir(f"{graph_path}/binaries"):
            print(f"gpts_{input_args.num_gpts} not found in {graph_path}/binaries")
            os.mkdir(f"{graph_path}/binaries/gpts_{input_args.num_gpts}")
            print(f"Created {graph_path}/binaries/gpts_{input_args.num_gpts}")

        expected_bins = ["vertices"] + [f"edgelist_{i}" for i in range(input_args.num_gpts)]
        if not all([binary in os.listdir(f"{graph_path}/binaries/gpts_{input_args.num_gpts}") for binary in expected_bins]):
            print(f"Not all expected binaries found in {graph_path}/binaries/gpts_{input_args.num_gpts}")
            for delete in os.scandir(f"{graph_path}/binaries/gpts_{input_args.num_gpts}"):
                os.remove(delete.path)
            print(f"Deleted all the files in {graph_path}/binaries/gpts_{input_args.num_gpts}")
            subprocess.run([f"{graph_reader}" ,
                            f"{graph_path}/graph.txt",
                            "false",
                            f"{input_args.num_gpts}",
                            f"{input_args.vertex_cache_line_size}",
                            f"{graph_path}/binaries/gpts_{input_args.num_gpts}"])
            print(f"Created the graph binaries in "
                    f"{graph_path}/binaries/gpts_{input_args.num_gpts}")
        image_path = f"{graph_path}/binaries/gpts_{input_args.num_gpts}"
    else:
        image_path = input_args.graph

    system = SEGA(input_args.num_gpts,
                input_args.cache_size,
                image_path,
                input_args.init_addr,
                input_args.init_value)
    root = Root(full_system = False, system = system)

    m5.instantiate()

    exit_event = m5.simulate()
    print(f"Exited simulation at tick {m5.curTick()} because {exit_event.getCause()}")
