from m5.params import *
from m5.objects.Probe import ProbeListenerObject


class LoopPoint(ProbeListenerObject):

    type = "LoopPoint"
    cxx_header = "cpu/probes/looppoint.hh"
    cxx_class = "gem5::LoopPoint"

    target_pc = VectorParam.UInt64(0, "the target PC")
    core = Param.BaseCPU(NULL, "the connected cpu")
    LPmanager = Param.LoopPointManager(NULL, "the looppoint manager")
