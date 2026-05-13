from m5.objects.Device import BasicPioDevice
from m5.params import *
from m5.proxy import *


class CxlHardwareBuffer(BasicPioDevice):
    type = "CxlHardwareBuffer"
    cxx_header = "dev/cxl_hw_buffer.hh"
    cxx_class = "gem5::CxlHardwareBuffer"

    pio_size = Param.MemorySize("Size of the PioDevice mapped range")

    transfer_latency = Param.Latency(
        "150ns", "Latency of off-host memory transfer"
    )

    num_endpoints = Param.Unsigned(1, "Number of MPI endpoints/ranks")
    segment_size = Param.MemorySize(
        "16MB", "Size of each endpoint's memory segment"
    )
    slot_size = Param.MemorySize("4kB", "Size of an individual message slot")
    mpsc_size = Param.MemorySize(
        "1MB", "Size of the MPSC central queue region"
    )

    rank_offsets = VectorParam.Addr(
        [],
        "List of internal PA offsets for initialized ranks for "
        "checkpoint restore",
    )
