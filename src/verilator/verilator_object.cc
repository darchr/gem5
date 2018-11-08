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

void
VerilatorObject::buildPayloadForWrite(RequestPtr &data_req, uint8_t * data,
        uint8_t * &packeddata)
{
    //get mask and figure out if we are trying to write a byte, half word or
    //a word
    uint8_t mask = dut.Top__DOT__tile__DOT__memory__DOT__async_data_dw_mask;
    bool half = mask == 0x3 || mask == 0x6 || mask == 12;
    bool byte = mask == 0x1 || mask == 0x2 || mask == 0x4 || mask ==0x8;
    //setup request for appropiatly sized packet
    if (byte){
        data_req = std::make_shared<Request>(
                new Request(
                dut.Top__DOT__tile__DOT__memory__DOT__async_data_dw_addr),
                1);
        packeddata = new uint8_t;
    }else if ( half ){
        data_req = std::make_shared<Request>(
                new Request(
                dut.Top__DOT__tile__DOT__memory__DOT__async_data_dw_addr),
                2);
        packeddata = new uint8_t[2];
    }else{
        data_req = std::make_shared<Request>(
                new Request(
                dut.Top__DOT__tile__DOT__memory__DOT__async_data_dw_addr),
                4);
        packeddata = data;
    }
    int j = 0;
    for (int i = 0; i < 4; ++i){
        //if the mask is active then use this byte in our packed data
        if (mask & (0x1 << i)){
                //get index of 1st occurance of a 1 for address offset
                if (j == 0)
                    j = i;
                packeddata[i] = data[i];
        }
    }
    //ofset address based on data size/type
    data_req->setPaddr(data_req->getPaddr() + j);
}

void
VerilatorObject::sendData(const RequestPtr &req, uint8_t *data, bool read)
{
    PacketPtr pkt = buildPacket(req, read);
    pkt->setData(data);
    pkt->allocate();

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
        Top__DOT__tile__DOT__core__DOT__c_io_ctl_stall = 1;
}


bool
VerilatorObject::handleResponse(PacketPtr pkt)
{
    DPRINTF(this, "Got response for addr %#x\n", pkt->getAddr());

    if (pkt->req->isInstFetch()) {
        //set packet data to sodor imem data signal
        uint8_t * respData;
        //get read data
        pkt->writeData(respData);
        //concat response data to inst data out signal
        dut.Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_1_data =
                respData[3] | respData[2] | respData[1] | respData[0];
        //set valid to true
        Top__DOT__tile__DOT__core__DOT__c_io_ctl_stall = 0;
        //no imem valid signal. set stall instead?
        instRequested = false;
    } else  if (pkt->req->isRead()){
        //set packet data to sodor dmem data signal
        uint8_t * respData;
        pkt->writeData(respData);
        dut.Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_0_data =
                respData[3] | respData[2] | respData[1] | respData[0];
        //set valid to true
        dut.Top__DOT__tile__DOT__memory__DOT__io_core_ports_0_resp_valid = 1;
        //no more outstanding requests so we can make another request
        //if need be
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
        dut.Top__DOT__tile__DOT__core__DOT__c_io_ctl_stall = 1;
    }

    if (Top__DOT__tile__DOT__memory__DOT__io_core_ports_0_req_valid
        && !dataRequested){
        RequestPtr data_req;
        uint8_t * packedData;
        bool write = dut.Top__DOT__tile__DOT__memory__DOT__async_data_dw_en;
        //if we are writing pack the data properly for writing
        if (write){
            buildPayloadForWrite(data_req,
                dut.Top__DOT__tile__DOT__memory__DOT__async_data_dw_data,
                packedData);
        }else{
        //if we are reading then just
            data_req = std::make_shared<Request>(
            new Request(
            dut.Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_0_addr),
            4);
                //this doesn't matter for a read, this data will do nothing
            packeddata =
                dut.Top__DOT__tile__DOT__memory__DOT__async_data_dw_data;
        }
        sendData(data_req, packedData, !write);
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
