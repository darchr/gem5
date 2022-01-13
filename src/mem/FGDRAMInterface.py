# Copyright (c) 2020 The Regents of the University of California.
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

from m5.objects.DRAMInterface import PageManage, DRAMInterface

class FGDRAM(DRAMInterface):

    device_bus_width = 64

    # HBM supports BL4 and BL2 (legacy mode only)
    burst_length = 4

    # size of channel in bytes, 4H stack of 2Gb dies is 1GB per stack;
    # with 8 channels, 128MB per channel
    device_size = '8MB'

    device_rowbuffer_size = '256B'

    # 1x128 configuration
    devices_per_rank = 1

    # HBM does not have a CS pin; set rank to 1
    ranks_per_channel = 1

    # HBM has 8 or 16 banks depending on capacity
    # 2Gb dies have 8 banks
    banks_per_rank = 2

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
    tBURST = '16ns'

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
    tXAW = '12ns'
    activation_limit = 32

    # 4tCK
    tXP = '8ns'

    # start with tRFC + tXP -> 160ns + 8ns = 168ns
    tXS = '168ns'
