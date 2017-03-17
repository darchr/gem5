# Copyright (c) 2012-2014 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2013 Amin Farmahini-Farahani
# Copyright (c) 2015 University of Kaiserslautern
# Copyright (c) 2015 The University of Bologna
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
# Authors: Andreas Hansson
#          Ani Udipi
#          Omar Naji
#          Matthias Jung
#          Erfan Azarkhish
#          Jason Lowe-Power

from m5.params import *
from m5.proxy import *
from MemObject import *
from DRAMCtrl import *

# DRAMCtrl is a single-channel single-ported DRAM controller model
# that aims to model the most important system-level performance
# effects of a DRAM without getting into too much detail of the DRAM
# itself.
class DRAMCacheStorage(MemObject):
    type = 'DRAMCacheStorage'
    cxx_header = "mem/dram_cache/dram_cache_storage.hh"

    # Things to make this similar to abstract memory
    range = Param.AddrRange("Address range (potentially interleaved)")
    system = Param.System(Parent.any, "System parent")

    block_size = Param.Unsigned(Parent.block_size, "Block size for the cache")

    # the basic configuration of the controller architecture, note
    # that each entry corresponds to a burst for the specific DRAM
    # configuration (e.g. x32 with burst length 8 is 32 bytes) and not
    # the cacheline size or request/packet size
    write_buffer_size = Param.Unsigned(64, "Number of write queue entries")
    read_buffer_size = Param.Unsigned(32, "Number of read queue entries")

    # threshold in percent for when to forcefully trigger writes and
    # start emptying the write buffer
    write_high_thresh_perc = Param.Percent(85, "Threshold to force writes")

    # threshold in percentage for when to start writes if the read
    # queue is empty
    write_low_thresh_perc = Param.Percent(50, "Threshold to start writes")

    # minimum write bursts to schedule before switching back to reads
    min_writes_per_switch = Param.Unsigned(16, "Minimum write bursts before "
                                           "switching to reads")

    # scheduler, address map and page policy
    mem_sched_policy = Param.MemSched('frfcfs', "Memory scheduling policy")
    addr_mapping = Param.AddrMap('RoRaBaCoCh', "Address mapping policy")
    page_policy = Param.PageManage('open_adaptive', "Page management policy")

    # enforce a limit on the number of accesses per row
    max_accesses_per_row = Param.Unsigned(16, "Max accesses per row before "
                                          "closing");

    # size of DRAM Chip in Bytes
    device_size = Param.MemorySize("Size of DRAM chip")

    # pipeline latency of the controller and PHY, split into a
    # frontend part and a backend part, with reads and writes serviced
    # by the queues only seeing the frontend contribution, and reads
    # serviced by the memory seeing the sum of the two
    static_frontend_latency = Param.Latency("10ns", "Static frontend latency")
    static_backend_latency = Param.Latency("10ns", "Static backend latency")

    # the physical organisation of the DRAM
    device_bus_width = Param.Unsigned("data bus width in bits for each DRAM "\
                                      "device/chip")
    burst_length = Param.Unsigned("Burst lenght (BL) in beats")
    device_rowbuffer_size = Param.MemorySize("Page (row buffer) size per "\
                                           "device/chip")
    devices_per_rank = Param.Unsigned("Number of devices/chips per rank")
    ranks_per_channel = Param.Unsigned("Number of ranks per channel")

    # default to 0 bank groups per rank, indicating bank group architecture
    # is not used
    # update per memory class when bank group architecture is supported
    bank_groups_per_rank = Param.Unsigned(0, "Number of bank groups per rank")
    banks_per_rank = Param.Unsigned("Number of banks per rank")
    # only used for the address mapping as the controller by
    # construction is a single channel and multiple controllers have
    # to be instantiated for a multi-channel configuration
    channels = Param.Unsigned(1, "Number of channels")

    # For power modelling we need to know if the DRAM has a DLL or not
    dll = Param.Bool(True, "DRAM has DLL or not")

    # DRAMPower provides in addition to the core power, the possibility to
    # include RD/WR termination and IO power. This calculation assumes some
    # default values. The integration of DRAMPower with gem5 does not include
    # IO and RD/WR termination power by default. This might be added as an
    # additional feature in the future.

    # timing behaviour and constraints - all in nanoseconds

    # the base clock period of the DRAM
    tCK = Param.Latency("Clock period")

    # the amount of time in nanoseconds from issuing an activate command
    # to the data being available in the row buffer for a read/write
    tRCD = Param.Latency("RAS to CAS delay")

    # the time from issuing a read/write command to seeing the actual data
    tCL = Param.Latency("CAS latency")

    # minimum time between a precharge and subsequent activate
    tRP = Param.Latency("Row precharge time")

    # minimum time between an activate and a precharge to the same row
    tRAS = Param.Latency("ACT to PRE delay")

    # minimum time between a write data transfer and a precharge
    tWR = Param.Latency("Write recovery time")

    # minimum time between a read and precharge command
    tRTP = Param.Latency("Read to precharge")

    # time to complete a burst transfer, typically the burst length
    # divided by two due to the DDR bus, but by making it a parameter
    # it is easier to also evaluate SDR memories like WideIO.
    # This parameter has to account for burst length.
    # Read/Write requests with data size larger than one full burst are broken
    # down into multiple requests in the controller
    # tBURST is equivalent to the CAS-to-CAS delay (tCCD)
    # With bank group architectures, tBURST represents the CAS-to-CAS
    # delay for bursts to different bank groups (tCCD_S)
    tBURST = Param.Latency("Burst duration (for DDR burst length / 2 cycles)")

    # CAS-to-CAS delay for bursts to the same bank group
    # only utilized with bank group architectures; set to 0 for default case
    # tBURST is equivalent to tCCD_S; no explicit parameter required
    # for CAS-to-CAS delay for bursts to different bank groups
    tCCD_L = Param.Latency("0ns", "Same bank group CAS to CAS delay")

    # time taken to complete one refresh cycle (N rows in all banks)
    tRFC = Param.Latency("Refresh cycle time")

    # refresh command interval, how often a "ref" command needs
    # to be sent. It is 7.8 us for a 64ms refresh requirement
    tREFI = Param.Latency("Refresh command interval")

    # write-to-read, same rank turnaround penalty
    tWTR = Param.Latency("Write to read, same rank switching time")

    # read-to-write, same rank turnaround penalty
    tRTW = Param.Latency("Read to write, same rank switching time")

    # rank-to-rank bus delay penalty
    # this does not correlate to a memory timing parameter and encompasses:
    # 1) RD-to-RD, 2) WR-to-WR, 3) RD-to-WR, and 4) WR-to-RD
    # different rank bus delay
    tCS = Param.Latency("Rank to rank switching time")

    # minimum row activate to row activate delay time
    tRRD = Param.Latency("ACT to ACT delay")

    # only utilized with bank group architectures; set to 0 for default case
    tRRD_L = Param.Latency("0ns", "Same bank group ACT to ACT delay")

    # time window in which a maximum number of activates are allowed
    # to take place, set to 0 to disable
    tXAW = Param.Latency("X activation window")
    activation_limit = Param.Unsigned("Max number of activates in window")

    # time to exit power-down mode
    # Exit power-down to next valid command delay
    tXP = Param.Latency("0ns", "Power-up Delay")

    # Exit Powerdown to commands requiring a locked DLL
    tXPDLL = Param.Latency("0ns", "Power-up Delay with locked DLL")

    # time to exit self-refresh mode
    tXS = Param.Latency("0ns", "Self-refresh exit latency")

    # time to exit self-refresh mode with locked DLL
    tXSDLL = Param.Latency("0ns", "Self-refresh exit latency DLL")

    # Currently rolled into other params
    ######################################################################

    # tRC  - assumed to be tRAS + tRP

    # Power Behaviour and Constraints
    # DRAMs like LPDDR and WideIO have 2 external voltage domains. These are
    # defined as VDD and VDD2. Each current is defined for each voltage domain
    # separately. For example, current IDD0 is active-precharge current for
    # voltage domain VDD and current IDD02 is active-precharge current for
    # voltage domain VDD2.
    # By default all currents are set to 0mA. Users who are only interested in
    # the performance of DRAMs can leave them at 0.

    # Operating 1 Bank Active-Precharge current
    IDD0 = Param.Current("0mA", "Active precharge current")

    # Operating 1 Bank Active-Precharge current multiple voltage Range
    IDD02 = Param.Current("0mA", "Active precharge current VDD2")

    # Precharge Power-down Current: Slow exit
    IDD2P0 = Param.Current("0mA", "Precharge Powerdown slow")

    # Precharge Power-down Current: Slow exit multiple voltage Range
    IDD2P02 = Param.Current("0mA", "Precharge Powerdown slow VDD2")

    # Precharge Power-down Current: Fast exit
    IDD2P1 = Param.Current("0mA", "Precharge Powerdown fast")

    # Precharge Power-down Current: Fast exit multiple voltage Range
    IDD2P12 = Param.Current("0mA", "Precharge Powerdown fast VDD2")

    # Precharge Standby current
    IDD2N = Param.Current("0mA", "Precharge Standby current")

    # Precharge Standby current multiple voltage range
    IDD2N2 = Param.Current("0mA", "Precharge Standby current VDD2")

    # Active Power-down current: slow exit
    IDD3P0 = Param.Current("0mA", "Active Powerdown slow")

    # Active Power-down current: slow exit multiple voltage range
    IDD3P02 = Param.Current("0mA", "Active Powerdown slow VDD2")

    # Active Power-down current : fast exit
    IDD3P1 = Param.Current("0mA", "Active Powerdown fast")

    # Active Power-down current : fast exit multiple voltage range
    IDD3P12 = Param.Current("0mA", "Active Powerdown fast VDD2")

    # Active Standby current
    IDD3N = Param.Current("0mA", "Active Standby current")

    # Active Standby current multiple voltage range
    IDD3N2 = Param.Current("0mA", "Active Standby current VDD2")

    # Burst Read Operating Current
    IDD4R = Param.Current("0mA", "READ current")

    # Burst Read Operating Current multiple voltage range
    IDD4R2 = Param.Current("0mA", "READ current VDD2")

    # Burst Write Operating Current
    IDD4W = Param.Current("0mA", "WRITE current")

    # Burst Write Operating Current multiple voltage range
    IDD4W2 = Param.Current("0mA", "WRITE current VDD2")

    # Refresh Current
    IDD5 = Param.Current("0mA", "Refresh current")

    # Refresh Current multiple voltage range
    IDD52 = Param.Current("0mA", "Refresh current VDD2")

    # Self-Refresh Current
    IDD6 = Param.Current("0mA", "Self-refresh Current")

    # Self-Refresh Current multiple voltage range
    IDD62 = Param.Current("0mA", "Self-refresh Current VDD2")

    # Main voltage range of the DRAM
    VDD = Param.Voltage("0V", "Main Voltage Range")

    # Second voltage range defined by some DRAMs
    VDD2 = Param.Voltage("0V", "2nd Voltage Range")

# A single DDR3-1600 x64 channel (one command and address bus), with
# timings based on a DDR3-1600 4 Gbit datasheet (Micron MT41J512M8) in
# an 8x8 configuration.
class DDR3_1600_x64_Storage(DRAMCacheStorage):
    # size of device in bytes
    device_size = '512MB'

    # 8x8 configuration, 8 devices each with an 8-bit interface
    device_bus_width = 8

    # DDR3 is a BL8 device
    burst_length = 8

    # Each device has a page (row buffer) size of 1 Kbyte (1K columns x8)
    device_rowbuffer_size = '1kB'

    # 8x8 configuration, so 8 devices
    devices_per_rank = 8

    # Use two ranks
    ranks_per_channel = 2

    # DDR3 has 8 banks in all configurations
    banks_per_rank = 8

    # 800 MHz
    tCK = '1.25ns'

    # 8 beats across an x64 interface translates to 4 clocks @ 800 MHz
    # Added 1 for the tag.
    tBURST = '6.25ns'

    # DDR3-1600 11-11-11
    tRCD = '13.75ns'
    tCL = '13.75ns'
    tRP = '13.75ns'
    tRAS = '35ns'
    tRRD = '6ns'
    tXAW = '30ns'
    activation_limit = 4
    tRFC = '260ns'

    tWR = '15ns'

    # Greater of 4 CK or 7.5 ns
    tWTR = '7.5ns'

    # Greater of 4 CK or 7.5 ns
    tRTP = '7.5ns'

    # Default same rank rd-to-wr bus turnaround to 2 CK, @800 MHz = 2.5 ns
    tRTW = '2.5ns'

    # Default different rank bus delay to 2 CK, @800 MHz = 2.5 ns
    tCS = '2.5ns'

    # <=85C, half for >85C
    tREFI = '7.8us'

    # Current values from datasheet
    IDD0 = '75mA'
    IDD2N = '50mA'
    IDD3N = '57mA'
    IDD4W = '165mA'
    IDD4R = '187mA'
    IDD5 = '220mA'
    VDD = '1.5V'

# A single HBM x128 interface (one command and address bus), with
# default timings based on data publically released
# ("HBM: Memory Solution for High Performance Processors", MemCon, 2014),
# IDD measurement values, and by extrapolating data from other classes.
# Architecture values based on published HBM spec
# A 4H stack is defined, 2Gb per die for a total of 1GB of memory.
class HBM_1000_4H_x128_Storage(DRAMCacheStorage):
    # HBM gen1 supports up to 8 128-bit physical channels
    # Configuration defines a single channel, with the capacity
    # set to (full_ stack_capacity / 8) based on 2Gb dies
    # To use all 8 channels, set 'channels' parameter to 8 in
    # system configuration

    # 128-bit interface legacy mode
    device_bus_width = 128

    # HBM supports BL4 and BL2 (legacy mode only)
    burst_length = 4

    # size of channel in bytes, 4H stack of 2Gb dies is 1GB per stack;
    # with 8 channels, 128MB per channel
    device_size = '128MB'

    device_rowbuffer_size = '2kB'

    # 1x128 configuration
    devices_per_rank = 1

    # HBM does not have a CS pin; set rank to 1
    ranks_per_channel = 1

    # HBM has 8 or 16 banks depending on capacity
    # 2Gb dies have 8 banks
    banks_per_rank = 8

    # depending on frequency, bank groups may be required
    # will always have 4 bank groups when enabled
    # current specifications do not define the minimum frequency for
    # bank group architecture
    # setting bank_groups_per_rank to 0 to disable until range is defined
    bank_groups_per_rank = 0

    # 500 MHz for 1Gbps DDR data rate
    tCK = '2ns'

    # use values from IDD measurement in JEDEC spec
    # use tRP value for tRCD and tCL similar to other classes
    tRP = '15ns'
    tRCD = '15ns'
    tCL = '15ns'
    tRAS = '33ns'

    # BL2 and BL4 supported, default to BL4
    # DDR @ 500 MHz means 4 * 2ns / 2 = 4ns
    # Modified to account for tags. Extra 4B per 32B burst or 8B for 64B.
    tBURST = '5ns'

    # value for 2Gb device from JEDEC spec
    tRFC = '160ns'

    # value for 2Gb device from JEDEC spec
    tREFI = '3.9us'

    # extrapolate the following from LPDDR configs, using ns values
    # to minimize burst length, prefetch differences
    tWR = '18ns'
    tRTP = '7.5ns'
    tWTR = '10ns'

    # start with 2 cycles turnaround, similar to other memory classes
    # could be more with variations across the stack
    tRTW = '4ns'

    # single rank device, set to 0
    tCS = '0ns'

    # from MemCon example, tRRD is 4ns with 2ns tCK
    tRRD = '4ns'

    # from MemCon example, tFAW is 30ns with 2ns tCK
    tXAW = '30ns'
    activation_limit = 4

    # 4tCK
    tXP = '8ns'

    # start with tRFC + tXP -> 160ns + 8ns = 168ns
    tXS = '168ns'

# A single HBM x64 interface (one command and address bus), with
# default timings based on HBM gen1 and data publically released
# A 4H stack is defined, 8Gb per die for a total of 4GB of memory.
# Note: This defines a pseudo-channel with a unique controller
# instantiated per pseudo-channel
# Stay at same IO rate (1Gbps) to maintain timing relationship with
# HBM gen1 class (HBM_1000_4H_x128) where possible
class HBM_1000_4H_x64_Storage(HBM_1000_4H_x128_Storage):
    # For HBM gen2 with pseudo-channel mode, configure 2X channels.
    # Configuration defines a single pseudo channel, with the capacity
    # set to (full_ stack_capacity / 16) based on 8Gb dies
    # To use all 16 pseudo channels, set 'channels' parameter to 16 in
    # system configuration

    # 64-bit pseudo-channle interface
    device_bus_width = 64

    # HBM pseudo-channel only supports BL4
    burst_length = 4

    # size of channel in bytes, 4H stack of 8Gb dies is 4GB per stack;
    # with 16 channels, 256MB per channel
    device_size = '256MB'

    # page size is halved with pseudo-channel; maintaining the same same number
    # of rows per pseudo-channel with 2X banks across 2 channels
    device_rowbuffer_size = '1kB'

    # HBM has 8 or 16 banks depending on capacity
    # Starting with 4Gb dies, 16 banks are defined
    banks_per_rank = 16

    # reset tRFC for larger, 8Gb device
    # use HBM1 4Gb value as a starting point
    tRFC = '260ns'

    # start with tRFC + tXP -> 160ns + 8ns = 168ns
    tXS = '268ns'
    # Default different rank bus delay to 2 CK, @1000 MHz = 2 ns
    tCS = '2ns'
    tREFI = '3.9us'

    # Numbers from http://dl.acm.org/citation.cfm?id=2989098
    IDD0 = '85mA'
    IDD2N = '8mA'
    IDD3N = '12mA'
    IDD3N2 = '3mA'
    IDD4W = '73mA'
    IDD4R = '66mA'
    IDD5 = '50mA'
    IDD02 = '3mA'
    IDD3P1 = '5mA'
    IDD2P1 = '3mA'
    IDD6 = '1mA'
    VDD = '1.2V'
    VDD2 = '2.5V'

    # Different number from
    # (http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=7004159)
    # IDD0 = '45mA'
    # IDD2N = '5mA'
    # IDD3N = '6mA'
    # IDD3N2 = '3mA'
    # IDD4W = '103mA'
    # IDD4R = '110mA'
    # IDD5 = '162mA'
    # IDD02 = '3mA'
    # IDD3P1 = '4mA'
    # IDD2P1 = '2mA'
    # IDD6 = '3mA'
    # VDD = '1.2V'
    # VDD2 = '2.5V'
