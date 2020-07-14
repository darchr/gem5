# -*- coding: utf-8 -*-
# Copyright (c) 2017 Jason Lowe-Power
# All rights reserved.
#

from m5.params import *
from m5.SimObject import SimObject

class HelloObject(SimObject):
    type = 'HelloObject'
    cxx_header = "learning_gem5/part2/hello_object.hh"

    time_to_wait = Param.Latency("Time before firing the event")
    number_of_fires = Param.Int(5, "Number of times to fire the event before "
                                    "goodbye")
    # first parameter is default value (1)
    goodbye_object = Param.GoodbyeObject("A goodbye object")
class GoodbyeObject(SimObject):
    type = 'GoodbyeObject'
    cxx_header = "learning_gem5/part2/goodbye_object.hh"

    buffer_size = Param.MemorySize('1kB', "Size of buffer to fill "
                                    "with goodbye")
    write_bandwidth = Param.MemoryBandwidth('100MB/s', "Bandwidth to fill "
                                            "the bugger")
