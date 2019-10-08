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
#ifndef __VERILATO_AXI_TO_MEM_HH__
#define __VERILATO_AXI_TO_MEM_HH__

//gem5 model includes
#include "verilator/verilator_mem_black_box.hh"

//gem5 model includes
#include "params/AXIToMem.hh"

//Interface for different types of memory models
class AXIToMem: public VerilatorMemBlackBox
{
  public:
    //memory access functions for blackbox
    void doMem(unsigned int req_addr, unsigned char req_operation,
      unsigned char req_write_data);

    BaseMasterPort& getMasterPort( const std::string& if_name,
                PortID idx = InvalidPortID ) override;

    AXIToMem( AXIToMemParams *params);

    void startup() override;
    uint8_t getDmemResp();

  protected:
  //master port for blackbox
    class AXIToMemPort :
    public VerilatorMemBlackBox::VerilatorMemBlackBoxPort
    {
      public:

         AXIToMemPort(const std::string& name,
                    AXIToMem *owner) :
                    VerilatorMemBlackBoxPort(name, owner)
                { }

        bool sendTimingPacket(PacketPtr pkt);

        //currently need this for synchronization. tells gem5 to access mem
        //model immediatly instead of scheduling event to do so
        bool sendAtomicPacket(PacketPtr pkt);

      protected:
        bool recvTimingResp(PacketPtr pkt) override;

        void recvReqRetry() override;
    };
    public:
    //deal with data going back to axi responder
    bool handleResponse( PacketPtr pkt );

    AXIToMemPort dataPort;
    AXIToMemPort fdataPort;


    //data for response to dpi
    uint8_t dmemResp;
    //0 for atomic/tests, 1 for timing
    int memMode;
};
#endif
