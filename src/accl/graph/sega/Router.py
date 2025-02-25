from m5.params import *
from m5.proxy import *
from m5.objects.ClockedObject import ClockedObject


class Router(ClockedObject):
    type = "Router"
    cxx_header = "accl/graph/sega/router.hh"
    cxx_class = "gem5::Router"

    in_ports = VectorResponsePort(
        "Incoming Ports to receive updates from " "remote outside"
    )

    out_ports = VectorRequestPort(
        "Outgoing Ports to send updates to " "remote outside"
    )
