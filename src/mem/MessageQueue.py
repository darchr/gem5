from m5.objects.ClockedObject import ClockedObject
from m5.params import *
from m5.proxy import *


class MessageQueue(ClockedObject):
    type = "MessageQueue"
    cxx_header = "mem/message_queue.hh"
    cxx_class = "gem5::MessageQueue"
    # can either have a vector of ports connecting to the cores, or have a vector of write ports and 1 read port
    # if using a xbar just need one or 2 ports
    # Vector
    cpu_side = ResponsePort("Core side ports, receives updates")
    queueSize = Param.Int(1024, "Max size a queue can be")
    myRange = Param.AddrRange("Address range of queue")
    # port_connection_count = Param.Int("how many ports")

    def get_range(self):
        return self.myRange

    def get_cpu_side(self):
        return self.cpu_side
