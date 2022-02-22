import m5
from m5.objects import *

class PyMPU(MPU):
    def __init__(self, clk_domain):
        super().__init__()
        self.clk_domain = clk_domain
        self.apply_engine = ApplyEngine()
        self.push_engine = PushEngine()
        self.wl_engine = WLEngine()

class SEGA(System):

    def __init__(self):
        super(SEGA, self).__init__()
        # Set up the clock domain and the voltage domain
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = '2GHz'
        self.clk_domain.voltage_domain = VoltageDomain()

        self.mpu = PyMPU(self.clk_domain)
        self.mem_ctrl = MemCtrl(dram = DDR4_2400_8x8(range=AddrRange("4GiB")))
        self.mpu.memPort = self.mem_ctrl.port
        self.mpu.reqPort = self.mpu.respPort


system = SEGA()
root = Root(full_system = False, system = system)

m5.instantiate()

exit_event = m5.simulate()
print("Simulation finished!")
exit()