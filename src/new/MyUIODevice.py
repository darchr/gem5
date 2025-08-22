# src/dev/MyUIODevice.py
from m5.params import *
from m5.SimObject import SimObject


class MyUIODevice(SimObject):
    type = "MyUIODevice"
    cxx_header = "new/my_uio_device.cc"
    cxx_class = "MyUIODevice"
    pio_addr = Param.Addr(0x10000000, "Base address")
    pio_size = Param.Addr(0x1000, "Size of address range")
