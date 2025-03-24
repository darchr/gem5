from m5.objects.ClockedObject import ClockedObject
from m5.params import *
from m5.proxy import *


class DataCell(ClockedObject):
    type = "DataCell"
    cxx_header = "network/DataCell.hh"
    cxx_class = "gem5::DataCell"
