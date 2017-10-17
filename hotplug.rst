:Authors: Jason Lowe-Power

Hot swap support
================

This document describes the newly expanded hotplug support for SimObjects.
Previously, CPUs could "takeOverFrom" other CPUs.
This hotplugging support expands this to cover all SimObjects.
Specifically, this enables any SimObject to implement a way to unplug itself from the simulation and a way to plug in the same kind of object in its place.

Now, during simulation, you can "hot swap" one component for another.
This is useful for two common cases.
#. Warming up caches and other structures.
You may want to use a fast but low-fedility model to warm up architecturual structures.
The CPUs supported takeOverFrom, which accomplished this goal, but not the next goal.
#. Modeling multiple different system configurations after fast-forwarding using a non-deterministic fast-forward method (e.g., KVM).
With hot swap support, you can fast-forward to the region of interest (or sample location) and simulate system A, system B, etc. *starting from the same base system state*.

Hot swap API
------------

First, when creating the system in your Python runscript *before calling instantiate()*, you must declare that an object will be hot swapped.
To do this, call the ``willHotSwap`` function.

.. function:: willHotSwap(self, other)

    This function says "other will be hot swapped for this object (self)."
    This function sets other to be "unplugged" and if this object has any port references this function sets up the ports of other to be connected to the same ports as this object is connected to.
    Any SimObject must override this function in the SimObject description file to support hot swapping and they must call SimObject._setupHotSwap() where the work actually occurs.
    However, if you override this function you *must* call the super class function in the overridden function.

Note: this function is not implemented in ``SimObject.py``, instead the function ``_setupHotSwap`` is in ``SimObject.py``.
The implementation of ``willHotSwap`` in ``SimObject.py`` panics.
This forces users to ensure that any object they are hot swapping has been modified to support hot swapping.

There are then functions to support swapping components in and out.
These functions are meant to be called after ``simulate()``.
For instance, you may call ``simulate()``, then exit simulation at the beginning of the ROI.
Then, you would use these functions to hot swap components before calling ``simulate()`` again.

.. function:: unplugged()

    Returns true if this object is currently unplugged.

.. function:: unplug()

    Unplug this component.
    This will disconnect all of the ports *in C++*.
    The Python port bindings are unchanged.
    You can override this function in the SimObject desciption file.

.. function:: plugIn()

    Plug this component into the system to use it as the timing model.
    This connects all of the port references *in C++* that have been established in Python.
    This function assumes that this component was registered with :func:`willHotSwap` previously.


Implementation
--------------

There are two new functions for the ``PortRef`` object.
``PortRef`` is the Python version of the ``Port`` C++ object.
The ``PortRef`` contains the code to connect up the ports in C++ after using the ``=`` in Python to signify the port connections.

.. function:: ccDisconnect()

    This function is similar to ``ccConnect`` except it disconnects instead of connecting the ports.
    Imporantly, like ``ccConnect``, the master port is the only port that operates on the connection.
    However, unlike ``ccConnect``, this function must handle the case when the ``PortRef`` is a slave port.
    This is required since only one SimObject in the master/slave pair will call ``ccDisconnect``.
    If the ``PortRef`` is a slave, this function grabs its peer to disconnect.

.. function:: hotplugConnect()

    This function behaves like ``ccConnect``, but is used when hot plugging a component.
    We cannot simple use ``ccConnect`` because it only operates with master ports and the object we are swapping in may have both master and slave ports.
    Therefore, in this function, we grab both the master and slave port in the ``PortRef`` relationship before calling the C++ function ``connectPorts``

There is also a new C++ function: ``disconnectPorts`` that behaves exactly the opposite of ``connectPorts``.
However, ``disconnectPorts`` only takes a single parameter, the master port to disconnect since the master port takes care of disconnecting both the master and the slave.
