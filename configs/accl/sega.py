import m5
from m5.objects import *

class MPU(SubSystem):
    def __init__(self):
        super(MPU, self).__init__()
        self.push_engine = PushEngine(base_edge_addr=0x80000000,
                                    push_req_queue_size = 16)
        self.coalesce_engine = CoalesceEngine(
                                    peer_push_engine=self.push_engine)
        self.wl_engine = WLEngine(coalesce_engine=self.coalesce_engine,
                                    update_queue_size = 16,
                                    on_the_fly_update_map_size=8)
        self.interconnect = SystemXBar()

        self.interconnect.cpu_side_ports = self.coalesce_engine.mem_port
        self.interconnect.cpu_side_ports = self.push_engine.mem_port

    def getRespPort(self):
        return self.wl_engine.resp_port
    def setRespPort(self, port):
        self.wl_engine.resp_port = port

    def getReqPort(self):
        return self.push_engine.req_port
    def setReqPort(self, port):
        self.push_engine.req_port = port

    def getMemPort(self):
        return self.interconnect.mem_side_ports
    def setMemPort(self, port):
        self.interconnect.mem_side_ports = port

class MPUMemory(SubSystem):
    def __init__(self, vertex_range, vertex_binary, edge_range, edge_binary):
        super(MPUMemory, self).__init__()
        self.vertex_mem_ctrl = SimpleMemory(
            range=vertex_range, bandwidth="25GB/s",
            latency="30ns", image_file=vertex_binary)
        self.edge_mem_ctrl = SimpleMemory(
            range=edge_range, bandwidth="25GB/s",
            latency="30ns", image_file=edge_binary)
        self.interconnect = SystemXBar()

        self.interconnect.mem_side_ports = self.vertex_mem_ctrl.port
        self.interconnect.mem_side_ports = self.edge_mem_ctrl.port

    def getPort(self):
        return self.interconnect.cpu_side_ports
    def setPort(self, port):
        self.interconnect.cpu_side_ports = port

class SEGA(System):
    def __init__(self):
        super(SEGA, self).__init__()
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = '1GHz'
        self.clk_domain.voltage_domain = VoltageDomain()

        self.mpu = MPU()
        self.mem_ctrl = MPUMemory(
            vertex_range=AddrRange(start=0x000000, size="2GiB"),
            vertex_binary="epinions/graph_binaries/vertices",
            edge_range=AddrRange(start=0x80000000, size="2GiB"),
            edge_binary="epinions/graph_binaries/edgelist_0")

        self.mpu.setReqPort(self.mpu.getRespPort())
        self.mpu.setMemPort(self.mem_ctrl.getPort())

system = SEGA()
root = Root(full_system = False, system = system)

m5.instantiate()

exit_event = m5.simulate()
print("Simulation finished!")
exit()
