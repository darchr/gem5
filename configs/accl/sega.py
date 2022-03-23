import m5
from m5.objects import *

class MPU(SubSystem):
    def __init__(self):
        super(MPU, self).__init__()
        self.push_engine = PushEngine(base_edge_addr=0x100000, push_req_queue_size = 16)
        self.coalesce_engine = CoalesceEngine(peer_push_engine=self.push_engine)
        self.wl_engine = WLEngine(coalesce_engine=self.coalesce_engine, update_queue_size = 16, on_the_fly_update_map_size=8)
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

    def getVertexMemPort(self):
        return self.coalesce_engine.mem_port
    def setVertexMemPort(self, port):
        self.coalesce_engine.mem_port = port

    def getEdgeMemPort(self):
        return self.push_engine.mem_port
    def setEdgeMemPort(self, port):
        self.push_engine.mem_port = port

class SEGA(System):
    def __init__(self):
        super(SEGA, self).__init__()

        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = '2GHz'
        self.clk_domain.voltage_domain = VoltageDomain()

        self.mpu = MPU()
        self.mem_ctrl = SimpleMemory(range=AddrRange("4GiB"), bandwidth="1000GB/s", latency = "30ns")
        # self.mem_ctrl = MemCtrl()
        # self.mem_ctrl.dram = DDR4_2400_8x8(range=AddrRange(start=0x000000, size="1MiB"))
        # self.mem_ctrl.nvm = NVM_2400_1x64(range=AddrRange(start=0x100000, size="1MiB"))
        self.mpu.setReqPort(self.mpu.getRespPort())
        self.mpu.setMemPort(self.mem_ctrl.port)

system = SEGA()
root = Root(full_system = False, system = system)

m5.instantiate()

exit_event = m5.simulate()
print("Simulation finished!")
exit()
