from m5.params import *

from m5.objects.System import System

class DinoCPUSystem(System):
    type = 'DinoCPUSystem'
    abstract = False
    cxx_header = 'verilator/dinocpu/system.hh'
