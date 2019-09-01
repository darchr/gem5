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

#ifndef __VERILATOR_NVDLA_WRAPPER__HH__
#define __VERILATOR_NVDLA_WRAPPER__HH__

#include "../driven_object.hh"
#include "./bare_nvdla/system.hh"
#include "./system.hh"
#include "axi_responder.hh"
#include "axi_to_mem.hh"
#include "csb_master.hh"
#include "nvdla/VNV_nvdla.h"
#include "params/NVDLAWrapper.hh"
#include "trace_loader.hh"

class NVDLAWrapper : public DrivenObject{
    public:
        NVDLAWrapper(NVDLAWrapperParams *p);

        void updateCycle() override;
        void startup() override;
    private:
        void initNVDLA();
        void runNVDLA();
        void evalTrace();
        void resetNVDLA();
        void initClearDLABuffers();

    private:
        //event queue var to schedule mem requests and cycle updates
        EventFunctionWrapper event;

        int bufferClearCycles;
        int waiting;
        int quiesc_timer;

        BareNVDLASystem * system;

        VNV_nvdla * dla;
        CSBMaster * csb;
        TraceLoader * tloader;
        AXIResponder<uint64_t> * axi_dbb;
        AXIResponder<uint64_t> * axi_cvsram;

};

#endif
