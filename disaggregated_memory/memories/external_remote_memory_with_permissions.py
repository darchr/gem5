# Copyright (c) 2023-24 The Regents of the University of California
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


class ExternalSecureRemoteMemory(ExternalRemoteMemory):
    """ExternalSecureRemoteMemory is an AbstractMemorySystem in gem5 that adds
    permission checks for remote memory addresses. While the rest of the system
    is unchanged, this extra piece of heardware is added before the downstream
    port is reached.

    allows SST
    to be interfaced as a component in the gem5's stdlib.

    This updated board is only compatible with the updated
    ArmComposableMemoryBoard. This should be a simple plug and play memory
    system.

    This memory can be initialized either using a size of a memory range.
    However *one of the above* has to be used to initialize this memory.

    @params
        :size: size of this memory.
        :addr_range: address range of this memory
        :use_sst_sim: set this variable to indicate that SST is used to
                    simulate the external memory. functional accesses will
                    still be mirrored. By default, it is set to True.
        :permission model: Based on the space-control paper, the user is able
                    to choose between no-permissions, flat-tables, deact,
                    mondrian and space-control

    * Notes *
        To set a latency to access the remote memory for SST, the user has to
        use the top-level runscript on SST-side to define the access latency
        value. Noncoherent XBars are deprecated from this version of
        ExternalRemoteMemory.
    """

    def __init__(
            self,
            size: "str" = None,
            addr_range: AddrRange = None,
            host_id: int = None,
            use_sst_sim: bool = True,
            permission_model: "str" = None
    ):
        """This class needs to be initialized similar to external memory"""
        super().__init__(size, addr_range, host_id, use_sst_sim)

