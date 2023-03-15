from m5.params import *
from m5.objects.Probe import ProbeListenerObject
from m5.objects import SimObject
from m5.util.pybind import *

class PcCountAnalsis(ProbeListenerObject):

    type = "PcCountAnalsis"
    cxx_header = "cpu/simple/probes/pc_count_anaylsis.hh"
    cxx_class = "gem5::PcCountAnalsis"

    ptmanager = Param.PcCountAnalsisManager("the PcCountAnalsi manager")

class PcCountAnalsisManager(SimObject):

    type = "PcCountAnalsisManager"
    cxx_header = "cpu/simple/probes/pc_count_anaylsis.hh"
    cxx_class = "gem5::PcCountAnalsisManager"

    cxx_exports = [
        PyBindMethod("getPcCount")
    ]
