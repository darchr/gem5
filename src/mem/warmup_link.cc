/*
 * Copyright (c) 2017 Jason Lowe-Power
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
 *
 * Authors: Jason Lowe-Power
 */

#include "mem/warmup_link.hh"

#include "debug/WarmupLink.hh"

WarmupLink::WarmupLink(WarmupLinkParams *params) :
    MemObject(params),
    cpuSidePort(params->name + ".slave", this),
    blocked(false)
{
    for (int i = 0; i < params->port_master_connection_count; ++i) {
        memPorts.emplace_back(name() + csprintf(".master[%d]", i), i, this);
    }
}

BaseMasterPort&
WarmupLink::getMasterPort(const std::string& if_name, PortID idx)
{
    if (if_name == "master") {
        assert(idx < memPorts.size());
        return memPorts[idx];
    } else {
        return MemObject::getMasterPort(if_name, idx);
    }
}

BaseSlavePort&
WarmupLink::getSlavePort(const std::string& if_name, PortID idx)
{
    panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

    if (if_name == "slave") {
        return cpuSidePort;
    } else {
        return MemObject::getSlavePort(if_name, idx);
    }
}

void
WarmupLink::CPUSidePort::sendPacket(PacketPtr pkt)
{
    // Note: This flow control is very simple since the memobj is blocking.

    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    // If we can't send the packet across the port, store it for later.
    if (!sendTimingResp(pkt)) {
        blockedPacket = pkt;
    }
}

AddrRangeList
WarmupLink::CPUSidePort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

void
WarmupLink::CPUSidePort::trySendRetry()
{
    if (needRetry && blockedPacket == nullptr) {
        // Only send a retry if the port is now completely free
        needRetry = false;
        DPRINTF(WarmupLink, "Sending retry req for %d\n", id);
        sendRetryReq();
    }
}

void
WarmupLink::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    // Just forward to the memobj.
    return owner->handleFunctional(pkt);
}

Tick
WarmupLink::CPUSidePort::recvAtomic(PacketPtr pkt)
{
    return owner->handleAtomic(pkt);
}

bool
WarmupLink::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    // Just forward to the memobj.
    if (!owner->handleRequest(pkt)) {
        needRetry = true;
        return false;
    } else {
        return true;
    }
}

void
WarmupLink::CPUSidePort::recvRespRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);

    // Grab the blocked packet.
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    // Try to resend it. It's possible that it fails again.
    sendPacket(pkt);
}

void
WarmupLink::MemSidePort::sendPacket(PacketPtr pkt)
{
    // Note: This flow control is very simple since the memobj is blocking.

    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

bool
WarmupLink::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    // Just forward to the memobj.
    return owner->handleResponse(pkt, idx);
}

void
WarmupLink::MemSidePort::recvReqRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);

    // Grab the blocked packet.
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    // Try to resend it. It's possible that it fails again.
    sendPacket(pkt);
}

void
WarmupLink::MemSidePort::recvRangeChange()
{
    owner->sendRangeChange();
}

bool
WarmupLink::handleRequest(PacketPtr pkt)
{
    if (blocked) {
        // There is currently an outstanding request. Stall.
        return false;
    }

    DPRINTF(WarmupLink, "Got request for addr %#x\n", pkt->getAddr());

    // This memobj is now blocked waiting for the response to this packet.
    blocked = true;

    // Simply forward to the memory port
    memPorts[0].sendPacket(pkt);

    return true;
}

bool
WarmupLink::handleResponse(PacketPtr pkt, int port_idx)
{
    assert(blocked);
    DPRINTF(WarmupLink, "Got response for addr %#x\n", pkt->getAddr());

    // The packet is now done. We're about to put it in the port, no need for
    // this object to continue to stall.
    // We need to free the resource before sending the packet in case the CPU
    // tries to send another request immediately (e.g., in the same callchain).
    blocked = false;

    cpuSidePort.sendPacket(pkt);

    // For each of the cpu ports, if it needs to send a retry, it should do it
    // now since this memory object may be unblocked now.
    cpuSidePort.trySendRetry();

    return true;
}

void
WarmupLink::handleFunctional(PacketPtr pkt)
{
    // Functionally update all of the masters
    PacketPtr pkt_copy = new Packet(pkt, false, true);
    for (auto& port : memPorts) {
        if (pkt->isResponse()) {
            // "pkt" has already been sent once. From now on, send a copy.
            PacketPtr tmp_pkt = new Packet(pkt_copy, false, true);
            port.sendFunctional(tmp_pkt);
            delete tmp_pkt;
        } else {
            port.sendFunctional(pkt);
        }
    }
    delete pkt_copy;
}

Tick
WarmupLink::handleAtomic(PacketPtr pkt)
{
    // Functionally update all of the masters
    PacketPtr pkt_copy = new Packet(pkt, false, true);
    Tick time = 0;
    for (auto& port : memPorts) {
        if (pkt->isResponse()) {
            // "pkt" has already been sent once. From now on, send a copy.
            PacketPtr tmp_pkt = new Packet(pkt_copy, false, true);
            time = port.sendAtomic(tmp_pkt);
            delete tmp_pkt;
        } else {
            time = port.sendAtomic(pkt);
        }
    }
    delete pkt_copy;
    // Return the time for the last packet. It doesn't really matter.
    return time;
}

AddrRangeList
WarmupLink::getAddrRanges() const
{
    DPRINTF(WarmupLink, "Sending new ranges\n");

    // Make sure all of the ports' address ranges are the same
    AddrRangeList seen_list;
    for (auto& port : memPorts) {
        AddrRangeList list = port.getAddrRanges();
        if (seen_list.empty()) {
            seen_list = list;
        } else {
            // Check to make sure each list matches (order could be different)
            int found = 0;
            for (auto seen_range : seen_list) {
                for (auto range : list) {
                    if (seen_range == range) {
                        found++;
                        break;
                    }
                }
            }
            panic_if(found != seen_list.size(),
                     "The master ports do not have the same address ranges!");
        }
    }

    return seen_list;
}

void
WarmupLink::sendRangeChange()
{
    cpuSidePort.sendRangeChange();
}



WarmupLink*
WarmupLinkParams::create()
{
    return new WarmupLink(this);
}
