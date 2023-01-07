from m5.params import *
from m5.proxy import *
from m5.SimObject import SimObject
from m5.objects.AbstractMemory import AbstractMemory

class Policy(Enum): vals = ['CascadeLakeNoPartWrs', 'RambusHypo', 'BearWriteOpt']

class PolicyManager(AbstractMemory):
    type = 'PolicyManager'
    cxx_header = "mem/policy_manager.hh"
    cxx_class = 'gem5::memory::PolicyManager'


    #system = Param.System(Parent.any, "System that the controller belongs to.")

    port = ResponsePort("This port responds to memory requests")
    loc_req_port = RequestPort("This port responds to requests for DRAM cache controller")
    far_req_port = RequestPort("This port responds to requests for backing store controller")

    loc_burst_size = Param.Unsigned(64, "Local memory burst size")
    far_burst_size = Param.Unsigned(64, "Far memory burst size")

    # loc_mem_ctrl = Param.MemCtrl("Local memory controller")
    # far_mem_ctrl = Param.MemCtrl("Far memory controller")

    loc_mem_policy = Param.Policy('CascadeLakeNoPartWrs', "DRAM Cache Policy")

    dram_cache_size = Param.MemorySize('128MiB', "DRAM cache block size in bytes")
    block_size = Param.Unsigned(64, "DRAM cache block size in bytes")
    addr_size = Param.Unsigned(64,"Addr size of the request from outside world")
    orb_max_size = Param.Unsigned(256, "Outstanding Requests Buffer size")
    crb_max_size = Param.Unsigned(32, "Conflicting Requests Buffer size")
    always_hit = Param.Bool(True, "Control flag for enforcing hit/miss")
    always_dirty = Param.Bool(False, "Control flag for enforcing clean/dirty")
    static_frontend_latency = Param.Latency("10ns", "Static frontend latency")
    static_backend_latency = Param.Latency("10ns", "Static backend latency")

    tRP = Param.Latency("Row precharge time")
    tRCD_RD = Param.Latency("RAS to Read CAS delay")
    tRL = Param.Latency("Read CAS latency")

    cache_warmup_ratio = Param.Float(0.7, "DRAM cache warmup ratio, after that it'll reset the stats")
    
