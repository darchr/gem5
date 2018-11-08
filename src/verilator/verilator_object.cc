#include "base/logging.hh"
#include "sim/sim_exit.hh"
#include "verilator_object.hh"

void
VerilatorCPUMemPort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

void
VerilatorCPUMemPort::recvReqRetry()
{
    assert(blockedPacket != nullptr);

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    sendPacket(pkt);
}

bool
VerilatorCPUMemPort::recvTimingResp(PacketPtr pkt)
{
    return owner->handleResponse(pkt);
}

PacketPtr
VerilatorObject::buildPacket(const RequestPtr &req, bool read)
{
    return read ? Packet::createRead(req) : Packet::createWrite(req);
}

/* Need to pack data correctly for halfwords and byte write requestes
void
VerilatorObject::buildPayload(uint8_t mask, uint8_t * data,
    uint8_t * packeddata)
{
    bool half = mask == 0x3 || mask == 0x6 || mask == 12;
    bool byte = mask == 0x1 || mask == 0x2 || mask == 0x4 || mask ==0x8;
    if (byte){
        packeddata = new uint8_t;
    }else if ( half ){
        packeddata = new uint8_t[2];
    }else{
        packeddata == data;
    }
    for (int i = 0; i < 4; ++i){
        if (mask & (0x1 << i))
                packeddata[i] = data[i];
    }
}*/

void
VerilatorObject::sendData(const RequestPtr &req, uint8_t *data, bool read)
{
    PacketPtr pkt = buildPacket(req, read);
    pkt->dataDynamic<uint8_t>(data);

    dataPort.sendPacket(pkt)
    dataRequested = true;
    //Assert io.dmem.resp.valid to false to insert bubbles
    dut.Top__DOT__tile__DOT__memory__DOT__io_core_ports_0_resp_valid = 0;

}

void
VerilatorObject::sendFetch(const RequestPtr &req)
{
        DPRINTF(this, "Sending fetch for addr %#x(pa: %#x)\n",
                req->getVaddr(), req->getPaddr());
        PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
        DPRINTF(this, " -- pkt addr: %#x\n", pkt->getAddr());

        instPort.sendPacket(pkt)

        instRequested = true;
        //assertn io.imem.resp.valid response to false to insert bubbles
}


bool
VerilatorObject::handleResponse(PacketPtr pkt)
{
    DPRINTF(this, "Got response for addr %#x\n", pkt->getAddr());

    if (pkt->req->isInstFetch()) {
        //set packet data to sodor imem data signal
        //dut.Top__DOT__tile__DOT__memory__DOT__async
        //_data_dataInstr_1_data = pkt->
        //set valid to true
        //no imem valid signal. set stall instead?
        instRequested = false;
    } else {
        //set packet data to sodor dmem data signal
        //dut.Top__DOT__tile__DOT__memory__DOT__async_
        //data_dataInstr_0_data = pkt->
        //set valid to true
        dut.Top__DOT__tile__DOT__memory__DOT__io_core_ports_0_resp_valid = 1;
        dataRequested = false;
    }
    return true;
}

VerilatorObject::VerilatorObject(VerilatorObjectParams *params) :
    SimObject(params),
    event([this]{processEvent();}, params->name),
    maxCycles(params->cycles),
    latency(params->latency),
   designStages(params->stages),
   start(params->startTime),
    loadMem(params->memData.c_str()),
   objName(params->name)
{
    //send program or other data to fesvr to communicate
    //with design
    toDtm.push_back(loadMem);
    if (!toDtm.size()){
        //DPRINT
        fatal("No binary specified for emulator\n");
    }

    inform("Instantiated DTM.\n");
    //give fesvr list of things to load/parse
    dtm = new dtm_t(toDtm);
    if (!dtm){
        panic("Could not allocate dtm\n");
    }
}

VerilatorObject::~VerilatorObject()
{
    delete dtm;
}

VerilatorObject*
VerilatorObjectParams::create()
{
    VerilatorObject * vo = new VerilatorObject(this);
    return vo;
}

void
VerilatorObject::processEvent()
{
    //has verilator finished running?
    if (dtm->done() && dut.io_success && Verilated::gotFinish()){
        inform("Simulation has Completed\n");
        exitSimLoop("Done Simulating", dtm->exit_code());
    }

    if (/*dut.io_imem_req_valid*/ && !instRequested){
        RequestPtr ifetch_req = std::make_shared<Request>();
        setupFetchRequest(ifetch_req);
        sendFetch(ifetch_req);
    }else if (instRequested){
        //stall
    }

    if (Top__DOT__tile__DOT__memory__DOT__io_core_ports_0_req_valid
        && !dataRequested){
        RequestPtr data_req = std::make_shared<Request>();
        data_req->setPaddr(
        dut.Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_0_addr);
        //sendData(data_req, dut.,
        //dut.Top__DOT__tile__DOT__memory__DOT__async_data_dw_en);
    }else if ( dataRequested ){
        dut.Top__DOT__tile__DOT__memory__DOT__io_core_ports_0_resp_valid = 0;
    }

    //run the device under test here through verilator
    dut.clock = 0;
    dut.eval();
    dut.clock = 1;
    dut.eval();

    //check if we should stop
    cyclesPassed += 1;
    if (maxCycles != 0 && cyclesPassed == maxCycles){
        inform("Simulation Timed Out\n");
        exitSimLoop("Done Simulating", dtm->exit_code());
    }

    schedule(event, curTick() + latency);
}

void
VerilatorObject::reset(int cycles)
{
    //if we are pipelining we want to run reset for the number
    //of stages we have
    for (int i = 0; i < cycles; ++i){
        //set reset signal and starting clock signal
        dut.reset = 1;
        dut.clock = 0;
        //run verilator for this state
        dut.eval();
        //run verilator for rising edge state
        dut.clock = 1;
        dut.eval();
        //done reseting
        dut.reset = 0;
    }
}

void
VerilatorObject::startup()
{
    reset(designStages);
    schedule(event, latency);
}
