# Copyright (c) 2021 The Regents of the University of California
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

"""Specifies the coherence protocol enum

Used for typing in python. This enum needs to be extended/modified as the
components library supports more coherence protocols.
"""

from enum import Enum


class CoherenceProtocol(Enum):
    MESI_THREE_LEVEL = 1
    MESI_THREE_LEVEL_HTM = 2
    ARM_MOESI_HAMMER = 3
    GARNET_STANDALONE = 4
    MESI_TWO_LEVEL = 5
    MOESI_CMP_DIRECTORY = 6
    MOESI_CMP_TOKEN = 7
    MOESI_AMD_BASE = 8
    MI_EXAMPLE = 9
    GPU_VIPER = 10
    CHI = 11

def is_classic(protocol: CoherenceProtocol) -> bool:
    classic = (
        CoherenceProtocol.CHI,
    )

    return protocol in classic

def is_ruby(protocol: CoherenceProtocol) -> bool:
    # TODO: I have no idea if this is correct. I should check.
    ruby = (
        CoherenceProtocol.MESI_THREE_LEVEL,
        CoherenceProtocol.MESI_THREE_LEVEL_HTM,
        CoherenceProtocol.ARM_MOESI_HAMMER,
        CoherenceProtocol.GARNET_STANDALONE,
        CoherenceProtocol.MESI_TWO_LEVEL,
        CoherenceProtocol.MOESI_CMP_DIRECTORY,
        CoherenceProtocol.MOESI_CMP_TOKEN,
        CoherenceProtocol.MOESI_AMD_BASE,
        CoherenceProtocol.MI_EXAMPLE,
        CoherenceProtocol.GPU_VIPER,
    )

    return protocol in ruby
