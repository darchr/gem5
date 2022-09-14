from m5.params import *
from m5.objects.Probe import ProbeListenerObject


class LoopPoint(ProbeListenerObject):

    type = "LoopPoint"
    cxx_header = "cpu/probes/looppoint.hh"
    cxx_class = "gem5::LoopPoint"

    target_pc = Param.UInt64(0, "the target PC")
    target_count = Param.Int(0, "the target PC ccount")
    core = Param.BaseCPU(NULL, "the connected cpu")
