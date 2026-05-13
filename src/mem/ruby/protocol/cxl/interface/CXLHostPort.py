from m5.objects.ClockedObject import ClockedObject
from m5.params import *


class CXLHostPort(ClockedObject):
    type = "CXLHostPort"
    cxx_header = "mem/ruby/protocol/cxl/interface/CXLHostPort.hh"
    cxx_class = "gem5::ruby::CXL::CXLHostPort"

    host_side_port = ResponsePort("Port to connect to the host side")
    host_binsp_port = RequestPort("Port to send back invalidate snoops")
    # import abstract controller
    ruby_system = Param.RubySystem(
        "The Ruby system that this port is connected to"
    )
    mem_ranges = VectorParam.AddrRange(
        "The physical memory range that corresponds to the remote CXL memory"
    )
    request_latency = Param.Int("Latency of requests")
    response_latency = Param.Int("Latency of responses")
    request_queue_size = Param.Int("Maximum size of the request queue")
    request_issue_width = Param.Int(
        "Amount of requests we can issue at the same time"
    )
