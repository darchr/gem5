#include "base/logging.hh"
#include "debug/Verilator.hh"
#include "sim/sim_exit.hh"
#include "verilator_object.hh"

void
VerilatorObject::VerilatorCPUMemPort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

void
VerilatorObject::VerilatorCPUMemPort::recvReqRetry()
{
    assert(blockedPacket != nullptr);

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    sendPacket(pkt);
}

BaseMasterPort&
VerilatorObject::getMasterPort(const std::string& if_name, PortID idx)
{
    panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

    // This is the name from the Python SimObject declaration (SimpleMemobj.py)
    if (if_name == "instPort") {
        return instPort;
    } else if (if_name == "dataPort") {
        return dataPort;
    } else {
        // pass it along to our super class
        return MemObject::getMasterPort(if_name, idx);
    }
}

bool
VerilatorObject::VerilatorCPUMemPort::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(Verilator, "MEMORY RESPONSE RECIEVED\n");
    return owner->handleResponse(pkt);
}

PacketPtr
VerilatorObject::buildPacket(const RequestPtr &req, bool read)
{
    return read ? Packet::createRead(req) : Packet::createWrite(req);
}

void
VerilatorObject::buildPayloadForWrite(RequestPtr &data_req, uint64_t data,
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
                dut.Top__DOT__tile__DOT__memory__DOT__async_data_dw_addr,
                1, 0, 0);
        packeddata = new uint8_t;
    }else if ( half ){
        data_req = std::make_shared<Request>(
                dut.Top__DOT__tile__DOT__memory__DOT__async_data_dw_addr,
                2, 0, 0);
        packeddata = new uint8_t[2];
    }else{
        data_req = std::make_shared<Request>(
                dut.Top__DOT__tile__DOT__memory__DOT__async_data_dw_addr,
                4, 0, 0);
        packeddata = new uint8_t[4];
        for (int i = 0; i < 4; ++i){
                packeddata[i] = data & (0xFF << i);
            }
    }
    int j = 0;
    for (int i = 0; i < 4; ++i){
        //if the mask is active then use this byte in our packed data
        if (mask & (0x1 << i)){
                //get index of 1st occurance of a 1 for address offset
                if (j == 0)
                    j = i;
                packeddata[i] = data & (0xFF << i);
        }
    }
    //ofset address based on data size/type
    data_req->setPaddr(data_req->getPaddr() + j);
}

void
VerilatorObject::sendData(const RequestPtr &req, uint8_t *data, bool read)
{
    PacketPtr pkt = buildPacket(req, read);
    pkt->allocate();
    pkt->setData(data);
    delete[] data;

    dataPort.sendPacket(pkt);
    dataRequested = true;
    //Assert io.dmem.resp.valid to false to insert bubbles
    dut.Top__DOT__tile__DOT__memory__DOT__io_core_ports_0_resp_valid = 0;

}

void
VerilatorObject::sendFetch(const RequestPtr &req)
{
        DPRINTF(Verilator, "Sending fetch for addr (pa: %#x)\n",
               req->getPaddr());
        PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
        DPRINTF(Verilator, " -- pkt addr: %#x\n", pkt->getAddr());
        pkt->allocate();

        instRequested = true;
        dut.Top__DOT__tile__DOT__core__DOT__c_io_ctl_stall = 1;
        instPort.sendPacket(pkt);

        //assertn io.imem.resp.valid response to false to insert bubbles
}


bool
VerilatorObject::handleResponse(PacketPtr pkt)
{
    DPRINTF(Verilator, "Got response for addr %#x\n", pkt->getAddr());
    uint8_t * respData = new uint8_t[4];
    if (pkt->req->isInstFetch()) {
        //set packet data to sodor imem data signal
        //get read data
        pkt->writeData(respData);
        //concat response data to inst data out signal
        dut.Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_1_data =
                respData[3] | respData[2] | respData[1] | respData[0];

        DPRINTF(Verilator, "Response is data %#x\n",
            dut.Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_1_data );
        //set valid to true
        dut.Top__DOT__tile__DOT__core__DOT__c_io_ctl_stall = 0;
        //no imem valid signal. set stall instead?
        instRequested = false;
    } else  if (pkt->isRead()){
        //set packet data to sodor dmem data signal
        pkt->writeData(respData);
        dut.Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_0_data =
                respData[3] | respData[2] | respData[1] | respData[0];
        //set valid to true
        dut.Top__DOT__tile__DOT__memory__DOT__io_core_ports_0_resp_valid = 1;
        //no more outstanding requests so we can make another request
        //if need be
        dataRequested = false;
    }
    delete[] respData;
    return true;
}

VerilatorObject::VerilatorObject(VerilatorObjectParams *params) :
    MemObject(params),
    event([this]{updateCycle();}, params->name),
    latency(params->latency),
    designStages(params->stages),
    instPort(params->name + ".instPort", this),
    dataPort(params->name + ".dataPort", this)
{
    /*//send program or other data to fesvr to communicate
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
    }*/
}

VerilatorObject::~VerilatorObject()
{
    //delete dtm;
}

VerilatorObject*
VerilatorObjectParams::create()
{
    void* ptr = aligned_alloc(128, sizeof(VerilatorObject));
    return new(ptr) VerilatorObject(this);

}

void
VerilatorObject::updateCycle()
{

    DPRINTF(Verilator, "\nTICKING\n");
    //has verilator finished running?
    if (/*dtm->done() && dut.io_success && */Verilated::gotFinish()){
        inform("Simulation has Completed\n");
        exitSimLoop("Done Simulating", 1 /*dtm->exit_code()*/);
    }

    DPRINTF(Verilator, "INSTRUCTION FETCH?\n");
    //has verilator finished running?
    if (!dut.Top__DOT__tile__DOT__core__DOT__c_io_ctl_stall && !instRequested){
        DPRINTF(Verilator, "YES BECAUSE !STALL %d && !INSTREQUESTED %d\n",
               !dut.Top__DOT__tile__DOT__core__DOT__c_io_ctl_stall,
               !instRequested);

        RequestPtr ifetch_req = std::make_shared<Request>(
        dut.Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_1_addr,
        4,0x100,0);
        sendFetch(ifetch_req);
    }else if (instRequested){
        DPRINTF(Verilator, "NO BECAUSE INSTREQUESTED %d\n", instRequested);
        dut.Top__DOT__tile__DOT__core__DOT__c_io_ctl_stall = 1;
    }

    if (dut.Top__DOT__tile__DOT__memory__DOT__io_core_ports_0_req_valid
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
            dut.Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_0_addr,
            4, 0, 0);
                //this doesn't matter for a read, this data will do nothing
            packedData = new uint8_t[4];

            for (int i = 0; i < 4; ++i){
                packedData[i] =
                    dut.Top__DOT__tile__DOT__memory__DOT__async_data_dw_data &
                    (0xFF << i);
            }
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

    schedule(event, nextCycle());
}

void
VerilatorObject::reset(int resetCycles)
{
    DPRINTF(Verilator, "RESETING FOR %d CYCLES\n", resetCycles);

    //if we are pipelining we want to run reset for the number
    //of stages we have
    for (int i = 0; i < resetCycles; ++i){
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
    DPRINTF(Verilator, "DONE RESETING\n");
}

void
VerilatorObject::startup()
{
    DPRINTF(Verilator, "STARTING UP SODOR\n");
    reset(designStages);

    DPRINTF(Verilator, "SCHEDULING FIRST TICK IN %d\n", latency);
    schedule(event, latency);
}
