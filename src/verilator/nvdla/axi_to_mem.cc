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

///verilator inlcudes
#include "svdpi.h"

//gem5 gemeral includes
#include "base/logging.hh"
#include "debug/Verilator.hh"
#include "mem/packet_access.hh"

//gem5 model includes
#include "axi_to_mem.hh"

//MASTER PORT DEFINITIONS
//send packet to gem5 memory system,schedules an event
bool
AXIToMem::AXIToMemPort
  ::sendTimingPacket(PacketPtr pkt)
{
  panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
  //send packet or block it and save packet for a retry
  if (!sendTimingReq(pkt)) {
    DPRINTF(Verilator, "Packet for addr: %#x blocked\n", pkt->getAddr());
    blockedPacket = pkt;
  }else{
    DPRINTF(Verilator, "TIMING MEMORY RESPONSE RECIEVED\n");
    //let blackbox determine how to deal with returned data
    return static_cast<AXIToMem *>(owner)->handleResponse(pkt);
  }
  return false;
}

//sends a packet to gem5 memory system, recieves response
//immidiately at end of call chain
bool
AXIToMem::AXIToMemPort
  ::sendAtomicPacket(PacketPtr pkt)
{
  panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
  //send packet or block it and save packet for a retry
  //block if response latency is non zero
  //
  //....i dont think blocking the packet here is right. do atomics ever get
  //request retries?
  if (sendAtomic(pkt)) {
    DPRINTF(Verilator, "Packet for addr: %#x blocked\n", pkt->getAddr());
    blockedPacket = pkt;
  }else{
    DPRINTF(Verilator, "ATOMIC MEMORY RESPONSE RECIEVED\n");
    //let blackbox determine how to deal with returned data
    return static_cast<AXIToMem *>(owner)->handleResponse(pkt);
  }
  return false;
}

//gem5 memory model has reponded to our memory request
bool
AXIToMem::AXIToMemPort
    ::recvTimingResp( PacketPtr pkt )
{
  DPRINTF(Verilator, "MEMORY RESPONSE RECIEVED\n");
  //let blackbox determine how to deal with returned data
  return static_cast<AXIToMem *>(owner)->handleResponse(pkt);
}

//retry sending a packet if it failed
void
AXIToMem::AXIToMemPort
  ::recvReqRetry()
{
  //we have to had saved the failed packet before doing a retry
  assert(blockedPacket != nullptr);

  //move data around
  PacketPtr pkt = blockedPacket;
  blockedPacket = nullptr;

  //try request again
  sendTimingPacket(pkt);
}

//used for configuring simulated device
BaseMasterPort&
AXIToMem::getMasterPort(
        const std::string& if_name, PortID idx )
{
  panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

  // This is the name from the Python SimObject declaration (SimpleMemobj.py)
  if (if_name == "dbb_data_port") {
    return dataPort;
  } else if ( if_name == "sram_data_port") {
    return fdataPort;
  }else{
    // pass it along to our super class
    return MemObject::getMasterPort(if_name, idx);
  }
}


//Create for memblkbx for gem5
AXIToMem*
AXIToMemParams::create()
{
  return new AXIToMem(this);
}

//setup our black box
AXIToMem::AXIToMem(
        AXIToMemParams *params) :
        VerilatorMemBlackBox(params),
        dataPort(params->name + ".dbb_data_port", this),
        fdataPort(params->name + ".sram_data_port", this),
        memMode(params->mem_mode)
{
}

//sets up a memory request for memory instructions to gem5 memory sytem
void
AXIToMem::doMem(unsigned int req_addr, unsigned char req_operation,
      unsigned char req_write_data)
{
  //are we reading or writing?
  bool operation = req_operation;


  //make request for corresponding byte size at the specified address provided
  //by dinocpu
  RequestPtr data_req = std::make_shared<Request>(
    req_addr,
    1,
    Request::PHYSICAL,
    0);

  DPRINTF(Verilator, "Sending data request for addr (pa: %#x)\n",
    data_req->getPaddr());

  //Is the packet a read or write request?

  PacketPtr pkt = nullptr;
  if (!operation){
      pkt = Packet::createRead(data_req);
  }else{
      pkt = Packet::createWrite(data_req);
  }

  DPRINTF(Verilator, " -- pkt addr: %#x\n", pkt->getAddr());

  //not sure if I need this. This is for non 4 byte writes.
  uint8_t * data = new uint8_t[1];
  if (operation){
    DPRINTF(Verilator, "WRITING %x AS %d BYTES\n",
      req_write_data, 1);
      data[0] = req_write_data;

    DPRINTF(Verilator, "DATA TO WRITE IS %x\n", data[0]);
  }

  //allocate space for memory request
  pkt->allocate();
  pkt->setLE(req_write_data);
  delete[] data;

  //send request
  if ( memMode )
    dataPort.sendTimingPacket(pkt);
  else
    dataPort.sendAtomicPacket(pkt);

}

//handles a successful response for a memory request
bool
AXIToMem::handleResponse( PacketPtr pkt )
{
  DPRINTF(Verilator, "Got response for addr %#x\n", pkt->getAddr());

  if (pkt->isRead()){
    //Get returned data from gem5 mem system and set it to blackbox
    DPRINTF(Verilator, "Handling response for data read\n");
    dmemResp = *pkt->getConstPtr<uint8_t>();
    DPRINTF(Verilator, "Data is %#x\n", dmemResp);
  }

  return true;
}

//setup reference to this class for use with dpi
void
AXIToMem::startup()
{
  DPRINTF(Verilator, "MEM BLACKBOX STARTUP\n");
}

uint8_t
AXIToMem::getDmemResp()
{
  return dmemResp;
}
