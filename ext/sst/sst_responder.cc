// Copyright (c) 2021 The Regents of the University of California
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

#include "sst_responder.hh"

#include <cassert>
#include <iostream>

#include "translator.hh"

SSTResponder::SSTResponder(SSTResponderSubComponent* owner_)
    : gem5::SSTResponderInterface()
{
    owner = owner_;
}

SSTResponder::~SSTResponder()
{
}

void
SSTResponder::setOutputStream(SST::Output* output_)
{
    output = output_;
}

gem5::Tick
SSTResponder::handleRecvAtomicReq(gem5::PacketPtr pkt)
{
    bool is_read = false;
    bool is_write = false;
    switch ((gem5::MemCmd::Command)pkt->cmd.toInt()) {
        //case gem5::MemCmd::HardPFReq:
        //case gem5::MemCmd::SoftPFReq:
        //case gem5::MemCmd::SoftPFExReq:
        //case gem5::MemCmd::LoadLockedReq:
        case gem5::MemCmd::ReadExReq:
        case gem5::MemCmd::ReadReq:
        //case gem5::MemCmd::SwapReq:
        case gem5::MemCmd::ReadSharedReq:
            is_read = true;
            break;
        //case gem5::MemCmd::StoreCondReq:
        case gem5::MemCmd::WriteReq:
        case gem5::MemCmd::WritebackDirty:
            is_write = true;
            break;
        default:
            panic("Unable to parser gem5 atomic access: %s\n", pkt->cmd.toString());
    }

    gem5::Addr pkt_addr = pkt->getAddr(); //pkt->req->getPaddr();
    size_t pkt_size = pkt->getSize();

    if (atomicAccessReservoir.find(pkt_addr) == atomicAccessReservoir.end())
        atomicAccessReservoir[pkt_addr] = std::move(std::vector<uint8_t>(64, 0)); // assuming cache_line_size of 64 bytes

    bool storeConditionalSuccessful = false;

    if (pkt->isRead())
    {
        if (pkt->isLLSC())
        {
            assert(!pkt->fromCache());
            //trackLoadLocked(pkt);
            // reserve the bytes
            loadLockLists.insert(pkt_addr);
            std::cout << "LLSC: Reserving " << std::hex << "0x" << pkt_addr << " (" << pkt->req->getPaddr() << ")" << std::dec << std::endl;
        }
        pkt->setData(atomicAccessReservoir[pkt_addr].data());
    }
    else if (pkt->isWrite())
    {
        bool allowStore = false;
        if (pkt->isLLSC())
        {
            std::cout << "LLSC: Try store " << std::hex << "0x" << pkt_addr << " (" << pkt->req->getPaddr() << ")" << std::dec << ": ";
            if (loadLockLists.find(pkt_addr) != loadLockLists.end())
            {
                storeConditionalSuccessful = true;
                allowStore = true;
                loadLockLists.erase(pkt_addr);
            }
            // if there was no reservation for the address, this is a SC failure.
            if (storeConditionalSuccessful)
                std::cout << "successful" << std::endl;
            else
                std::cout << "failed" << std::endl;
        }
        else
        {
            allowStore = true;
        }

        if (allowStore)
            pkt->writeData(atomicAccessReservoir[pkt_addr].data());
    }
    else
    {
        std::cout << "Uninteresting packet: " << pkt_addr << std::endl;
    }

    /*
    if (is_write)
    {
        uint8_t* data_ptr = pkt->getPtr<uint8_t>();
        if (pkt->isMaskedWrite())
        {
            for (size_t offset = 0; offset < pkt_size; offset++)
                if (pkt->req->getByteEnable()[offset])
                {
                    std::cout << pkt_addr << "::" << offset <<std::endl;
                    atomicAccessReservoir[pkt_addr][offset] = data_ptr[offset];
                }
        }
        else
        {
            std::vector<uint8_t> data = std::vector<uint8_t>(data_ptr, data_ptr + pkt_size);
            atomicAccessReservoir[pkt_addr] = std::move(data);
        }
    }
    */

    /*
    if (pkt->isWrite())
        std::cout << "WRITE to " ;
    if (pkt->isRead())
        std::cout << "READ from ";
    if (pkt->isWrite() || pkt->isRead())
    {
        std::cout << std::hex << pkt_addr << " (" << pkt->getAddr() << ")" << std::dec << std::endl;
        for (size_t t = 0; t < pkt_size; t++)
            std::cout << std::hex << (uint64_t)atomicAccessReservoir[pkt_addr][t] << " " << std::dec;
        std::cout << std::endl;
    }
    */
    // Response
    if (pkt->needsResponse())
        pkt->makeResponse();
    // Resolve the success of Store Conditionals
    if (pkt->isLLSC() && pkt->isWrite())
    {
        // SC interprets ExtraData == 1 as the store was successful
        pkt->req->setExtraData(storeConditionalSuccessful ? 1 : 0);
    }
    //pkt->setData(atomicAccessReservoir[pkt_addr].data());
    // Clear out bus delay notifications
    pkt->headerDelay = pkt->payloadDelay = 0;

    return 1000; // some fixed latency
}

bool
SSTResponder::handleRecvTimingReq(gem5::PacketPtr pkt)
{
    auto request = Translator::gem5RequestToSSTRequest(
        pkt, owner->sstRequestIdToPacketMap
    );
    std::cout << "Received Timing Request at address: 0x" << std::hex << pkt->getAddr() << std::dec << std::endl;
    return owner->handleTimingReq(request);
}

void
SSTResponder::handleRecvRespRetry()
{
    owner->handleRecvRespRetry();
}

void
SSTResponder::handleRecvFunctional(gem5::PacketPtr pkt)
{
    //owner->handleRecvFunctional(pkt);
    std::cout << "Functional access to " << pkt->getAddr() << std::endl;
    this->handleRecvAtomicReq(pkt);
}
