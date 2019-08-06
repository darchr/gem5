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

//gem5 general includes
#include "mem/mem_object.hh"
#include "sim/clocked_object.hh"

//gem5 model includes
#include "params/VerilatorMemBlackBox.hh"

//Interface for different types of memory models
class VerilatorMemBlackBox: public MemObject
{
  public:
    //memory access functions for blackbox
    //virtual void doFetch(){}
    //virtual void doMem(){}

    //virtual uint32_t getDmemResp() = 0;
    //virtual uint32_t getImemResp() = 0;

    VerilatorMemBlackBox( VerilatorMemBlackBoxParams *params) :
    MemObject(params)
    {}

  protected:
  //master port for blackbox
    class VerilatorMemBlackBoxPort : public MasterPort
    {
      protected:
        VerilatorMemBlackBox *owner;
        PacketPtr blockedPacket;

      public:

         VerilatorMemBlackBoxPort(const std::string& name,
                    VerilatorMemBlackBox *owner) :
                    MasterPort(name, owner),
                    owner(owner),
                    blockedPacket(nullptr)
                { }

        virtual void sendTimingPacket(){}

        //currently need this for synchronization. tells gem5 to access mem
        //model immediatly instead of scheduling event to do so
        virtual void sendAtomicPacket(){}
    };
    //deal with data going back to verilator c++
   virtual void handleResponse(){}
};
#endif
