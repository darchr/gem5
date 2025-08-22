from m5.objects.PciDevice import PciDevice
from m5.params import *
from m5.SimObject import SimObject


class MyPCIDevice(PciDevice):
    type = "MyPCIDevice"
    cxx_header = "new/my_pci_device.cc"
    cxx_class = "MyPCIDevice"
