/*
 * Copyright (c) 2020 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "accl/graph/sega/wl_directory.hh"

namespace gem5
{
WLDirectory::WLDirectory(const WLDirectoryParams &params) :
    ClockedObject(params),
    worklistPort(name() + ".worklist_port", this, 0),
    applyPort(name() + ".apply_port", this, 1),
    memPort(name() + ".mem_port", this)
{}

Port&
WLDirectory::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "worklist_port") {
        return worklistPort;
    } else if (if_name == "apply_port") {
        return applyPort;
    } else if (if_name == "mem_port") {
        return memPort;
    } else {
        return SimObject::getPort(if_name, idx);
    }
}



        virtual Tick recvAtomic(PacketPtr pkt);
        virtual void recvFunctional(PacketPtr pkt);
        virtual void recvRespRetry();

AddrRangeList
WLDirectory::RespPort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

bool
WLDirectory::RespPort::recvTimingReq(PacketPtr pkt)
{
    return owner->handleRequest(PacketPtr pkt);

}

Tick
WLDirectory
}