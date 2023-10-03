from m5.params import *
from m5.proxy import * 
from m5.objects.ClockedObject import ClockedObject


class MessageQueue(ClockedObject):
    type = 'MessageQueue'
    cxx_header = "mem/message_queue.hh"
    cxx_class = "gem5::MessageQueue"
    # can either have a vector of ports connecting to the cores, or have a vector of write ports and 1 read port
    # if using a xbar just need one or 2 ports
    # Vector
    cpu_side = ResponsePort("Core side ports, receives updates")
    mem_side = RequestPort("Core fake port")
    queueSize = Param.Int(1024, "Max size a queue can be")
    my_range = Param.AddrRange("Address range of queue")
    # port_connection_count = Param.Int("how many ports")