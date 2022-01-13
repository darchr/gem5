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

class LLM(DRAMInterface):

    # SerDes working at 32GHz, 32GHz x 2ns = 64
    beats_per_clock = 64

    # 500 MHz for 2GB/s per Die
    tCK = '2ns'

    # Each channel connecting to a single optical IO
    device_bus_width = 1

    # Using close-page  policy to  have deterministic
    # memory access latency
    page_policy = 'close'

    # Sending 64bytes every 16ns (tBURST)
    # BL = 16ns / 2ns (tCK) = 8
    burst_length = 512

    # Size of channel in bytes, 4H stack of 8Gb dies is 4GB per stack.
    # 16 channels (memscheduler) For 64 Bank per channel.
    device_size = '4MB'

    # Row buffer size for  HBM2.0 is 1KB
    # To save activation energy we divide this by half
    # To conpensate for the bank BW we can increase the bitlines
    device_rowbuffer_size = '256B'

    devices_per_rank = 1

    ranks_per_channel = 1

    # Hack: each bank has 2 mini-banks
    banks_per_rank = 2

    # One bank group per bank
    bank_groups_per_rank = 0

    # use values from IDD measurement in JEDEC spec
    # use tRP value for tRCD and tCL similar to other classes
    tRP = '15ns'
    # Decreasing the length of global bitline by a factor of 2.
    tRCD = '8ns'
    # Pre-GSA tCL + post-GSA tCL
    tCL = '5ns'
    tRAS = '15ns'
    # We consider tCCD_L here as tRCD
    # Requests going to the same bank and different mini-banks
    # should be tRCD apart
    # We are doing this because we are modeling mini0banks with
    # multiple banks per ranks
    tCCD_L = '15ns'

    tBURST_MIN = '2ns'
    tBURST_MAX = '4ns'
    #  64Byte (atom size) / 32Gbps (datarate of wavelength) = 16ns
    tBURST = '16ns'

    tWA = '3ns'
    # value for 2Gb device from JEDEC spec
    tRFC = '160ns'

    # value for 2Gb device from JEDEC spec
    tREFI = '3.9us'


    tWR = '18ns'
    # The time between read to precharge
    # ~ tRAS - tRCD
    tRTP = '7ns'
    # No bus turnaround. One waveguide per read and one per write.
    tWTR = '0ns'

    # Same as above: busturnaround penalty (read-to-write, write-to-read)
    tRTW = '0ns'
    # single rank device, set to 0
    tCS = '0ns'

    # from MemCon example, tRRD is 4ns with 2ns tCK
    tRRD = '2ns'
    tRRD_L = '2ns'

    tXAW = '12ns'
    # Reduced the activation row size by a factor of 4.
    # We can have 4 times more acivations.
    activation_limit = 32

    # 4tCK
    tXP = '8ns'

    # start with tRFC + tXP -> 160ns + 8ns = 168ns
    tXS = '168ns'

    addr_mapping = 'RoCoRaBaCh'

    salp_enable = True