
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
