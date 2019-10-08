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
#ifndef __VERILATOR_NON_COMB_MEM_BLACK_BOX__HH__
#define __VERILATOR_NON_COMB_MEM_BLACK_BOX__HH__
///verilator inlcudes
#include "svdpi.h"

//gem5 model includes
#include "verilator/verilator_mem_black_box.hh"

//gem5 model includes
#include "params/NonCombMemBlackBox.hh"

class NonCombMemBlackBox: public VerilatorMemBlackBox
{
  public:
    //memory access functions for blackbox
    void doFetch(unsigned char imem_request_ready,
      unsigned char imem_request_valid, unsigned int imem_request_bits_address,
      unsigned char* imem_response_valid);

    void doMem(unsigned char dmem_request_ready,
      unsigned char dmem_request_valid, int dmem_request_bits_address,
      int dmem_request_bits_writedata,
      unsigned char dmem_request_bits_operation,
      unsigned char* dmem_response_valid);

    BaseMasterPort& getMasterPort( const std::string& if_name,
                PortID idx = InvalidPortID ) override;

    //param setup for blackbox warpper
    NonCombMemBlackBox( NonCombMemBlackBoxParams *params );

    //setsup singleton for use with dpi getters
    void startup() override;
    static NonCombMemBlackBox * getSingleton();
    uint32_t getDmemResp();
    uint32_t getImemResp();

  private:
    //master port for blackbox
    class NonCombMemBlackBoxPort :
    public VerilatorMemBlackBox::VerilatorMemBlackBoxPort
    {

      public:
        NonCombMemBlackBoxPort(const std::string& name,
                    NonCombMemBlackBox *owner) :
                    VerilatorMemBlackBoxPort(name, owner)
                { }

        void sendTimingPacket(PacketPtr pkt);

        //currently need this for synchronization. tells gem5 to access mem
        //model immediatly instead of scheduling event to do so
        bool sendAtomicPacket(PacketPtr pkt);

      protected:
        bool recvTimingResp(PacketPtr pkt) override;

        void recvReqRetry() override;
    };
    //deal with data going back to verilator c++
    bool handleResponse( PacketPtr pkt );

    //memory ports for imem and dmem requests
    NonCombMemBlackBoxPort instPort;
    NonCombMemBlackBoxPort dataPort;
    //pointer for dpi
    static NonCombMemBlackBox * singleton;

    //data for response to dpi
    uint32_t dmemResp;
    uint32_t imemResp;

};
#endif
