from m5.params import *
from m5.objects import SimObject


class LoopPointManager(SimObject):

    type = "LoopPointManager"
    cxx_header = "cpu/probes/looppointmanager.hh"
    cxx_class = "gem5::LoopPointManager"

    target_count = Param.Int(0, "the target PC ccount")

    def setup(self, cpulist, targetpc):
        for core in cpulist:
            core.core.addLoopPointProbe(targetpc, self)
