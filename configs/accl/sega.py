import m5
import argparse

from math import log
from m5.objects import *
from m5.util.convert import toMemorySize

class MPU(SubSystem):
    def __init__(self):
        super(MPU, self).__init__()
        self.push_engine = PushEngine(push_req_queue_size=32,
                                    attached_memory_atom_size=64,
                                    resp_queue_size=64)
        # self.push_engine = PushEngine(base_edge_addr=base_edge_addr,
        #                             push_req_queue_size=32,
        #                             attached_memory_atom_size=64,
        #                             resp_queue_size=64)
        self.coalesce_engine = CoalesceEngine(
                                    peer_push_engine=self.push_engine,
                                    attached_memory_atom_size=32,
                                    cache_size="8MiB",
                                    num_mshr_entry=32,
                                    num_tgts_per_mshr=16)
        self.wl_engine = WLEngine(coalesce_engine=self.coalesce_engine,
                                update_queue_size=64,
                                register_file_size=32)

    def getRespPort(self):
        return self.wl_engine.resp_port
    def setRespPort(self, port):
        self.wl_engine.resp_port = port

    def getReqPort(self):
        return self.push_engine.req_port
    def setReqPort(self, port):
        self.push_engine.req_port = port

    def getVertexMemPort(self):
        return self.coalesce_engine.mem_port
    def setVertexMemPort(self, port):
        self.coalesce_engine.mem_port = port

    def getEdgeMemPort(self):
        return self.push_engine.mem_port
    def setEdgeMemPort(self, port):
        self.push_engine.mem_port = port

class MPUMemory(SubSystem):
    def __init__(self,
                    num_channels: int,
                    cache_line_size: int,
                    vertex_memory_size: str,
                    edge_memory_size: str,
                    graph_path: str):
        super(MPUMemory, self).__init__()

        self._vertex_ranges = self._interleave_addresses(
                                AddrRange(start=0, size=vertex_memory_size),\
                                num_channels,\
                                cache_line_size)

        self._edge_chunk_size = int(\
                                toMemorySize(edge_memory_size)/num_channels)
        self._edge_ranges = [AddrRange(\
                            start=toMemorySize(vertex_memory_size)+\
                            self._edge_chunk_size*i,\
                            size=self._edge_chunk_size)\
                            for i in range(num_channels)]

        vertex_mem_ctrl = []
        edge_mem_ctrl = []
        for i in range(num_channels):
            vertex_mem_ctrl.append(
                SimpleMemory(range=self._vertex_ranges[i],
                            bandwidth="19.2GB/s",
                            latency="30ns")
            )
            edge_mem_ctrl.append(
                # SimpleMemory(range=self._edge_ranges[i],
                #             bandwidth="4.8GB/s",
                #             latency="30ns",
                #             image_file=f"{graph_path}/edgelist_{i}")
                SimpleMemory(range=AddrRange(self._edge_chunk_size),
                            bandwidth="4.8GB/s",
                            latency="30ns",
                            image_file=f"{graph_path}/edgelist_{i}",
                            in_addr_map=False)
            )
        self.vertex_mem_ctrl = vertex_mem_ctrl
        self.edge_mem_ctrl = edge_mem_ctrl

    def _interleave_addresses(self,
                            plain_range,
                            num_channels,
                            cache_line_size):
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

    def getVertexPort(self, i):
        return self.vertex_mem_ctrl[i].port
    def setVertexPort(self, port, i):
        self.vertex_mem_ctrl[i].port = port

    def getEdgeBaseAddr(self, i):
        return self._edge_ranges[i].start
    def getEdgePort(self, i):
        return self.edge_mem_ctrl[i].port
    def setEdgePort(self, port, i):
        self.edge_mem_ctrl[i].port = port

class SEGA(System):
    def __init__(self,
                num_mpus,
                vertex_cache_line_size,
                graph_path,
                first_addr,
                first_value):
        super(SEGA, self).__init__()
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = '1GHz'
        self.clk_domain.voltage_domain = VoltageDomain()
        self.cache_line_size = vertex_cache_line_size
        self.mem_mode = "timing"

        self.interconnect = NoncoherentXBar(frontend_latency=1,
                                            forward_latency=1,
                                            response_latency=1,
                                            width=64)

        self.ctrl = CenteralController(addr=first_addr, value=first_value,
                                    image_file=f"{graph_path}/vertices")
        self.ctrl.req_port = self.interconnect.cpu_side_ports

        self.mem_ctrl = MPUMemory(
                            num_mpus,
                            self.cache_line_size,
                            "2GiB",
                            "14GiB",
                            graph_path)

        mpus = []
        for i in range(num_mpus):
            mpus.append(MPU())
            mpus[i].setReqPort(self.interconnect.cpu_side_ports)
            mpus[i].setRespPort(self.interconnect.mem_side_ports)
            mpus[i].setVertexMemPort(self.mem_ctrl.getVertexPort(i))
            mpus[i].setEdgeMemPort(self.mem_ctrl.getEdgePort(i))
        self.mpu = mpus

def get_inputs():
    argparser = argparse.ArgumentParser()
    argparser.add_argument("num_mpus", type=int)
    argparser.add_argument("vertex_cache_line_size", type=int)
    argparser.add_argument("graph_path", type=str)
    argparser.add_argument("init_addr", type=int)
    argparser.add_argument("init_value", type=int)
    args = argparser.parse_args()
    return args.num_mpus, args.vertex_cache_line_size, \
            args.graph_path, args.init_addr, args.init_value

if __name__ == "__m5_main__":
    num_mpus, vertex_cache_line_size, \
        graph_path, first_addr, first_value = get_inputs()

    print(f"Creating a system with {num_mpus} mpu(s) and graph {graph_path}")
    system = SEGA(num_mpus, vertex_cache_line_size, \
                graph_path, first_addr, first_value)
    root = Root(full_system = False, system = system)

    m5.instantiate()

    exit_event = m5.simulate()
    print(f"Exited simulation because {exit_event.getCause()}")
    exit()
