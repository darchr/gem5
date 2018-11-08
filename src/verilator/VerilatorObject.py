from m5.params import *
from m5.SimObject import SimObject

class VerilatorObject(SimObject):
    type = 'VerilatorObject'
    cxx_header = "verilator/verilator_object.hh"

    cycles = Param.Int(0, \
    "Maximum number of cycles to simulate, 0 means unlimited")
    latency = Param.Latency("Clock period of device under test")
    stages = Param.Int(1, "Number of stages in the device under test")
    startTime = Param.Int(0, "Time to start simulation")
    memData = Param.String("Path to data to be loaded in device memory")
    name = Param.String("VerilatorObject","Object Name")
    instPort = Param.MasterPort("Instruction Port")
    dataPort = Param.MasterPort("Data Port")
