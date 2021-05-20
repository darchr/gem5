from __future__ import print_function
from __future__ import absolute_import

from math import log

from m5.objects import *
from m5.util.convert import *

from .MemInfo import *
from .TrafficGen import *

class CachelessSystem(System):
    def __init__(self, mem_type, num_chnls):
        super(CachelessSystem, self).__init__()
        self.initialize(mem_type, num_chnls)
        self.setupGenerator()
        self.createMemoryCtrl()
        self.setupInterconnect()
        self.connectComponents()

    def initialize(self, mem_type, num_chnls):
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = "4GHz"
        self.clk_domain.voltage_domain = VoltageDomain()

        self._mem_type = mem_type
        self._num_chnls = num_chnls
        self._mem_size = self.getMemSize()
        self.mem_ranges = [AddrRange(self._mem_size)]
        self.mem_mode = 'timing'
        self.mmap_using_noreserve = True
        self.cache_line_size = self.getCachelineSize()
        self._mem_name = mem_info[mem_type]['gname']

    def getCachelineSize(self):
        return mem_info[self._mem_type]['cache_line_size']

    def getMemSize(self):
        return str(mem_info[self._mem_type]['channel_cap']
        * self._num_chnls) + 'MB'

    def setupGenerator(self):
        self.generator = PyTrafficGen()

    def createMemoryCtrl(self):
        mem_ctrls = []
        mem_name = self._mem_name
        num_chnls = self._num_chnls
        addr_range = self.mem_ranges[0]
        addr_map = mem_name.addr_mapping
        intlv_size = self.cache_line_size

        cls_d = mem_name
        cls_n = mem_info['NVM']['gname']

        #addr_map == "RoRaBaCoCh"
        intlv_low_bit = int(log(intlv_size, 2))
        intlv_bits = int(log(num_chnls, 2))

        for chnl in range(num_chnls):
            interfaceD = cls_d()
            interfaceD.range = AddrRange(
                addr_range.start,
                size=addr_range.size(),
                intlvHighBit=intlv_low_bit + intlv_bits - 1,
                xorHighBit=0,
                intlvBits=intlv_bits,
                intlvMatch=chnl,
            )

            interfaceN = cls_n()
            interfaceN.range = AddrRange(
                addr_range.start,
                size=addr_range.size(),
                intlvHighBit=intlv_low_bit + intlv_bits - 1,
                xorHighBit=0,
                intlvBits=intlv_bits,
                intlvMatch=chnl,
            )

            ctrl = MemCtrl()
            ctrl.dram = interfaceD
            ctrl.nvm  = interfaceN
            ctrl.dram.null = True

            mem_ctrls.append(ctrl)

        self.mem_ctrls = mem_ctrls

    def setupInterconnect(self):
        self.interconnect = NoncoherentXBar(
            width=64, forward_latency=1, frontend_latency=1, response_latency=1
        )
        self.system_port = self.interconnect.cpu_side_ports

    def connectComponents(self):
        self.generator.port = self.interconnect.cpu_side_ports
        for mem_ctrl in self.mem_ctrls:
            mem_ctrl.port = self.interconnect.mem_side_ports

    def startLinearTraffic(self, gen_options):
        gen_options.min_addr = 0
        gen_options.max_addr = toMemorySize(self._mem_size)
        self.generator.start(createLinearTraffic(self.generator, gen_options))

    def startRandomTraffic(self, gen_options):
        gen_options.min_addr = 0
        gen_options.max_addr = toMemorySize(self._mem_size)
        self.generator.start(createRandomTraffic(self.generator, gen_options))
