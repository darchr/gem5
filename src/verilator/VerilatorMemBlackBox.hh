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

#define VM_TRACE 0
#define VL_THREADED 0

#include "VTop_DualPortedMemoryBlackBox.h"
#include "mem/mem_object.hh"
#include "params/VerilatorMemBlackBox.hh"

class VerilatorMemBlackBox: public MemObject
{
    public:
        VTop_DualPortedMemoryBlackBox* blkbox;

        void doFetch();
        void doMem();

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

                void sendPacket(PacketPtr pkt);

            protected:
                bool recvTimingResp(PacketPtr pkt) override;

                void recvReqRetry() override;
        };

        VerilatorMemBlackBoxPort instPort;
        VerilatorMemBlackBoxPort dataPort;


        VerilatorMemBlackBox( VerilatorMemBlackBoxParams *p );
        ~VerilatorMemBlackBox();
    private:
        bool handleResponse( PacketPtr pkt );
        BaseMasterPort& getMasterPort( const std::string& if_name,
                PortID idx );


};
#endif
