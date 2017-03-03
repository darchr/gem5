
from m5.params import *
from MemObject import MemObject

class RandomDelayBridge(MemObject):
    type = 'RandomDelayBridge'
    cxx_header = "mem/random_delay_bridge.hh"
    slave = SlavePort('Slave port')
    master = MasterPort('Master port')

    max_delay = Param.Latency('0ns', "Maximum random delay added to packets")
