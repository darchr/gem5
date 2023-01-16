# -*- coding: utf-8 -*-
# Copyright (c) 2017 Jason Lowe-Power
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

from m5.params import *
from m5.proxy import *
from m5.objects.ClockedObject import ClockedObject

class RouterEngine(ClockedObject):
    type = "RouterEngine"
    cxx_header = "accl/graph/sega/router_engine.hh"
    cxx_class = "gem5::RouterEngine"

    # push_req_queue_size = Param.Int("Size of the queue to "
    #                                 "queue push requests.")
    # # resp_queue_size should probably be
    # # significantly bigger than push_req_queue_size
    # resp_queue_size = Param.Int("Size of the response queue in the "
    #                                 "push engine where it stores the "
    #                                 "edges read from memory.")

    # max_propagates_per_cycle = Param.Int("Maximum number of propagates "
    #                                                     "done per cycle.")

    # update_queue_size = Param.Int("Maximum number of entries "
    #                                 "for each update queue.")

    gpt_req_side = VectorRequestPort("Outgoing ports to local GPTs")
    gpt_resp_side = VectorRequestPort("incoming ports from local GPTs")
    
    gpn_req_side = VectorRequestPort("Outgoing ports to remote GPNs")
    gpn_resp_side = VectorRequestPort("incoming ports from local GPNs")
    # remote_resp_side = VectorRsponsePort("Incoming ports from GPNs to router")
