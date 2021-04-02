# Copyright (c) 2020 Jason Lowe-Power
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

"""
Internal gem5 C++ components exposed to Python.

This module should only be used by internal gem5 Python libraries.  The APIs
and modules contained in _m5 could change at any time. The name is _m5 to keep
backwards compatibility with all scripts from before the m5->gem5 name change.

This file wraps the actual _m5 that is in the gem5 binary. If we are not
running in the context of a gem5 binary, we raise a ModuleNotFoundError.
"""

# Import the actual _m5 from gem5.
try:
    import _m5
except ModuleNotFoundError as e:
    # If _m5 is not found, this means that we're not running in gem5
    print("Cannot import _m5. Please run this script with the gem5 binary.")
    raise

# Only expose the parts of gem5 that are needed by python scripts.
# Other objects (like the  param_* objects) are only used internally.
core = _m5.core
debug = _m5.debug
drain = _m5.drain
event = _m5.event
net = _m5.net
range = _m5.range
serialize = _m5.serialize
sim = _m5.sim
stats = _m5.stats
systemc = _m5.systemc
trace = _m5.trace

__all__ = [
    core,
    debug,
    drain,
    event,
    net,
    range,
    serialize,
    sim,
    stats,
    systemc,
    trace,
]
