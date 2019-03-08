from m5.params import *
from m5.SimObject import SimObject

class VerilatorDinoCPU(SimObject):
    type = 'VerilatorDinoCPU'
    cxx_header = "verilator/VerilatorDinoCPU.hh"

    latency = Param.Latency("Clock period of device under test")
    stages = Param.Int(1, "Number of stages in the device under test")
