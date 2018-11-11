from m5.params import *
from MemObject import MemObject

class VerilatorObject(MemObject):
    type = 'VerilatorObject'
    cxx_header = "verilator/verilator_object.hh"

    latency = Param.Latency("Clock period of device under test")
    stages = Param.Int(1, "Number of stages in the device under test")
    instPort = MasterPort("Instruction Port")
    dataPort = MasterPort("Data Port")
