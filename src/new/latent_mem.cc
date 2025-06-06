#include "mem/latent_mem.hh"
#include "base/trace.hh"
#include "debug/LatentMem.hh"
#include "sim/system.hh"
#include "sim/sim_exit.hh"

LatentMem::LatentMem(const LatentMemParams *p)
    : SimObject(p), latency(p->latency)
{
    cpuPort = new CPUSidePort(name() + ".cpu_side", this);
    memPort = new MemSidePort(name() + ".mem_side", this);
}

BaseSlavePort& LatentMem::getSlavePort(const std::string &name, PortID)
{
    return *cpuPort;
}

BaseMasterPort& LatentMem::getMasterPort(const std::string &name, PortID)
{
    return *memPort;
}

// === CPUSidePort ===

LatentMem::CPUSidePort::CPUSidePort(const std::string& name, LatentMem* owner)
    : SlavePort(name, owner), owner(owner) {}

Tick LatentMem::CPUSidePort::recvAtomic(PacketPtr pkt)
{
    return owner->memPort->sendAtomic(pkt);
}

void LatentMem::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    owner->memPort->sendFunctional(pkt);
}

void LatentMem::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    owner->schedule(new EventFunctionWrapper([=] {
        owner->memPort->sendTimingReq(pkt);
    }, owner->name()), curTick() + owner->latency);
}

AddrRangeList LatentMem::CPUSidePort::getAddrRanges() const
{
    return owner->memPort->getAddrRanges();
}

// === MemSidePort ===

LatentMem::MemSidePort::MemSidePort(const std::string& name, LatentMem* owner)
    : MasterPort(name, owner), owner(owner) {}

bool LatentMem::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    return owner->cpuPort->sendTimingResp(pkt);
}
