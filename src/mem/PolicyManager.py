from m5.params import *
from m5.proxy import *
from m5.SimObject import SimObject
from m5.objects.ReplacementPolicies import *
from m5.objects.AbstractMemory import AbstractMemory
from m5.objects.DRAMInterface import *

class Policy(Enum): vals = ['CascadeLakeNoPartWrs', 'Oracle', 'BearWriteOpt', 'TDRAM_NP', 'TDRAM']
class ReplPolicySetAssoc(Enum): vals = ['bip_rp', 'brrip_rp', 'dueling_rp', 'fifo_rp', 'lfu_rp', 'lru_rp', 'mru_rp', 'random_rp', 'second_chance_rp', 'ship_rp', 'tree_plru_rp', 'weighted_lru_rp']

class PolicyManager(AbstractMemory):
    type = 'PolicyManager'
    cxx_header = "mem/policy_manager.hh"
    cxx_class = 'gem5::memory::PolicyManager'

    port = ResponsePort("This port responds to memory requests")
    loc_req_port = RequestPort("This port responds to requests for DRAM cache controller")
    far_req_port = RequestPort("This port responds to requests for backing store controller")

    loc_burst_size = Param.Unsigned(64, "Local memory burst size")
    far_burst_size = Param.Unsigned(64, "Far memory burst size")

    loc_mem_policy = Param.Policy('CascadeLakeNoPartWrs', "DRAM Cache Policy")
    loc_mem = Param.AbstractMemory("local memory device")

    assoc = Param.Unsigned(1, "Number of ways per each set in DRAM cache, if it is set-associative")
    replPol = Param.ReplPolicySetAssoc('lru_rp', "Replacement policy, if it is set-associative")
    replacement_policy = Param.BaseReplacementPolicy(LRURP(), "Replacement policy")
    
    dram_cache_size = Param.MemorySize('128MiB', "DRAM cache block size in bytes")
    block_size = Param.Unsigned(64, "DRAM cache block size in bytes")
    addr_size = Param.Unsigned(64,"Addr size of the request from outside world")
    orb_max_size = Param.Unsigned(256, "Outstanding Requests Buffer size")
    crb_max_size = Param.Unsigned(32, "Conflicting Requests Buffer size")
    extreme = Param.Bool(False, "Control flag for enforcing hit/miss & dirty/clean")
    always_hit = Param.Bool(True, "Control flag for enforcing hit/miss")
    always_dirty = Param.Bool(False, "Control flag for enforcing clean/dirty")

    static_frontend_latency = Param.Latency("10ns", "Static frontend latency")
    static_backend_latency = Param.Latency("10ns", "Static backend latency")

    cache_warmup_ratio = Param.Float(0.95, "DRAM cache warmup ratio")

    bypass_dcache = Param.Bool(False, "if the DRAM cache needs to be bypassed")

    channel_index = Param.String("0","number of DRAM cache channels in the system")

    
