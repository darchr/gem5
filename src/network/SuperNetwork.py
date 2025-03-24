from m5.objects.ClockedObject import ClockedObject
from m5.params import *


class SuperNetwork(ClockedObject):
    type = "SuperNetwork"
    cxx_header = "network/SuperNetwork.hh"
    cxx_class = "gem5::SuperNetwork"

    # Vector of data cells in the network
    dataCells = VectorParam.DataCell("Data cells in the network")

    # Vector of layers in the network
    layers = VectorParam.Layer("Layers in the network")

    #   max_packets: 0 means not provided
    max_packets = Param.UInt64(
        0, "Maximum number of packets to schedule; 0 means not provided"
    )

    #   schedule_path: empty string means not provided
    schedule_path = Param.String(
        "", "File path for schedule (empty means not provided)"
    )
