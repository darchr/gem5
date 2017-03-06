
from m5.objects import *
import system
from dram_cache import DRAMCache

class Baseline(system.MySystem):

    def __init__(self, secondDisk, reqMem):
        super(Baseline, self).__init__(secondDisk, reqMem)

    def createMemorySystem(self):
        self.createMemoryControllersDDR3(self.membus)


class InfiniteCache(system.MySystem):

    def __init__(self, secondDisk, reqMem):
        super(InfiniteCache, self).__init__(secondDisk, reqMem)

    def createMemorySystem(self):
       self.createMemoryControllersHBM(self.membus)

class FOMDRAMCache(system.MySystem):
    """ This has perfect SRAM tags, adaptive WB, fill on writeback """

    def __init__(self, secondDisk, reqMem):
        super(FOMDRAMCache, self).__init__(secondDisk, reqMem)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(fom = True,
                                   writeback_policy = 'writeback',
                                   no_dirty_list = True)

        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges([self.mem_ranges[0],
                                      self.mem_ranges[-1]])

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

class BaselineDRAMCache(system.MySystem):
    """ This has DRAM tags, writeback, fill on writeback """

    def __init__(self, secondDisk, reqMem):
        super(BaselineDRAMCache, self).__init__(secondDisk, reqMem)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='writeback',
                                   no_dirty_list=True)
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges([self.mem_ranges[0],
                                      self.mem_ranges[-1]])

        for bank in self.l3cache.banks:
            bank.writeback_clean = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

class BaselinePlusCWBFilter(system.MySystem):
    """ This has DRAM tags, writeback, fill on writeback """

    def __init__(self, secondDisk, reqMem):
        super(BaselinePlusCWBFilter, self).__init__(secondDisk, reqMem)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='writeback',
                                   no_dirty_list=True)
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges([self.mem_ranges[0],
                                      self.mem_ranges[-1]])

        for bank in self.l3cache.banks:
            bank.writeback_clean = True

        self.membus.filter_clean_writebacks = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

class WriteThroughDRAMCache(system.MySystem):
    """ This has DRAM tags, writeback, fill on writeback """

    def __init__(self, secondDisk, reqMem):
        super(WriteThroughDRAMCache, self).__init__(secondDisk, reqMem)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='writethrough',
                                   no_dirty_list=True)
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges([self.mem_ranges[0],
                                      self.mem_ranges[-1]])

        for bank in self.l3cache.banks:
            bank.writeback_clean = True

        self.membus.filter_clean_writebacks = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

class AdaptiveWBDRAMCache(system.MySystem):
    """ This has DRAM tags, writeback, fill on writeback """

    def __init__(self, secondDisk, reqMem):
        super(AdaptiveWBDRAMCache, self).__init__(secondDisk, reqMem)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='adaptive',
                                   no_dirty_list=False,
                                   region_size = '16kB')
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges([self.mem_ranges[0],
                                      self.mem_ranges[-1]])

        for bank in self.l3cache.banks:
            bank.writeback_clean = True

        self.membus.filter_clean_writebacks = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

class AdaptiveWBDRAMCacheSmall(system.MySystem):
    """ This has DRAM tags, writeback, fill on writeback """

    def __init__(self, secondDisk, reqMem):
        super(AdaptiveWBDRAMCacheSmall, self).__init__(secondDisk, reqMem)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='adaptive',
                                   no_dirty_list=False,
                                   region_size = '16kB',
                                   fraction=0.125)
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges([self.mem_ranges[0],
                                      self.mem_ranges[-1]])

        for bank in self.l3cache.banks:
            bank.writeback_clean = True

        self.membus.filter_clean_writebacks = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])


class FullAdaptiveWBDRAMCache(system.MySystem):
    """ This has DRAM tags, writeback, fill on writeback """

    def __init__(self, secondDisk, reqMem):
        super(FullAdaptiveWBDRAMCache, self).__init__(secondDisk, reqMem)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='adaptive',
                                   no_dirty_list=False,
                                   region_size = '16kB',
                                   full_tags = True)
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges([self.mem_ranges[0],
                                      self.mem_ranges[-1]])

        for bank in self.l3cache.banks:
            bank.writeback_clean = True

        self.membus.filter_clean_writebacks = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

class AdaptiveWBInfDLDRAMCache(system.MySystem):
    """ This has DRAM tags, writeback, fill on writeback """

    def __init__(self, secondDisk, reqMem):
        super(AdaptiveWBInfDLDRAMCache, self).__init__(secondDisk, reqMem)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='adaptive',
                                   no_dirty_list=False,
                                   infinite_dirty_list=True)
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges([self.mem_ranges[0],
                                      self.mem_ranges[-1]])

        for bank in self.l3cache.banks:
            bank.writeback_clean = True

        self.membus.filter_clean_writebacks = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])

class WBWithInfTagsCache(system.MySystem):
    """ This has SRAM tags, writeback, fill on writeback """

    def __init__(self, secondDisk, reqMem):
        super(WBWithInfTagsCache, self).__init__(secondDisk, reqMem)

    def createMemorySystem(self):

        self.l4bus = L2XBar(width = 64,
                        snoop_filter = SnoopFilter(max_capacity='32MB'))
        self.dramcache = DRAMCache(writeback_policy='writeback',
                                   no_dirty_list=False,
                                   infinite_dirty_list=True)
        self.dramcache.connectCPUSideBus(self.membus)
        self.dramcache.connectMemSideBus(self.l4bus)
        # Just the ranges that memory responds to
        self.dramcache.setAddrRanges([self.mem_ranges[0],
                                      self.mem_ranges[-1]])

        for bank in self.l3cache.banks:
            bank.writeback_clean = True

        self.membus.filter_clean_writebacks = True

        self.createMemoryControllersDDR3(self.l4bus)

        self.dramcache.setBackingCtrl(self.mem_cntrls[0])


def getSystem(config, secondDisk, reqMem):
    configs = {'baseline': Baseline,
               'dramcache-0': FOMDRAMCache,
               'dramcache-1': BaselineDRAMCache,
               'dramcache-2': BaselinePlusCWBFilter,
               'dramcache-3': WriteThroughDRAMCache,
               'dramcache-4': AdaptiveWBDRAMCache,
               'dramcache-4-small': AdaptiveWBDRAMCacheSmall,
               'dramcache-4-full': FullAdaptiveWBDRAMCache,
               'dramcache-4-inf': AdaptiveWBInfDLDRAMCache,
               'dramcache-2-tags': WBWithInfTagsCache,
               'infcache': InfiniteCache,}

    return configs[config](secondDisk, reqMem)
