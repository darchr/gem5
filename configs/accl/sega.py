import m5
from m5.objects import *

class MPU(SubSystem):
    def __init__(self, base_edge_addr):
        super(MPU, self).__init__()
        self.push_engine = PushEngine(base_edge_addr=base_edge_addr,
                                    push_req_queue_size=16,
                                    attached_memory_atom_size=64)
        self.coalesce_engine = CoalesceEngine(
                                    peer_push_engine=self.push_engine,
                                    attached_memory_atom_size=64)
        self.wl_engine = WLEngine(coalesce_engine=self.coalesce_engine,
                                update_queue_size=16,
                                on_the_fly_update_map_size=8)

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
    def __init__(self, vertex_range, vertex_binary, edge_range, edge_binary):
        super(MPUMemory, self).__init__()
        self.vertex_mem_ctrl = SimpleMemory(
            range=vertex_range, bandwidth="19.2GB/s",
            latency="30ns", image_file=vertex_binary)
        self.edge_mem_ctrl = SimpleMemory(
            range=edge_range, bandwidth="19.2GB/s",
            latency="30ns", image_file=edge_binary)

    def getVertexPort(self):
        return self.vertex_mem_ctrl.port
    def setVertexPort(self, port):
        self.vertex_mem_ctrl.port = port

    def getEdgePort(self):
        return self.edge_mem_ctrl.port
    def setEdgePort(self, port):
        self.edge_mem_ctrl.port = port

class SEGA(System):
    def __init__(self):
        super(SEGA, self).__init__()
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = '1GHz'
        self.clk_domain.voltage_domain = VoltageDomain()

        self.mpu = MPU(base_edge_addr=0x80000000)
        self.mem_ctrl = MPUMemory(
            vertex_range=AddrRange(start=0x000000, size="2GiB"),
            vertex_binary="graphs/epinions/graph_binaries/vertices",
            edge_range=AddrRange(start=0x80000000, size="2GiB"),
            edge_binary="graphs/epinions/graph_binaries/edgelist_0")

        self.mpu.setReqPort(self.mpu.getRespPort())
        self.mpu.setVertexMemPort(self.mem_ctrl.getVertexPort())
        self.mpu.setEdgeMemPort(self.mem_ctrl.getEdgePort())

system = SEGA()
root = Root(full_system = False, system = system)

m5.instantiate()

exit_event = m5.simulate()
print("Simulation finished!")
exit()
