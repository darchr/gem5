import m5
from m5.objects import *

class MPU(SubSystem):
    def __init__(self):
        super(MPU, self).__init__()
        self.push_engine = PushEngine()
        self.apply_engine = ApplyEngine(push_engine = self.push_engine)
        self.wl_engine = WLEngine(apply_engine = self.apply_engine)
        self.interconnect = SystemXBar()

        self.interconnect.cpu_side_ports = self.wl_engine.mem_port
        self.interconnect.cpu_side_ports = self.apply_engine.mem_port
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

class SEGA(System):
    def __init__(self):
        super(SEGA, self).__init__()

        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = '2GHz'
        self.clk_domain.voltage_domain = VoltageDomain()

        self.mpu = MPU()
        self.mem_ctrl = MemCtrl(dram = DDR4_2400_8x8(range=AddrRange("4GiB")))

        self.mpu.setReqPort(self.mpu.getRespPort())
        self.mpu.setMemPort(self.mem_ctrl.port)

system = SEGA()
root = Root(full_system = False, system = system)

m5.instantiate()

exit_event = m5.simulate()
print("Simulation finished!")
exit()
