

from m5.params import *
from m5.proxy import *

from Device import BasicPioDevice
from X86TLB import X86TLB
from Process import EmulatedDriver

class Daxpy(BasicPioDevice):
    type = 'Daxpy'
    cxx_header = "accel/daxpy.hh"

    memory_port = MasterPort("Port to memory")
    system = Param.System(Parent.any, "system object")

    max_unroll = Param.Int(8, "Max number of concurrent iterations of loop")

    tlb = Param.X86TLB(X86TLB(), "TLB/MMU to walk page table")

class DaxpyDriver(EmulatedDriver):
    type = 'DaxpyDriver'
    cxx_header = "accel/daxpy.hh"
    filename = "daxpy"

    hardware = Param.Daxpy("The daxpy hardware")
