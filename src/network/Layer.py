from m5.objects.ClockedObject import ClockedObject
from m5.params import *
from m5.proxy import *


class Layer(ClockedObject):
    type = "Layer"
    cxx_header = "network/Layer.hh"
    cxx_class = "gem5::Layer"

    # Layer parameters
    range_size = Param.UInt64("Range size of the layer (dynamic range)")
