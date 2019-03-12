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

#include "VeriilatorMemBlackBox.hh"
#include "base/Logging.hh"

void VerilatorMemBlackBox::sendPacket(PacketPtr pkt){
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }

}

bool VerilatorMemBlackBox::recvTimingResp( PacketPtr pkt )
{
    DPRINTF(Verilator, "MEMORY RESPONSE RECIEVED\n");
    return owner->handleResponse(pkt);

}

void VerilatorMemBlackBox::recvReqRetry(){
    assert(blockedPacket != nullptr);

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    sendPacket(pkt);

}


//need to rewrite after changing memory.scala
VerilatorMemBlackBox::VerilatorMemBlackBox(
        VerilatorMemBlackBoxParams *params) :
    MemObject(params),
    instData(params->name + ".instPort", this),
    dataPort(params->name + ".dataPort", this)
{
}

VerilatorMemBlackBox::~VerilatorMemBlackBox()
{
}

void VerilatorMemBlackBox::doFetch()
{
     RequestPtr ifetch_req = std::make_shared<Request>(
        blkbox.Top__DOT__tile_,
        4,
        Request::Flags.INSTR_FETCH,
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
    //bool read = blkbox.TOP_DOT_tile_DOT_cpu_
    //DOT_memory_DOT_dmem_DOT_writeenable
    //if (blkbox.TOP_DOT_tile_DOT_cpu_DOT_
    //memory_DOT_dmem_DOT_maskmode)
    //determine size to access depending on above signal

    RequestPtr data_req = std::make_shared<Request>(
        blkbox.Top__DOT__tile_,
        maskmode,
        Request::Flags.PHYSICAL,
        0);

    DPRINTF(Verilator, "Sending data request for addr (pa: %#x)\n",
               data_req->getPaddr());

    PacketPtr pkt = read ? Packet::createRead(data_req)
        : Packet::createWrite(data_req);
    DPRINTF(Verilator, " -- pkt addr: %#x\n", pkt->getAddr());
    pkt->allocate();
    pkt->setData(data);
    delete[] data;

    dataPort.sendPacket(pkt);
}

bool VerilatorMemBlackBox::handleResponse( PacketPtr pkt )
{
 DPRINTF(Verilator, "Got response for addr %#x\n", pkt->getAddr());
    const uint32_t *t =  pkt->getConstPtr<uint32_t>();
    if (pkt->req->isInstFetch()) {

    } else  if (pkt->isRead()){
        //set packet data to sodor dmem data signal
        const uint32_t *tmp = pkt->getConstPtr<uint32_t>();
    }
    return true;

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
