/*# Copyright (c) 2019 The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Nima Ganjehloo
*/

#include "base/logging.hh"
#include "debug/Verilator.hh"
#include "verilator_mem_black_box.hh"

//MASTER PORT DEFINITIONS
void VerilatorMemBlackBox::VerilatorMemBlackBoxPort::sendPacket(PacketPtr pkt){
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }

}

bool VerilatorMemBlackBox::VerilatorMemBlackBoxPort::recvTimingResp
    ( PacketPtr pkt )
{
    DPRINTF(Verilator, "MEMORY RESPONSE RECIEVED\n");
    return owner->handleResponse(pkt);

}

void VerilatorMemBlackBox::VerilatorMemBlackBoxPort::recvReqRetry(){
    assert(blockedPacket != nullptr);

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    sendPacket(pkt);

}

BaseMasterPort& VerilatorMemBlackBox::getMasterPort(
        const std::string& if_name, PortID idx )
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


//Create for memblkbx
VerilatorMemBlackBox*
VerilatorMemBlackBoxParams::create(){
    return new VerilatorMemBlackBox(this);
}
//need to rewrite after changing memory.scala
VerilatorMemBlackBox::VerilatorMemBlackBox(
        VerilatorMemBlackBoxParams *params) :
    MemObject(params),
    instPort(params->name + ".instPort", this),
    dataPort(params->name + ".dataPort", this)
{
}

void VerilatorMemBlackBox::doFetch()
{
     RequestPtr ifetch_req = std::make_shared<Request>(
        blkbox->imem_address,
        4,
        Request::INST_FETCH,
        0);

     DPRINTF(Verilator, "Sending fetch for addr (pa: %#x)\n",
               ifetch_req->getPaddr());
     PacketPtr pkt = new Packet(ifetch_req, MemCmd::ReadReq);
     DPRINTF(Verilator, " -- pkt addr: %#x\n", pkt->getAddr());
     pkt->allocate();
     instPort.sendPacket(pkt);

}

void VerilatorMemBlackBox::doMem()
{
    //need more stuff here
    unsigned int maskmode = 4;
    bool read = blkbox->dmem_memread;
    //determine size to access depending on above signal
    if ( blkbox->dmem_maskmode != 2 )
        maskmode = 4;
    else if ( blkbox->dmem_maskmode == 0 )
        maskmode = 2;
    else
        maskmode = 1;

    RequestPtr data_req = std::make_shared<Request>(
        blkbox->dmem_address,
        maskmode,
        Request::PHYSICAL,
        0);

    DPRINTF(Verilator, "Sending data request for addr (pa: %#x)\n",
               data_req->getPaddr());

    PacketPtr pkt = read ? Packet::createRead(data_req)
        : Packet::createWrite(data_req);
    DPRINTF(Verilator, " -- pkt addr: %#x\n", pkt->getAddr());

    uint8_t * data = new uint8_t[4];
    for (int i = 0; i < 4; ++i){
        data[i] = blkbox->dmem_writedata & (0xFF << i);
    }
    pkt->allocate();
    pkt->setData(data);
    delete[] data;

    dataPort.sendPacket(pkt);
}

bool VerilatorMemBlackBox::handleResponse( PacketPtr pkt )
{
    DPRINTF(Verilator, "Got response for addr %#x\n", pkt->getAddr());

    if (pkt->req->isInstFetch()) {
        blkbox->imem_dataout = *pkt->getConstPtr<uint32_t>();
    } else  if (pkt->isRead()){
        //set packet data to sodor dmem data signal
        blkbox->dmem_dataout = *pkt->getConstPtr<uint32_t>();
    }

    return true;

}
