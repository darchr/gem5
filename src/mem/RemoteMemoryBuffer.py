from m5.params import *
from m5.SimObject import SimObject


class RemoteMessageMemory(SimObject):
    type = "RemoteMessageMemory"
    cxx_header = "mem/remote_message_memory.hh"

    num_fifos = Param.Unsigned(8, "Number of FIFO buffers")
    addr_base = Param.Addr(0x70000000, "Base address")
    addr_stride = Param.Unsigned(0x40, "Stride per FIFO address")
    buffer_depth = Param.Unsigned(32, "Number of messages stored in hot queue")
    max_msg_size = Param.Unsigned(256, "Maximum message size (bytes)")
    latency = Param.Cycles(20, "Fixed service latency")
    spill_enable = Param.Bool(
        True, "Enable writeback to main memory when full"
    )

    spill_base = Param.Addr(
        0x80000000, "Base address in DRAM for spill region"
    )
    spill_span = Param.Unsigned(
        1 << 20, "Bytes reserved for each FIFO spill region"
    )

    port_type = "mem"
    cpu_side_ports = VectorPort("CPU-side ports (upstream)")
    mem_side_ports = VectorPort("Memory-side ports (downstream for spills)")
