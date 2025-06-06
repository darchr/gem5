from m5.params import *
from m5.SimObject import SimObject

# from m5.objects.MemObject import MemObject


class LatentMem(SimObject):
    type = "LatentMem"
    cxx_header = "mem/latent_mem.hh"

    latency = Param.Latency("Latency to add to each request")

    cpu_side = SlavePort("Port facing the CPU/L3 cache")
    mem_side = MasterPort("Port facing the memory")
