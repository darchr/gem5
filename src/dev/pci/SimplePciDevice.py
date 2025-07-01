from m5.objects.PciDevice import PciDevice
from m5.params import *
from m5.SimObject import SimObject


class SimplePciDevice(PciDevice):
    type = "SimplePciDevice"
    cxx_header = "dev/pci/simple_pci_device.hh"
    cxx_class = "gem5::SimplePciDevice"

    pio_addr = Param.Addr(0x10000000, "PIO base address")
    pio_size = Param.Addr(0x1000, "PIO size")
