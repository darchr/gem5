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
    thread(this, 0, params->system, params->workload[0], params->itb, params->dtb, params->isa[0]);

Note: So far, this will only support SE mode.
FS mode has a different signature for the thread context (for some reason), so you have to instantiate the thread context dynamically if you want to support both modes.

Initialization
----------------

I think we also need to add some stuff to init and startup.

.. code-block:: c++

    void
    LearningSimpleCPU::init()
    {
        DPRINTF(LearningSimpleCPU, "LearningSimpleCPU init\n");

        BaseCPU::init();

        thread.getTC()->initMemProxies(thread.getTC());
    }

    void
    LearningSimpleCPU::startup()
    {
        DPRINTF(LearningSimpleCPU, "LearningSimpleCPU startup\n");

        BaseCPU::startup();

        thread.startup();
    }


So, with all this, nothing happens.
We just run until the simulation limit is reached.

I added the wakeup implemenation to call the thread context activate function.
Actually, this function isn't called with the simple script, only the starup function is called.

Activating the context
----------------------

When the thread context is activated, it in turn calls cpu->activateContext().
I don't quite understand this circular dependence, but I'll go with it for now.
I'm not sure when activateContext() is executed since wakeup() isn't called.

I'm not sure what the difference between the `ThreadContext` and the `SimpleThread` is.
According to a comment "Returns the pointer to this SimpleThread's ThreadContext. Used when a ThreadContext must be passed to objects outside of the CPU."
Not that helpful...

Well, I added an `activateContext` function.
But it seems like it causes an infinite loop.
Am I missing something?
I guess there isn't an infinite loop, but I don't understand how.

The order of initialization is the following:

::
    0: system.cpu: LearningSimpleCPU init
    0: system.cpu: ActivateContext thread: 0
    Beginning simulation!
    0: system.cpu: LearningSimpleCPU startup

It looks like the `activateContext` function is where we should kick off the import events to start fetching, etc.
Let's see how that goes.

Fetching instructions
---------------------

As part of the activation, we need to kick of instruction fetch.
I guess I'm going to use an event to kick that off and keep doing that event whenever an instruction commits.

Fetching needs to be split into two parts.
First, we must access the TLB with a translation state.
Then, when the TLB finishes (and calls our callback), we need to actually send the memory request.

The size of the fetch is `sizeof(MachInst)`.
This is an ISA-specif size.
And, it may not actually be the whole instruction that we need, since we haven't decoded the instruction yet.
This makes things very complicated.
For now, we're going to assume all instructions are the same size (and use RISC-V so this is true).
We'll talk more about instrcution size when we get to decoding.

Decoding instructions
---------------------

Once we have the instruction from memory, we need to decode it.
We decode it by using the `thread`'s decoder (this is ISA-specific).

To decode an instruction, we first call `decoder.moreBytes` and pass it the current bytes that we have fetched from memory.
Once we have given the decoder this information, we can try to decode the instruction.

When decoding, we may not have enough information (e.g., the instruction is bigger than `MachInst`).
If this is the case, the decoder's decode function will return null, and we'll have to go back and fetch some more data.
Then, once that data is fetched, we can call `moreBytes` again to give the decoder more data to work with and maybe then it can decode the instruction.

Executing instructions
----------------------

Non-memory instructions
========================

Let's start with the simpler case of instructions that don't access memory.

The `StaticInst` type has a `execute` function.
I think we can just call this.
(I'm still not totally clear where this class is implemented. I think it's in the ISA, but I don't know for sure.)

However, to do this, we need to implement an execution context.

Execution context
==================

This class mostly just wraps the thread context and calls different functions on the thread context to update and read registers.
It also allows the instruction to call back into the CPU for things it needs (like accessing memory).
I don't fully understand why this exists.

To do
======
* Make sure to fetch from Microcode ROM if it is needed. (See other CPU models.)
* Write up more details on things than what I have here. Write the chapter of the Learning gem5 book.
