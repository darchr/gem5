from m5.params import *
from MemObject import MemObject

class VerilatorMemBlackBox(MemObject):
    type = 'VerilatorMemBlackBox'
    cxx_header = "verilator/VerilatorMemBlackBox.hh"

    instPort = MasterPort("Instruction Port")
    dataPort = MasterPort("Data Port")

