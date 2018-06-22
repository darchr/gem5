
#include "learning_gem5/part4/lsc_thread_context.hh"

#include "base/output.hh"

LSCThreadContext::LSCThreadContext(LearningSimpleCPU *cpu, System *system
                                   Process *process) :
    cpu(cpu), system(system), process(process), contextId(-1), threadId(0),
    status(ThreadContext::Halted), lastActivate(0), lastSuspend(0)


void
LSCThreadContext::initMemProxies(ThreadContext *tc)
{
    // The port proxies only refer to the data port on the CPU side
    // and can safely be done at init() time even if the CPU is not
    // connected, i.e. when restoring from a checkpoint and later
    // switching the CPU in.
    if (FullSystem) {
        assert(physProxy == NULL);
        // This cannot be done in the constructor as the thread state
        // itself is created in the base cpu constructor and the
        // getDataPort is a virtual function
        physProxy = new PortProxy(cpu->getDataPort(),
                                  cpu->cacheLineSize());

        assert(virtProxy == NULL);
        virtProxy = new FSTranslatingPortProxy(tc);
    } else {
        assert(proxy == NULL);
        proxy = new SETranslatingPortProxy(cpu->getDataPort(),
                                           process,
                                           SETranslatingPortProxy::NextPage);
    }
}

void
LSCThreadContext::activate()
{
    if (status() == ThreadContext::Active)
        return;

    lastActivate = curTick();
    status = ThreadContext::Active;

    // Note: In the CPU function it calls LSCThreadContext::activate. This is
    // why we have to set the context as active before calling this function.
    // TODO: Check if the above is actually necessary. Could we *not* call
    // activate in the CPU or here? Is one of them redundant? It could be that
    // some things call this first and other call CPU first.
    cpu->activateContext(threadId);
}

void
LSCThreadContext::suspend()
{
    if (status() == ThreadContext::Suspended)
        return;

    lastActivate = curTick();
    lastSuspend = curTick();
    status = ThreadContext::Suspended;
    baseCpu->suspendContext(threadId);
}

void
LSCThreadContext::clearArchRegs()
{
    pcState = 0;
    registers.clearRegisters();
    isa->clear();
}
