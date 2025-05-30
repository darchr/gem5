// Copyright (c) 2025 The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met: redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer;
// redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution;
// neither the name of the copyright holders nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "new/dual_port.hh"

#include "base/trace.hh"
#include "debug/DualPort.hh"

namespace gem5
{

DualPort::DualPort(const DualPortParams &params) :
    SimObject(params),
    cpuSidePort(params.name + ".cpu_side_port", this),
    memSidePort(params.name + ".mem_side_port", this),
    blocked(false)
{
}

Port &
DualPort::getPort(const std::string &if_name, PortID idx)
{
    panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

    // This is the name from the Python SimObject declaration (DualPort.py)
    if (if_name == "mem_side_port") {
        return memSidePort;
    } else if (if_name == "cpu_side_port") {
        return cpuSidePort;
    } else {
        // pass it along to our super class
        return SimObject::getPort(if_name, idx);
    }
}

void
DualPort::CPUSidePort::sendPacket(PacketPtr pkt)
{
    // Note: This flow control is very simple since the memobj is blocking.

    panic_if(blockedPackets.size() > 0, "Should never try to send if blocked!");

    // If we can't send the packet across the port, store it for later.
    if (!sendTimingResp(pkt)) {
        // make sure to push this packet to the queue.
        blockedPackets.push(pkt);
    }
}

AddrRangeList
DualPort::CPUSidePort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

void
DualPort::CPUSidePort::trySendRetry()
{
    if (needRetry && blockedPackets.size() == 0) { //  == nullptr) {
        // Only send a retry if the port is now completely free
        needRetry = false;
        DPRINTF(DualPort, "Sending retry req for %d\n", id);
        sendRetryReq();
    }
}

void
DualPort::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    // Just forward to the memobj.
    return owner->handleFunctional(pkt);
}

bool
DualPort::CPUSidePort::recvTimingReq(PacketPtr pkt)
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
DualPort::CPUSidePort::recvRespRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPackets.size() > 0);

    // Grab the blocked packet.
    PacketPtr pkt = blockedPackets.front();
    // make sure to pop this packet
    blockedPackets.pop();
    // blockedPacket = nullptr;

    // Try to resend it. It's possible that it fails again.
    sendPacket(pkt);
}

void
DualPort::MemSidePort::sendPacket(PacketPtr pkt)
{
    // Note: This flow control is very simple since the memobj is blocking.

    panic_if(blockedPackets.size() != 0, "Should never try to send if blocked!");

    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        // This gets appended into the queue.
        blockedPackets.push(pkt);
    }
}

bool
DualPort::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    // Just forward to the memobj.
    return owner->handleResponse(pkt);
}

void
DualPort::MemSidePort::recvReqRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPackets.size() > 0); // != nullptr);

    // Grab the blocked packet.
    while(isBlocked() && sendPacket(blockedPackets.front()))
        blockedPackets.pop();

    // PacketPtr pkt = blockedPackets.front();
    // // Make sure that the queue is popped immediately!
    // blockedPackets.pop();
    // // blockedPacket = nullptr;

    // // Try to resend it. It's possible that it fails again.
    // sendPacket(pkt);
}
bool
DualPort::MemSidePort::isBlocked() {
    return !blockedPackets.empty();
}
void
DualPort::MemSidePort::recvRangeChange()
{
    owner->sendRangeChange();
}

bool
DualPort::handleRequest(PacketPtr pkt)
{
    if (blocked) {
        // There is currently an outstanding request. Stall.
        return false;
    }

    DPRINTF(DualPort, "Got request for addr %#x\n", pkt->getAddr());

    // This memobj is now blocked waiting for the response to this packet.
    blocked = true;

    // Simply forward to the memory port
    memSidePort.sendPacket(pkt);

    return true;
}

bool
DualPort::handleResponse(PacketPtr pkt)
{
    assert(blocked);
    DPRINTF(DualPort, "Got response for addr %#x\n", pkt->getAddr());

    // The packet is now done. We're about to put it in the port, no need for
    // this object to continue to stall.
    // We need to free the resource before sending the packet in case the CPU
    // tries to send another request immediately (e.g., in the same callchain).
    blocked = false;

    // Simply forward to the memory port
    // Doesn't matter if this is a instruction or a data request
    cpuSidePort.sendPacket(pkt);
    cpuSidePort.trySendRetry();
    // if (pkt->req->isInstFetch()) {
    //     instPort.sendPacket(pkt);
    // } else {
    //     dataPort.sendPacket(pkt);
    // }

    // // For each of the cpu ports, if it needs to send a retry, it should do it
    // // now since this memory object may be unblocked now.
    // instPort.trySendRetry();
    // dataPort.trySendRetry();

    return true;
}

void
DualPort::handleFunctional(PacketPtr pkt)
{
    // Just pass this on to the memory side to handle for now.
    memSidePort.sendFunctional(pkt);
}

AddrRangeList
DualPort::getAddrRanges() const
{
    DPRINTF(DualPort, "Sending new ranges\n");
    // Just use the same ranges as whatever is on the memory side.
    return memSidePort.getAddrRanges();
}

void
DualPort::sendRangeChange()
{
    cpuSidePort.sendRangeChange();
}

} // namespace gem5
