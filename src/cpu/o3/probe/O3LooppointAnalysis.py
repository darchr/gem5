from m5.params import *
from m5.objects.Probe import ProbeListenerObject
from m5.objects import SimObject
from m5.util.pybind import *

class O3LooppointAnalysis(ProbeListenerObject):

    type = "O3LooppointAnalysis"
    cxx_header = "cpu/o3/probe/o3looppoint_analysis.hh"
    cxx_class = "gem5::o3::O3LooppointAnalysis"

    ptmanager = Param.O3LooppointAnalysisManager("the PcCountAnalsi manager")
    validAddrRangeStart = Param.Addr(0, "the starting address of the valid "
                                     "insturction address range")
    validAddrRangeSize = Param.Addr(0, "the size of the valid address range")

class O3LooppointAnalysisManager(SimObject):

    type = "O3LooppointAnalysisManager"
    cxx_header = "cpu/o3/probe/o3looppoint_analysis.hh"
    cxx_class = "gem5::o3::O3LooppointAnalysisManager"

    cxx_exports = [
        PyBindMethod("getCounter"),
        PyBindMethod("getPcCount"),
        PyBindMethod("getMostRecentPc"),
        PyBindMethod("getCurrentPc")
    ]
