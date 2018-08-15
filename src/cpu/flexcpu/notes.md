
- Goal #1: Write a Simple Dataflow CPU with no register renaming, branch
  prediction, etc. This CPU should fetch as many instructions as possible,
  and execute them in 1? cycle as soon as all operands are available. Commit
  in-order for exception purposes.


- BaseCPU on doxygen might refer to wrong location. Actual base definition is
  in src/cpu/base.hh.
- Remember to ask about license details

- ProxyThreadContext class allows SimpleThread to behave as a ThreadContext,
  even though it is actually inheriting from ThreadState.

- It seems the concept of a thread within the framework is actually more like a
  core of the CPU, and should probably hold its own code. The BaseCPU interface
  is actually really lightweight. It feels like the existing CPU models don't
  actually make this distinction, and therefore have slightly more complex
  code. A reoccuring theme is that a function's first action is to grab a
  reference to a single thread's state, then operate only on that thread
  anyway. It might make more sense to define a thread class, and have such
  functions become members of that class. (Discussed this with Jason, threads
  are supposed to be like hyperthreads, and CPUs are like cores)


- A decision was made to split the logic of the CPU such that control flow was
  initiated by the logical thread, but resources were managed by the CPU object
  itself. This should allow us to decouple the logic of executing instructions
  in dataflow fashion from the details of managing hardware resources. I don't
  know if this will hold true, but my thought is that it will make it easy to
  minimize necessary changes to logic, and keep our tunable "dials" in the CPU
  object that the logical thread will attempt to access.

# Terminology:
- In-flight == Any instruction that has been decoded but has not yet been
  committed
- Waiting == An instruction whose dependencies have not yet been met
- Executing == All dependencies have been met, so we are just waiting for the
  result of the instruction, such as for memory, or complex arithmetic.

# Ideas:
- Hold reference to a SimpleThread object for purposes of true state. Hold
  speculative state in an ExecContext implementation as a "ROB" of sorts. Make
  sure to flush speculative state upon access to true state through the
  ThreadContext interface. (This is similar to how O3 seems to do it).
