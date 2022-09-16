from m5.params import *
from m5.objects import SimObject


class LoopPointManager(SimObject):

    type = "LoopPointManager"
    cxx_header = "cpu/probes/looppointmanager.hh"
    cxx_class = "gem5::LoopPointManager"

    target_count = VectorParam.Int(0, "the target PC count")
    target_pc = VectorParam.UInt64(0, "the target PC")

    def setup(self, cpulist):
        for core in cpulist:
            core.core.addLoopPointProbe(self.target_pc, self)
