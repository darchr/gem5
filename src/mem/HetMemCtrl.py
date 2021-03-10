#Copyright (c) 2021 The Regents of the University of California.
#All Rights Reserved
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

'''
Intel's Integrated memory controller

References:

[1]Characterizing and Modeling Non-Volatile Memory Systems
'''

from m5.params import *
from m5.proxy import *
from m5.objects.QoSMemCtrl import *


class HetMemCtrl(QoSMemCtrl):
    type = 'IntMemCtrl'
    cxx_header = "mem/het_mem_ctrl.hh"

    # single-ported on the system interface side, instantiate with a
    # bus in front of the controller for multiple ports
    port = ResponsePort("This port responds to memory requests")

    # Interface to volatile, DRAM media
    dram = Param.DRAMInterface(NULL, "DRAM interface")

    # Interface to non-volatile media
    nvm = Param.NVMInterface(NULL, "NVM interface")

    # WPQ = 512B (or 8 entries if cache line size is 64B) in
    # Cascade Lake based on [1]
    wpq_enrties = Param.Int(8,"write pending queue entries")

    rpq_enrties = Param.Int(8,"read pending queue entries")