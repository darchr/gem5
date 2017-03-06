# Copyright (c) 2016 Mark D. Hill and David A. Wood
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Jason Lowe-Power

from m5.objects import *

import SimpleOpts

class DRAMCache(SubSystem):

    def __init__(self, dram_cache_size = '1GB',
                       dram_cache_banks = 16,
                       writeback_policy = 'writethrough',
                       no_dirty_list = False,
                       infinite_dirty_list = False,
                       dirty_list_entries = 2**15,
                       fom = False,
                       perfect_dirty_list = False,
                       region_size = '4kB',
                       full_tags = False,
                       fraction = 0.25):
        super(DRAMCache, self).__init__()

        self._size = MemorySize(dram_cache_size).getValue()

        if self._size == 0:
            m5.panic("Tried to construct empty DRAM cache")

        self._numBanks = dram_cache_banks

        bank_size = self._size / self._numBanks
        if self._size % self._numBanks != 0:
            m5.panic("Indivisible number of banks (power of 2?)")

        print "entries", dirty_list_entries, "banks", self._numBanks,
        print "entries per bank", dirty_list_entries / self._numBanks

        if no_dirty_list:
            get_dirty_list = lambda: NULL
        elif infinite_dirty_list:
            get_dirty_list = lambda: InfiniteDirtyList(cache_size =
                                                         str(bank_size)+'B')
        elif perfect_dirty_list:
            region_size_bytes = MemorySize(region_size).getValue()
            bank_lines = bank_size / 64
            region_lines = region_size_bytes / self._numBanks / 64
            cache_regions = bank_lines / region_lines
            print "Bank size", bank_size, "lines:", bank_lines
            print "Entries:", region_lines
            print "Region size:", region_size, "lines:", region_lines
            print "Regions:", cache_regions
            print "Tags per entry:", region_lines
            print "Total size:", region_lines * 3 * cache_regions / 1024, 'KB'
            get_dirty_list = lambda: FullSADirtyList(
                              cache_size = str(bank_size)+'B',
                              entries = cache_regions,
                              region_size = region_size)
        else:
            region_size_bytes = MemorySize(region_size).getValue()
            bank_lines = bank_size / 64
            region_lines = region_size_bytes / self._numBanks / 64
            cache_regions = bank_lines / region_lines
            print "Bank size", bank_size, "lines:", bank_lines
            print "Entries:", region_lines
            print "Region size:", region_size, "lines:", region_lines
            print "Regions:", cache_regions
            print "Tags per entry:", region_lines
            print "Total size:", region_lines * 3 * cache_regions / 1024, 'KB'
            if full_tags:
                entries = cache_regions
            else:
                entries = int(cache_regions * fraction)
            get_dirty_list = lambda: SplitDirtyList(
                              cache_size = str(bank_size)+'B',
                              entries = entries,
                              region_size = region_size)
        if fom:
            cls = DRAMCacheCtrlFOM
        else:
            cls = DRAMCacheCtrl

        self.banks = [cls(size = str(bank_size)+'B',
                               banks = self._numBanks,
                               dirty_list = get_dirty_list(),
                               writeback_policy = writeback_policy,
                               bank_number = i)
                      for i in range(self._numBanks)]


        for bank in self.banks:
            # NOTE: If you change the number of controllers, you HAVE to
            #       update the interleaved ranges!!!!
            bank.setStorageCtrl(HBM_1000_4H_x64_Storage)

    def setAddrRanges(self, ranges):
        # For some reason we need a special case for 1 bank
        if self._numBanks == 1:
            self.banks[0].addr_ranges = ranges
            return

        for bank in self.banks:
            bank.addr_ranges = []

        for rng in ranges:
            inlv_ranges = self._getInterleaveRanges(rng, self._numBanks, 6)
            for i,bank in enumerate(self.banks):
                bank.addr_ranges.append(inlv_ranges[i])

    def connectCPUSideBus(self, bus):
        for bank in self.banks:
            bank.cpu_side = bus.master

    def connectMemSideBus(self, bus):
        for bank in self.banks:
            bank.mem_side = bus.slave

    def setBackingCtrl(self, ctrl):
        """ Set the dram_ctrl of all the banks.
        """
        for bank in self.banks:
            bank.dram_ctrl = ctrl

    def _getInterleaveRanges(self, rng, num, intlv_low_bit):
        from math import log
        bits = int(log(num, 2))
        if 2**bits != num:
            m5.fatal("Non-power of two number of memory controllers")

        intlv_bits = bits
        ranges = [
            AddrRange(start=rng.start,
                      end=rng.end,
                      intlvHighBit = intlv_low_bit + intlv_bits - 1,
                      xorHighBit = 0,
                      intlvBits = intlv_bits,
                      intlvMatch = i)
                for i in range(num)
            ]

        return ranges
