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
    memPort(name() + ".mem_port", this),
    nextPendingReadEvent([this] { processNextPendingReadEvent(); }, name())
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

AddrRangeList
WLDirectory::RespPort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

bool
WLDirectory::RespPort::recvTimingReq(PacketPtr pkt)
{
    return owner->handleMemReq(pkt, _id);

}

Tick
WLDirectory::RespPort::recvAtomic(PacketPtr)
{
    panic("recvAtomic called!");
}

void
WLDirectory::RespPort::recvFunctional(PacketPtr pkt)
{
    owner->recvFunctional(pkt);
}

void
WLDirectory::RespPort::recvRespRetry()
{
    panic("recvRespRetry called!");
}

void
WLDirectory::MemPort::sendPacket(PacketPtr pkt)
{
    panic_if(_blocked, "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt))
    {
        blockedPacket = pkt;
        _blocked = true;
    }
}

bool
WLDirectory::MemPort::recvTimingResp(PacketPtr pkt)
{
    return owner->handleMemResp(pkt);

}

void
WLDirectory::MemPort::recvReqRetry()
{
    panic_if(!(_blocked && blockedPacket), "Received retry without a blockedPacket");

    _blocked = false;
    sendPacket(blockedPacket);

    if (!blocked()) {
        blockedPacket = nullptr;
    }
}


AddrRangeList
WLDirectory::getAddrRanges()
{
    return memPort.getAddrRanges();
}

void
WLDirectory::recvFunctional(PacketPtr pkt)
{
    memPort.sendFunctional(pkt);
}

bool
WLDirectory::handleMemReq(PacketPtr pkt, PortID port_id)
{
    // TODO: Add the functionality to send retries.
    Addr addr = pkt->getAddr();
    if (pkt->isRead()) {
        // if (addrResidenceMap.find(addr) == addrResidenceMap.end()) {
        //     if (!memPort.blocked()) {
        //         addrResidenceMap[addr] = port_id;
        //         routeBackMap[pkt->req] = port_id;
        //         pendingR
        //         return true;
        //     } else {
        //         return false;
        //     }
        // } else {
        pendingReads.push_back(std::make_pair(pkt, port_id));
        if (!nextPendingReadEvent.scheduled() && !pendingReads.empty()) {
            schedule(nextPendingReadEvent, nextCycle());
        }
        return true;
        // }
    } else if (pkt->isWrite()) {
        if (addrResidenceMap.find(addr) == addrResidenceMap.end()) {
            panic("Should not write an address that has not be acquired.");
        } else {
            if (!memPort.blocked()) {
                addrResidenceMap.erase(addr);
                routeBackMap[pkt->req] = port_id;
                memPort.sendPacket(pkt);
                return true;
            } else {
                return false;
            }
        }
    } else {
        if (!memPort.blocked()) {
            memPort.sendPacket(pkt);
            return true;
        } else {
            return false;
        }
    }
}

bool
WLDirectory::handleMemResp(PacketPtr pkt) {
    if (pkt->isWrite()) {
        routeBackMap.erase(pkt->req);
        return true;
    }
    PortID dest_port_id = routeBackMap[pkt->req];
    if (worklistPort.Id() == dest_port_id) {
        routeBackMap.erase(pkt->req);
        worklistPort.sendTimingResp(pkt);
    } else if (applyPort.Id() == dest_port_id) {
        routeBackMap.erase(pkt->req);
        applyPort.sendTimingResp(pkt);
    } else {
        panic("Received a response for a port other than wl/apply!");
    }
    return true;
}

void
WLDirectory::processNextPendingReadEvent()
{
    for (std::deque<std::pair<PacketPtr, PortID>>::iterator
        it = pendingReads.begin(); it != pendingReads.end(); it++) {
        PacketPtr pkt = it->first;
        Addr addr = pkt->getAddr();
        if ((addrResidenceMap.find(addr) == addrResidenceMap.end()) &&
            (!memPort.blocked())) {
            PacketPtr read_req = it->first;
            PortID new_port_id = it->second;
            addrResidenceMap[addr] = new_port_id;
            memPort.sendPacket(read_req);
            it = pendingReads.erase(it);
        }
    }

    if (!nextPendingReadEvent.scheduled() && !pendingReads.empty()) {
        schedule(nextPendingReadEvent, nextCycle());
    }

}

}