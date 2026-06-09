from m5.objects.SimpleMemory import SimpleMemory
from m5.params import *
from m5.proxy import *


class CxlHardwareBuffer(SimpleMemory):
    type = "CxlHardwareBuffer"
    cxx_header = "dev/cxl_hw_buffer.hh"
    cxx_class = "gem5::CxlHardwareBuffer"

    # Inherited from SimpleMemory/AbstractMemory:
    #   range       - address range (replaces pio_addr + pio_size)
    #   latency     - base memory latency (default 30ns)
    #   bandwidth   - memory bandwidth
    #   port        - ResponsePort for memory hierarchy connection
    #   in_addr_map - register with PhysicalMemory (default True)
    #   kvm_map     - auto-register as KVM memory slot (default True)

    # this is the latency used to model CXL network time + logic
    transfer_latency = Param.Latency(
        "150ns", "Latency of off-host memory transfer"
    )

    # this is used for setup, we know when to switch modes when we
    # see a file opened for each endpoing
    num_endpoints = Param.Unsigned(1, "Number of MPI endpoints/ranks")

    # helps us figure out offsets of SPSC queues
    segment_size = Param.MemorySize(
        "16MB", "Size of each endpoint's memory segment"
    )

    # size of a cell, currently static
    slot_size = Param.MemorySize("4kB", "Size of an individual message slot")

    # Physical size of the actual MPSC queue, this in real hardware
    # would need actual memory cells
    mpsc_size = Param.MemorySize(
        "1MB", "Size of the MPSC central queue region"
    )

    # deprecated since adding checkpoint restore support
    rank_offsets = VectorParam.Addr(
        [],
        "List of internal PA offsets for initialized ranks for "
        "checkpoint restore",
    )
