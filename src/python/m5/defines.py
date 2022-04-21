# Copyright (c) 2022 The Regents of the University of California
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

from ._defines import buildEnv

from . import options
from .util import warn

from m5.objects.RubyProtocols import RubyProtocols

def getRubyProtocol() -> RubyProtocols:
    """Returns the protocol that this simulation is using.

    If only only protocol is built, this will default to that one protocol.
    Otherwise, it uses the protocol given by the gem5 command line parameter
    --main-ruby-protocol.
    """

    # Try and get the command line option. This will not be available at build
    # time, but it's safe to ignore it then.
    if not hasattr(options, 'main_ruby_protocol'):
        return RubyProtocols(buildEnv["PROTOCOL"][0])

    if options.main_ruby_protocol is None:
        # For backwards compatibility with configs/ruby/Ruby.py
        if len(buildEnv["PROTOCOL"]) != 1:
            warn(f"Defaulting to the first build protocol: "
                 f"{buildEnv['PROTOCOL'][0]}. Use `--main-ruby-protocol to "
                 f"specify the protocol to use.")
        protocol = buildEnv["PROTOCOL"][0]
    else:
        protocol = options.main_ruby_protocol
    return RubyProtocols(protocol)

__all__ = ["buildEnv", "getRubyProtocol"]
