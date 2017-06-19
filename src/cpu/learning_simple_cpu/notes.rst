Notes.


Obvious first steps
-------------------

Create SConscript, LearningSimpleCPU.py SimObject declaration file and cpu.cc/hh.

In cpu.hh override the functions that are pure virtual:
#. `MasterPort &getDataPort()`
#. `MasterPort &getInstPort()`
#. `void wakeup(ThreadID tid)`
#. `Counter totalInsts()`
#. `Counter totalOps()`

Create a MasterPort class
-------------------------

Let's start with something as simple as possible.
For this, I copied the code from the SimpleMemObject.
For now, let's only have a single port for both instructions and data.
We can make it split later.

Now... I run into a problem
----------------------------

At this point, I would expect this to compile. I'm not sure why it isn't.
I'm getting the following error.

::

    [    LINK]  -> X86/gem5.opt
    build/X86/sim/probe/lib.o.partial: In function `class_<>':
    /local.chinook/gem5/gem5/ext/pybind11/include/pybind11/pybind11.h:914: undefined reference to `typeinfo for LearningSimpleCPU'
    build/X86/sim/probe/lib.o.partial: In function `pybind11::detail::descr pybind11::detail::_<LearningSimpleCPU>()':
    /local.chinook/gem5/gem5/ext/pybind11/include/pybind11/descr.h:165: undefined reference to `typeinfo for LearningSimpleCPU'
    build/X86/sim/probe/lib.o.partial: In function `pybind11::detail::type_caster_base<LearningSimpleCPU>::cast(LearningSimpleCPU const*, pybind11::return_value_policy, pybind11::handle)':
    /local.chinook/gem5/gem5/ext/pybind11/include/pybind11/cast.h:408: undefined reference to `typeinfo for LearningSimpleCPU'
    /local.chinook/gem5/gem5/ext/pybind11/include/pybind11/cast.h:408: undefined reference to `typeinfo for LearningSimpleCPU'
    build/X86/cpu/learning_simple_cpu/lib.o.partial: In function `LearningSimpleCPU::LearningSimpleCPU(LearningSimpleCPUParams*)':
    /local.chinook/gem5/gem5/build/X86/cpu/learning_simple_cpu/cpu.cc:37: undefined reference to `vtable for LearningSimpleCPU'
    /local.chinook/gem5/gem5/build/X86/cpu/learning_simple_cpu/cpu.cc:37: undefined reference to `vtable for LearningSimpleCPU'
    /local.chinook/gem5/gem5/build/X86/cpu/learning_simple_cpu/cpu.cc:37: undefined reference to `vtable for LearningSimpleCPU'
    collect2: error: ld returned 1 exit status
    scons: *** [build/X86/gem5.opt] Error 1
    scons: building terminated because of errors.


Ah. The error was that I forgot to implement one function in the LearningSimpleCPU. Oops.


Running with almost no code..
=============================

So, when I try to run it like this, I get the error:

::
    fatal: Process system.cpu.workload is not associated with any HW contexts!

So, I think I need to do something about that!


Execution context?
------------------

This seems to be the next thing we need. But it isn't really documented anywhere.

To set up the process, we have to call the following function:

.. code-block:: c++

    // After getting registered with system object, tell process which
    // system-wide context id it is assigned.
    void
    assignThreadContext(ContextID context_id)
    {
        contextIds.push_back(context_id);
    }


To do this, we call the following function defined in BaseCPU:

.. code-block:: c++

    void registerThreadContexts();


This function assumes you have put something in `std::vector<ThreadContext *> threadContexts;`.
So, I guess we need to do that before we call the function.
Let's also see where other models call this function.
They don't. This is called in `BaseCPU::init()`.

So, before init(), we need to put a thread context in the `threadContexts` vector.
This is for *`ThreadContexts`*, not `Threads`, whatever they are...
I'm going to try to get away with just using SimpleThread.

I'm going to add a vector of SimpleThreads to my CPU in case I need them.
I don't know if I will or not.
Actually, not a vector. We're only supporting one thread anyway.

I need to pass it some stuff. I think it should look something like the following:

.. code-block:: c++
    thread = new SimpleThread(this, 0, params->system, params->workload[0],
                              params->itb, params->dtb, params->isa[0]);

Note: So far, this will only support SE mode.
FS mode has a different signature for the thread context (for some reason), so you have to instantiate the thread context dynamically if you want to support both modes.

I think we also need to add some stuff to init and startup.

.. code-block:: c++

    void
    LearningSimpleCPU::init()
    {
        DPRINTF(LearningSimpleCPU, "LearningSimpleCPU init\n");

        BaseCPU::init();

        thread->getTC()->initMemProxies(thread->getTC());
    }

    void
    LearningSimpleCPU::startup()
    {
        DPRINTF(LearningSimpleCPU, "LearningSimpleCPU startup\n");

        BaseCPU::startup();

        thread->startup();
    }

I would like to go back and make the "thread" be not a pointer.
I'll do that tomorrow.
