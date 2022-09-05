import m5
import argparse

from math import log
import math
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
    argparser.add_argument("num_mpus", type=int)
    argparser.add_argument("cache_size", type=str)
    argparser.add_argument("graph_path", type=str)
    argparser.add_argument("init_addr", type=int)
    argparser.add_argument("init_value", type=int)
    args = argparser.parse_args()

    return args.num_mpus, args.cache_size, \
            args.graph_path, args.init_addr, args.init_value

if __name__ == "__m5_main__":
    num_mpus, cache_size, graph_path, first_addr, first_value = get_inputs()

    print(f"Creating a system with {num_mpus} mpu(s) and graph {graph_path}")
    system = SEGA(num_mpus, cache_size, graph_path, first_addr, first_value)
    root = Root(full_system = False, system = system)

    m5.instantiate()

    exit_event = m5.simulate()
    print(f"Exited simulation at tick {m5.curTick()} because {exit_event.getCause()}")
