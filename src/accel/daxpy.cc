
#include "accel/daxpy.hh"

#include <string>

#include "cpu/translation.hh"
#include "debug/Accel.hh"
#include "mem/packet_access.hh"

using namespace std;

Daxpy::Daxpy(const Params *p) :
    BasicPioDevice(p, 4096), completedIterations(0),
    memoryPort(p->name+".memory_port", this), monitorAddr(0), paramsAddr(0),
    context(nullptr), tlb(p->tlb), maxUnroll(p->max_unroll),
    status(Uninitialized), runEvent(this)
{
    assert(this->pioAddr == p->pio_addr);
}

BaseMasterPort &
Daxpy::getMasterPort(const string &if_name, PortID idx)
{
    if (if_name == "memory_port") {
        return memoryPort;
    } else {
        return MemObject::getMasterPort(if_name, idx);
    }
}


AddrRangeList
Daxpy::getAddrRanges() const
{
    AddrRangeList ranges;

    DPRINTF(Accel, "dispatcher registering addr range at %#x size %#x\n",
            pioAddr, pioSize);

    ranges.push_back(RangeSize(pioAddr, pioSize));

    return ranges;
}

void
Daxpy::open(ThreadContext *tc)
{
    DPRINTF(Accel, "Got an open request\n");
    context = tc;
}

Tick
Daxpy::read(PacketPtr pkt)
{
    assert(pkt->getAddr() >= pioAddr);
    assert(pkt->getAddr() < pioAddr + pioSize);

    DPRINTF(Accel, "Got a read request %s", pkt->print());
    DPRINTF(Accel, "Data %#x\n", pkt->get<uint64_t>());

    pkt->set(12);
    pkt->makeAtomicResponse();
    return 1;
}

Tick
Daxpy::write(PacketPtr pkt)
{
    assert(pkt->getAddr() >= pioAddr);
    assert(pkt->getAddr() < pioAddr + pioSize);

    DPRINTF(Accel, "Got a write request %s", pkt->print());
    DPRINTF(Accel, "Data %#x\n", pkt->get<uint64_t>());

    if (monitorAddr == 0) {
        monitorAddr = pkt->get<uint64_t>();
    } else if (paramsAddr == 0) {
        paramsAddr = pkt->get<uint64_t>();
    } else {
        panic("Too many writes to Daxpy!");
    }

    if (monitorAddr != 0 && paramsAddr !=0) {
        status = Initialized;
        schedule(runEvent, clockEdge(Cycles(1)));
    }

    pkt->makeAtomicResponse();
    return 1;
}

void
Daxpy::runDaxpy()
{
    assert(status == Initialized);

    loadParams();
}

void
Daxpy::loadParams()
{
    status = GettingParams;
    assert(paramsAddr != 0);

    paramsLoaded = 0;

    uint8_t *data_X = new uint8_t[8];
    accessMemory(paramsAddr, 8, BaseTLB::Read, data_X);

    uint8_t *data_Y = new uint8_t[8];
    accessMemory(paramsAddr+8, 8, BaseTLB::Read, data_Y);

    uint8_t *data_alpha = new uint8_t[8];
    accessMemory(paramsAddr+16, 8, BaseTLB::Read, data_alpha);

    uint8_t *data_N = new uint8_t[4];
    accessMemory(paramsAddr+24, 4, BaseTLB::Read, data_N);
}

void
Daxpy::recvParam(PacketPtr pkt)
{
    if (pkt->req->getVaddr() == paramsAddr) {
        pkt->writeData((uint8_t*)&daxpyParams.X);
    } else if (pkt->req->getVaddr() == paramsAddr+8) {
        pkt->writeData((uint8_t*)&daxpyParams.Y);
    } else if (pkt->req->getVaddr() == paramsAddr+16) {
        pkt->writeData((uint8_t*)&daxpyParams.alpha);
    } else if (pkt->req->getVaddr() == paramsAddr+24) {
        pkt->writeData((uint8_t*)&daxpyParams.N);
    } else {
        panic("recv. response for address not expected while getting params");
    }

    paramsLoaded++;

    if (paramsLoaded == 4) {
        executeLoop();
    }
}

void
Daxpy::executeLoop()
{
    status = ExecutingLoop;

    DPRINTF(Accel, "Got all of the params!\n");
    DPRINTF(Accel, "X: %#x, Y: %#x, alpha: %f, N: %d\n", daxpyParams.X,
            daxpyParams.Y, daxpyParams.alpha, daxpyParams.N);

    new LoopIteration(maxUnroll, daxpyParams, this);
}

Daxpy::LoopIteration::LoopIteration(int step, FuncParams params, Daxpy* accel):
    i(0), step(step), x(0), y(0), stage(0), params(params),
    accel(accel), runStage2(this), runStage3(this), runStage4(this)
{
    DPRINTFS(Accel, accel, "Initializing loop iterations %d\n", step);
    for (int i=0; i<step && i<params.N; i++) {
        DPRINTFS(Accel, accel, "New iteration for %d\n", i);
        new LoopIteration(i, step, params, accel);
    }
}

void
Daxpy::LoopIteration::stage1()
{
    // Load Y[i]
    uint8_t *y = new uint8_t[8];
    stage = 1;
    accel->accessMemoryCallback(params.Y+8*i, 8, BaseTLB::Read, y, this);
}

void
Daxpy::LoopIteration::stage2()
{
    // Load X[i]
    uint8_t *x = new uint8_t[8];
    stage = 2;
    accel->accessMemoryCallback(params.X+8*i, 8, BaseTLB::Read, x, this);
}

void
Daxpy::LoopIteration::stage3()
{
    double tmp = y;
    double ans = x * params.alpha + y;
    uint8_t *y = new uint8_t[8];
    *(double*)y = ans;
    DPRINTF(Accel, "Performing %f * %f + %f = %f\n",
            params.alpha, x, tmp, ans);

    stage = 3;
    accel->accessMemoryCallback(params.Y+8*i, 8, BaseTLB::Write, y, this);
}

void
Daxpy::LoopIteration::stage4()
{
    accel->completedIterations++;
    if (i+step < params.N) {
        DPRINTFS(Accel, accel, "New iteration for %d\n", i+step);
        new LoopIteration(i+step, step, params, accel);
    } else if (accel->completedIterations == params.N) {
        accel->sendFinish();
    }

    delete this;
}

void
Daxpy::LoopIteration::recvResponse(PacketPtr pkt)
{
    // Note: each stage should happen 1 cycle after the response
    switch (stage) {
        case 1:
            pkt->writeData((uint8_t*)&y);
            accel->schedule(runStage2, accel->nextCycle());
            break;
        case 2:
            pkt->writeData((uint8_t*)&x);
            accel->schedule(runStage3, accel->nextCycle());
            break;
        case 3:
            accel->schedule(runStage4, accel->nextCycle());
            break;
        default:
            panic("Don't know what to do with this response!");
    }
}

void
Daxpy::sendFinish()
{
    status = Returning;

    DPRINTF(Accel, "Sending finish daxpy\n");

    uint8_t *data = new uint8_t[4];
    *(int*)data = 12;

    accessMemory(monitorAddr, 4, BaseTLB::Write, data);
}

void
Daxpy::accessMemoryCallback(Addr addr, int size, BaseTLB::Mode mode,
                              uint8_t *data, LoopIteration *iter)
{
    setAddressCallback(addr, iter);
    accessMemory(addr, size, mode, data);
}

void
Daxpy::accessMemory(Addr addr, int size, BaseTLB::Mode mode, uint8_t *data)
{
    RequestPtr req = new Request(-1, addr, size, 0, 0, 0, 0, 0);

    DPRINTF(Accel, "Tranlating for addr %#x\n", req->getVaddr());

    WholeTranslationState *state =
                new WholeTranslationState(req, data, NULL, mode);
    DataTranslation<Daxpy*> *translation
            = new DataTranslation<Daxpy*>(this, state);

    tlb->translateTiming(req, context, translation, mode);
}

void
Daxpy::finishTranslation(WholeTranslationState *state)
{
    if (state->getFault() != NoFault) {
        panic("Page fault in Daxpy. Addr: %#x", state->mainReq->getVaddr());
    }

    DPRINTF(Accel, "Got response for translation. %#x -> %#x\n",
            state->mainReq->getVaddr(), state->mainReq->getPaddr());

    sendData(state->mainReq, state->data, state->mode == BaseTLB::Read);

    delete state;
}

void
Daxpy::sendData(RequestPtr req, uint8_t *data, bool read)
{
    DPRINTF(Accel, "Sending request for addr %#x\n", req->getPaddr());

    PacketPtr pkt = read ? Packet::createRead(req) : Packet::createWrite(req);
    pkt->dataDynamic<uint8_t>(data);

    memoryPort.schedTimingReq(pkt, nextCycle());
}

void
Daxpy::setAddressCallback(Addr addr, LoopIteration* iter)
{
    addressCallbacks[addr] = iter;
}

void
Daxpy::recvLoop(PacketPtr pkt)
{
    auto it = addressCallbacks.find(pkt->req->getVaddr());
    if (it == addressCallbacks.end()) {
        panic("Can't find address in loop callback");
    }

    LoopIteration *iter = it->second;
    iter->recvResponse(pkt);
}

bool
Daxpy::MemoryPort::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(Accel, "Got a response for addr %#x\n", pkt->req->getVaddr());

    Daxpy& daxpy = dynamic_cast<Daxpy&>(owner);

    if (daxpy.status == GettingParams) {
        daxpy.recvParam(pkt);
    } else if (daxpy.status == ExecutingLoop) {
        daxpy.recvLoop(pkt);
    } else if (daxpy.status == Returning) {
        // No need to process
    } else {
        panic("Got a memory response at a bad time");
    }

    delete pkt->req;
    delete pkt;

    return true;
}

Daxpy*
DaxpyParams::create()
{
    return new Daxpy(this);
}

DaxpyDriver*
DaxpyDriverParams::create()
{
    return new DaxpyDriver(this);
}
