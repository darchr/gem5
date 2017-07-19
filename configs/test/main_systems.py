
from m5.objects import *
import system
from dram_cache import DRAMCache
import SimpleOpts

class Baseline(system.MySystem):

    def __init__(self, opts):
        super(Baseline, self).__init__(opts)

    def createMemorySystem(self):
        self.createMemoryControllersDDR3(self.membus)


class InfiniteCache(system.MySystem):

    def __init__(self, opts):
        super(InfiniteCache, self).__init__(opts)

    def createMemorySystem(self):
       self.createMemoryControllersHBM(self.membus)


class BaselineDRAMCache(system.MySystem):
    """ This has DRAM tags, writeback, fill on writeback """

    def __init__(self, opts):
        super(BaselineDRAMCache, self).__init__(opts)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='writeback',
                                   no_dirty_list=True)
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges(self.mem_ranges)

        for bank in self.l3cache.banks:
            bank.writeback_clean = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

class BaselinePlusCWBFilter(system.MySystem):
    """ This has DRAM tags, writeback, fill on writeback """

    def __init__(self, opts):
        super(BaselinePlusCWBFilter, self).__init__(opts)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='writeback',
                                   no_dirty_list=True)
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges(self.mem_ranges)

        for bank in self.l3cache.banks:
            bank.writeback_clean = True

        self.membus.filter_clean_writebacks = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

class WriteThroughDRAMCache(system.MySystem):
    """ This has DRAM tags, writeback, fill on writeback """

    def __init__(self, opts):
        super(WriteThroughDRAMCache, self).__init__(opts)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='writethrough',
                                   no_dirty_list=True)
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges(self.mem_ranges)

        for bank in self.l3cache.banks:
            bank.writeback_clean = True

        self.membus.filter_clean_writebacks = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

class AdaptiveWBDRAMCache(system.MySystem):
    """ This has DRAM tags, writeback, fill on writeback """

    SimpleOpts.add_option("--region_size", default='16kB')

    def __init__(self, opts):
        self._region_size = opts.region_size
        super(AdaptiveWBDRAMCache, self).__init__(opts)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='adaptive',
                                   no_dirty_list=False,
                                   region_size = self._region_size,
                                   dirty_list_entries=0.125)
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges(self.mem_ranges)

        for bank in self.l3cache.banks:
            bank.writeback_clean = True

        self.membus.filter_clean_writebacks = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

class AdaptiveWBFullDRAMCache(system.MySystem):
    """ This has DRAM tags, writeback, fill on writeback """

    def __init__(self, opts):
        self._region_size = opts.region_size
        super(AdaptiveWBFullDRAMCache, self).__init__(opts)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='adaptive',
                                   no_dirty_list=False,
                                   region_size = self._region_size,
                                   dirty_list_entries=1)
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges(self.mem_ranges)

        for bank in self.l3cache.banks:
            bank.writeback_clean = True

        self.membus.filter_clean_writebacks = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

class AdaptiveWBInfDLDRAMCache(system.MySystem):
    """ This has DRAM tags, writeback, fill on writeback """

    def __init__(self, opts):
        super(AdaptiveWBInfDLDRAMCache, self).__init__(opts)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='adaptive',
                                   no_dirty_list=False,
                                   infinite_dirty_list=True)
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges(self.mem_ranges)

        for bank in self.l3cache.banks:
            bank.writeback_clean = True

        self.membus.filter_clean_writebacks = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])


class FOMDRAMCache(system.MySystem):

    def __init__(self, opts):
        super(FOMDRAMCache, self).__init__(opts)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(fom = True,
                                   writeback_policy = 'writeback',
                                   no_dirty_list = True)

        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges(self.mem_ranges)

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

class KNLDRAMCache(system.MySystem):

    def __init__(self, opts):
        super(KNLDRAMCache, self).__init__(opts)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(fom = True,
                                   writeback_policy = 'writeback',
                                   no_dirty_list = True,
                                   send_backprobes = True)

        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges(self.mem_ranges)

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

class BEARDRAMCache(system.MySystem):

    def __init__(self, opts):
        super(BEARDRAMCache, self).__init__(opts)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(fom = True,
                                   writeback_policy = 'writeback',
                                   no_dirty_list = True,
                                   send_backprobes = True)

        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges(self.mem_ranges)

        self.membus.track_dcp = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])


class WBWithInfTagsCache(system.MySystem):
    """ This has SRAM tags, writeback, fill on writeback """

    def __init__(self, opts):
        super(WBWithInfTagsCache, self).__init__(opts)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='writeback',
                                   no_dirty_list=False,
                                   infinite_dirty_list=True)
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges(self.mem_ranges)

        for bank in self.l3cache.banks:
           bank.writeback_clean = True

        self.membus.filter_clean_writebacks = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

def getSystem(config, opts):
    configs = {'baseline': Baseline,
               'knl-like': KNLDRAMCache,
               'bear-like': BEARDRAMCache,
               'dramcache-0': FOMDRAMCache,
               'dramcache-1': BaselineDRAMCache,
               'dramcache-2': BaselinePlusCWBFilter,
               'dramcache-3': WriteThroughDRAMCache,
               'dramcache-4': AdaptiveWBDRAMCache,
               'dramcache-4-full': AdaptiveWBFullDRAMCache,
               'dramcache-4-inf': AdaptiveWBInfDLDRAMCache,
               'dramcache-2-tags': WBWithInfTagsCache,
               'infcache': InfiniteCache,}

    return configs[config](opts)
