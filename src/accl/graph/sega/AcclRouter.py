from m5.params import *
from m5.proxy import *
from m5.objects.ClockedObject import ClockedObject
from m5.util.pybind import PyBindMethod


class AcclRouter(ClockedObject):
    type = "AcclRouter"
    cxx_header = "accl/graph/sega/accl_router.hh"
    cxx_class = "gem5::AcclRouter"

    in_ports = VectorResponsePort(
        "Incoming Ports to receive updates from " "remote outside"
    )

    out_ports = VectorRequestPort(
        "Outgoing Ports to send updates to " "remote outside"
    )

    mpu_vector = VectorParam.MPU("All mpus in the system.")

    # SRNoC params
    crosspoint_delay = Param.Float(-1, "Crosspoint delay in picoseconds")
    merger_delay = Param.Float(-1, "Merger delay in picoseconds")
    splitter_delay = Param.Float(-1, "Splitter delay in picoseconds")
    circuit_variability = Param.Float(-1, "Circuit variability in picoseconds")
    variability_counting_network = Param.Float(
        -1, "Variability in counting network in picoseconds"
    )
    crosspoint_setup_time = Param.Float(
        -1, "Crosspoint setup time in picoseconds"
    )
    hold_time = Param.Float(-1, "Hold time in picoseconds")

    cxx_exports = [
        PyBindMethod("setStaticDelayMode"),
        PyBindMethod("setSRNoCMode"),
    ]
