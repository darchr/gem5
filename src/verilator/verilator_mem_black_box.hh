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
#ifndef __VERILATOR_VERILATOR_MEM_BLACK_BOX__HH__
#define __VERILATOR_VERILATOR_MEM_BLACK_BOX__HH__

//We don't want verilator to use its trace features or multi thread
//I changed sconscript to deal with this REMOVE
#define VM_TRACE 0
#define VL_THREADED 0

//verilator includes
#include "VTop_DualPortedMemoryBlackBox.h"
//gem5 general includes
#include "mem/mem_object.hh"
//gem5 design includes
#include "params/VerilatorMemBlackBox.hh"

class VerilatorMemBlackBox: public MemObject
{
  public:
    //pointer to actual verilator blackbox c++
    VTop_DualPortedMemoryBlackBox* blkbox;

    //memory access functions for blackbox
    void doFetch();
    void doMem();

    BaseMasterPort& getMasterPort( const std::string& if_name,
                PortID idx = InvalidPortID ) override;

    //param setup for blackbox warpper
    VerilatorMemBlackBox( VerilatorMemBlackBoxParams *p );

  private:
    //master port for blackbox
    class VerilatorMemBlackBoxPort : public MasterPort
    {
      private:
        VerilatorMemBlackBox *owner;
        PacketPtr blockedPacket;

      public:
        VerilatorMemBlackBoxPort(const std::string& name,
                    VerilatorMemBlackBox *owner) :
                    MasterPort(name, owner),
                    owner(owner),
                    blockedPacket(nullptr)
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
    VerilatorMemBlackBoxPort instPort;
    VerilatorMemBlackBoxPort dataPort;

};
#endif
