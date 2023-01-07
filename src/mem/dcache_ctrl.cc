/// The copyright needs be modified for UCD/DArchR/the names of the writers
/*
 * Copyright (c) 2010-2020 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2013 Amin Farmahini-Farahani
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

#include "mem/dcache_ctrl.hh"

#include "base/trace.hh"
#include "debug/DRAM.hh"
#include "debug/DcacheCtrl.hh"
#include "debug/Drain.hh"
#include "debug/NVM.hh"
#include "debug/QOS.hh"
#include "mem/mem_interface.hh"
#include "sim/system.hh"

DcacheCtrl::DcacheCtrl(const DcacheCtrlParams &p) :
    QoS::MemCtrl(p),
    port(name() + ".port", *this), isTimingMode(false),
    retry(false),
    nextReqEvent([this]{ processNextReqEvent(); }, name()),
    respondEvent([this]{ processRespondEvent(); }, name()),
    dramReadEvent([this]{ processDramReadEvent(); }, name()),
    respDramReadEvent([this]{ processRespDramReadEvent(); }, name()),
    waitingToIssueNvmReadEvent([this]
    { processWaitingToIssueNvmReadEvent(); }, name()),
    nvmReadEvent([this]{ processNvmReadEvent(); }, name()),
    respNvmReadEvent([this]{ processRespNvmReadEvent(); }, name()),
    overallWriteEvent([this]{ processOverallWriteEvent(); }, name()),
    dram(p.dram), nvm(p.nvm),
    dramCacheSize(p.dram_cache_size),
    blockSize(p.block_size),
    addrSize(p.addr_size),
    orbMaxSize(p.orb_max_size), orbSize(0),
    crbMaxSize(p.crb_max_size), crbSize(0),
    writeHighThreshold(p.write_high_thresh_perc * p.orb_max_size / 100.0),
    writeLowThreshold(p.write_low_thresh_perc* p.orb_max_size / 100.0),
    minWritesPerSwitch(p.min_writes_per_switch),
    memSchedPolicy(p.mem_sched_policy),
    frontendLatency(p.static_frontend_latency),
    backendLatency(p.static_backend_latency),
    commandWindow(p.command_window),
    nextBurstAt(0), prevArrival(0),
    nextReqTime(0),
    stats(*this)
{
    DPRINTF(DcacheCtrl, "Setting up controller\n");

    pktDramRead.resize(1);
    pktNvmReadWaitIssue.resize(1);
    pktNvmRead.resize(1);
    pktDramWrite.resize(1);
    pktNvmWrite.resize(1);

    stallRds = false;
    drainDramWrite = false;
    drainNvmWrite = false;

    if (orbMaxSize>512) {
        dramWrDrainPerc = 0.25;
    }
    else {
        dramWrDrainPerc = 0.5;
    }
    // NVM Write Drain is defined by the write queue size
    // defined by the NVM interface

    if (orbMaxSize == 1) {
        writeHighThreshold = 1;
    }

    // if (orbMaxSize == 1) {
    //     minWritesPerSwitch = 2;
    //     minDrWrPerSwitch = 1;
    //     minNvWrPerSwitch = 1;
    // }
    // else {
    //     minWritesPerSwitch = orbMaxSize * 0.2;
    //     minDrWrPerSwitch = 0.7 * minWritesPerSwitch;
    //     minNvWrPerSwitch = minWritesPerSwitch - minDrWrPerSwitch;
    // }

    minDrWrPerSwitch = 0.7 * minWritesPerSwitch;
    minNvWrPerSwitch = minWritesPerSwitch - minDrWrPerSwitch;

    drWrCounter = 0;
    nvWrCounter = 0;

    // dramCacheSize = dram->dramDeviceCapacity;
    // dramCacheSize = dramCacheSize*1024*1024;

    tagMetadataStore.resize(dramCacheSize/blockSize);

    // Hook up interfaces to the controller
    if (dram)
        dram->setCtrl(this, commandWindow);
    if (nvm)
        nvm->setCtrl(this, commandWindow);

    fatal_if(!dram && !nvm, "Memory controller must have an interface");

    // perform a basic check of the write thresholds
    if (p.write_low_thresh_perc >= p.write_high_thresh_perc)
        fatal("Write buffer low threshold %d must be smaller than the "
              "high threshold %d\n", p.write_low_thresh_perc,
              p.write_high_thresh_perc);
}

void
DcacheCtrl::init()
{
   if (!port.isConnected()) {
        fatal("DcacheCtrl %s is unconnected!\n", name());
    } else {
        port.sendRangeChange();
    }
}

void
DcacheCtrl::startup()
{
    // remember the memory system mode of operation
    isTimingMode = system()->isTimingMode();

    if (isTimingMode) {
        // shift the bus busy time sufficiently far ahead that we never
        // have to worry about negative values when computing the time for
        // the next request, this will add an insignificant bubble at the
        // start of simulation
        nextBurstAt = curTick() + (dram ? dram->commandOffset() :
                                          nvm->commandOffset());
    }
}

Tick
DcacheCtrl::recvAtomic(PacketPtr pkt)
{
    DPRINTF(DcacheCtrl, "recvAtomic: %s 0x%x\n",
                     pkt->cmdString(), pkt->getAddr());

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    Tick latency = 0;
    // do the actual memory access and turn the packet into a response
    if (dram && dram->getAddrRange().contains(pkt->getAddr())) {
        dram->access(pkt);

        if (pkt->hasData()) {
            // this value is not supposed to be accurate, just enough to
            // keep things going, mimic a closed page
            latency = dram->accessLatency();
        }
    } else if (nvm && nvm->getAddrRange().contains(pkt->getAddr())) {
        nvm->access(pkt);

        if (pkt->hasData()) {
            // this value is not supposed to be accurate, just enough to
            // keep things going, mimic a closed page
            latency = nvm->accessLatency();
        }
    } else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
    }

    return latency;
}

Tick
DcacheCtrl::recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &backdoor)
{
    Tick latency = recvAtomic(pkt);
    if (dram) {
        dram->getBackdoor(backdoor);
    } else if (nvm) {
        nvm->getBackdoor(backdoor);
    }
    return latency;
}

Addr
DcacheCtrl::returnTagDC(Addr request_addr, unsigned size)
{
    int index_bits = ceilLog2(dramCacheSize/blockSize);
    int block_bits = ceilLog2(size);
    return bits(request_addr, addrSize-1, (index_bits+block_bits));
}

Addr
DcacheCtrl::returnIndexDC(Addr request_addr, unsigned size)
{
    return bits(request_addr, ceilLog2(size) +
            ceilLog2(dramCacheSize/blockSize)-1, ceilLog2(size));
}

void
DcacheCtrl::checkHitOrMiss(reqBufferEntry* orbEntry)
{
    // access the tagMetadataStore data structure to
    // check if it's hit or miss
    orbEntry->isHit =
    tagMetadataStore.at(orbEntry->indexDC).validLine &&
    (orbEntry->tagDC == tagMetadataStore.at(orbEntry->indexDC).tagDC);

    if (!tagMetadataStore.at(orbEntry->indexDC).validLine &&
        !orbEntry->isHit) {
        stats.numColdMisses++;
    }
    else if (tagMetadataStore.at(orbEntry->indexDC).validLine &&
             !orbEntry->isHit) {
        stats.numHotMisses++;
    }

    // always hit
    // orbEntry->isHit = true;

    // always miss
    // orbEntry->isHit = false;
}

bool
DcacheCtrl::checkDirty(Addr addr)
{
    Addr index = returnIndexDC(addr, blockSize);
    return (tagMetadataStore.at(index).validLine &&
            tagMetadataStore.at(index).dirtyLine);


    // always dirty
    //return true;

    // always clean
    //return false;
}

void
DcacheCtrl::handleDirtyCacheLine(reqBufferEntry* orbEntry)
{
    assert(orbEntry->dirtyLineAddr != -1);

    MemPacket* wbDccPkt = nvm->decodePacket(nullptr,
                            orbEntry->dirtyLineAddr,
                            orbEntry->owPkt->getSize(),
                            false, false);

    nvm->setupRank(wbDccPkt->rank, false);

    pktNvmWrite[0].push_back(wbDccPkt);

    if (pktNvmWrite[0].size() >= nvm->getMaxPendingWrites()) {
        stallRds = true;
        drainNvmWrite = true;
        if (!overallWriteEvent.scheduled()) {
            schedule(overallWriteEvent, std::max(nextReqTime, curTick()));
        }
    }

    unsigned rdsNum = pktDramRead[0].size() +
                      pktNvmReadWaitIssue[0].size() +
                      pktNvmRead[0].size();
    unsigned wrsNum = pktNvmWrite[0].size() +
                      pktDramWrite[0].size();

    if ((rdsNum == 0 && wrsNum != 0) ||
        (wrsNum >= writeHighThreshold)) {
        stallRds = true;
        if (!overallWriteEvent.scheduled()) {
            schedule(overallWriteEvent, std::max(nextReqTime, curTick()));
        }
    }

    if (pktNvmWrite[0].size() > maxNvWrEv) {
        maxNvWrEv = pktNvmWrite[0].size();
        stats.maxNvWrEvQ = pktNvmWrite[0].size();
    }

    stats.numWrBacks++;

    // no need to call nvm->access for the dirty line.
    // Because, we already have written it in nvm, while
    // we were processing it into dram cache.
}

void
DcacheCtrl::handleRequestorPkt(PacketPtr pkt)
{
    // Set is_read and is_dram to
    // "true", to do initial dram Read
    MemPacket* dcc_pkt = dram->decodePacket(pkt,
                                            pkt->getAddr(),
                                            pkt->getSize(),
                                            true,
                                            true);

    // pass the second argument "true", for
    // initial DRAM Read for all the received packets
    dram->setupRank(dcc_pkt->rank, true);

    reqBufferEntry* entry = new reqBufferEntry(
                                true, curTick(),
                                returnTagDC(pkt->getAddr(), pkt->getSize()),
                                returnIndexDC(pkt->getAddr(), pkt->getSize()),
                                pkt, dcc_pkt,
                                dramRead, false, false,
                                -1, false,
                                curTick(), MaxTick,
                                MaxTick, MaxTick, MaxTick,
                                MaxTick,
                                MaxTick, MaxTick, MaxTick
                            );

    reqBuffer.emplace(pkt->getAddr(), entry);

    if (pkt->isRead()) {
        logRequest(DcacheCtrl::READ, pkt->requestorId(), pkt->qosValue(),
                   pkt->getAddr(), 1);
    }
    else {
        //copying the packet
        PacketPtr copyOwPkt = new Packet(pkt, false, pkt->isRead());

        accessAndRespond(pkt, frontendLatency, false);

        reqBuffer.at(copyOwPkt->getAddr()) =  new reqBufferEntry(
                                entry->validEntry, entry->arrivalTick,
                                entry->tagDC, entry->indexDC,
                                copyOwPkt, entry->dccPkt,
                                entry->state, entry->isHit, entry->conflict,
                                entry->dirtyLineAddr,
                                entry->handleDirtyLine,
                                entry->drRd,
                                entry->drWr,
                                entry->nvWait,
                                entry->nvRd,
                                entry->nvWr,
                                entry->nvmIssueReadyTime,
                                entry->dramRdDevTime,
                                entry->dramWrDevTime,
                                entry->nvmRdDevTime
                          );
        delete entry;

        entry = reqBuffer.at(copyOwPkt->getAddr());

        logRequest(DcacheCtrl::WRITE, copyOwPkt->requestorId(),
                   copyOwPkt->qosValue(),
                   copyOwPkt->getAddr(), 1);
    }

    checkHitOrMiss(entry);

    if (checkDirty(entry->owPkt->getAddr()) && !entry->isHit) {
        entry->dirtyLineAddr = tagMetadataStore.at(entry->indexDC).nvmAddr;
        entry->handleDirtyLine = true;
    }

    // Updating Tag & Metadata
    tagMetadataStore.at(entry->indexDC).tagDC = entry->tagDC;
    tagMetadataStore.at(entry->indexDC).indexDC = entry->indexDC;
    tagMetadataStore.at(entry->indexDC).validLine = true;

    if (entry->owPkt->isRead()) {
        if (entry->isHit) {
            tagMetadataStore.at(entry->indexDC).dirtyLine =
            tagMetadataStore.at(entry->indexDC).dirtyLine;
        }
        else {
            tagMetadataStore.at(entry->indexDC).dirtyLine = false;
        }
    }
    else {
        tagMetadataStore.at(entry->indexDC).dirtyLine = true;
    }

    tagMetadataStore.at(entry->indexDC).nvmAddr =
                        entry->owPkt->getAddr();

    if (entry->owPkt->isRead()) {
        stats.readReqs++;
    }
    else {
        stats.writeReqs++;
    }
}

bool
DcacheCtrl::checkConflictInDramCache(PacketPtr pkt)
{
    unsigned indexDC = returnIndexDC(pkt->getAddr(), pkt->getSize());

    for (auto e = reqBuffer.begin(); e != reqBuffer.end(); ++e) {
        if (indexDC == e->second->indexDC
            && e->second->validEntry
            //&& confReqBuffer.size() < crbMaxSize
            ) {

            e->second->conflict = true;

            return true;
        }
    }

    return false;
}

void
DcacheCtrl::checkConflictInCRB(reqBufferEntry* orbEntry)
{
    for (auto e = confReqBuffer.begin(); e != confReqBuffer.end(); ++e) {

        auto entry = *e;

        if (returnIndexDC(entry.second->getAddr(),entry.second->getSize())
            == orbEntry->indexDC) {
                orbEntry->conflict = true;
                break;
        }
    }
}

void
DcacheCtrl::logStatsDcache(reqBufferEntry* orbEntry)
{
    if (orbEntry->owPkt->isRead()) {
        if (orbEntry->isHit) {
            assert(orbEntry->drRd != MaxTick);
            assert(orbEntry->dramRdDevTime != MaxTick);

            long long int stateTick1 = (curTick() - orbEntry->drRd);
            assert(stateTick1 > 0);
            stats.timeInDramRead += stateTick1;

            stats.totNumPktsDrRd++;
            stats.drRdDevTime += orbEntry->dramRdDevTime;

            long long int stateQT1 = stateTick1 -
                                     (orbEntry->dramRdDevTime);
            assert(stateQT1 >= 0);
            stats.drRdQingTime += stateQT1;

            stats.numHits++;
            stats.numRdHits++;
        }
        else {
            assert(orbEntry->drRd != MaxTick);
            assert(orbEntry->nvWait != MaxTick);
            assert(orbEntry->nvRd != MaxTick);
            assert(orbEntry->drWr != MaxTick);
            assert(orbEntry->nvmIssueReadyTime != MaxTick);
            assert(orbEntry->dramRdDevTime != MaxTick);
            assert(orbEntry->nvmRdDevTime != MaxTick);
            assert(orbEntry->dramWrDevTime != MaxTick);

            long long int  stateTick1 =
                           (orbEntry->nvWait - orbEntry->drRd);
            assert(stateTick1 > 0);
            stats.timeInDramRead += stateTick1;

            stats.totNumPktsDrRd++;
            stats.drRdDevTime += orbEntry->dramRdDevTime;

            long long int  stateQT1 = stateTick1 -
                                      (orbEntry->dramRdDevTime);
            assert(stateQT1 >= 0);
            stats.drRdQingTime += stateQT1;

            long long int  stateTick2 =
            (orbEntry->nvRd - orbEntry->nvWait);
            assert(stateTick2 >= 0);
            stats.timeInWaitingToIssueNvmRead += stateTick2;

            if (stateTick2 > 0) {
                stats.totNumPktsNvmRdWait++;
            }

            long long int  stateTick3 =
                           (orbEntry->drWr - orbEntry->nvRd);
            assert(stateTick3 > 0);
            stats.timeInNvmRead += stateTick3;

            stats.totNumPktsNvmRd++;
            stats.nvRdDevTime += orbEntry->nvmRdDevTime;

            long long int  stateQT2 = stateTick2 + stateTick3 -
                                      (orbEntry->nvmRdDevTime);
            assert(stateQT2 >= 0);
            stats.nvmRdQingTime += stateQT2;

            long long int  stateTick4 =
            (orbEntry->dccPkt->readyTime - curTick());
            assert(stateTick4 > 0);
            stats.timeInDramWrite += stateTick4;

            stats.totNumPktsDrWr++;
            stats.drWrDevTime += orbEntry->dramWrDevTime;

            long long int  stateQT3 = stateTick4 -
                                      (orbEntry->dramWrDevTime);
            assert(stateQT3 >= 0);
            stats.drWrQingTime += stateQT3;

            stats.numMisses++;
            stats.numRdMisses++;
        }
    }
    else {
        if (orbEntry->isHit) {
            assert(orbEntry->drRd != MaxTick);
            assert(orbEntry->drWr != MaxTick);
            assert(orbEntry->dramRdDevTime != MaxTick);
            assert(orbEntry->dramWrDevTime != MaxTick);

            long long int  stateTick1 =
                           (orbEntry->drWr - orbEntry->drRd);
            assert(stateTick1 > 0);
            stats.timeInDramRead += stateTick1;

            stats.totNumPktsDrRd++;
            stats.drRdDevTime += orbEntry->dramRdDevTime;

            long long int  stateQT1 = stateTick1 -
                                      (orbEntry->dramRdDevTime);
            assert(stateQT1 >= 0);
            stats.drRdQingTime += stateQT1;

            long long int  stateTick2 =
            (orbEntry->dccPkt->readyTime - curTick());
            assert(stateTick2 > 0);
            stats.timeInDramWrite += stateTick2;

            stats.totNumPktsDrWr++;
            stats.drWrDevTime += orbEntry->dramWrDevTime;

            long long int  stateQT2 =
                           stateTick2 - (orbEntry->dramWrDevTime);
            assert(stateQT2 >= 0);
            stats.drWrQingTime += stateQT2;

            stats.numHits++;
            stats.numWrHits++;
        }
        else {
            assert(orbEntry->drRd != MaxTick);
            assert(orbEntry->nvWait != MaxTick);
            assert(orbEntry->nvRd != MaxTick);
            assert(orbEntry->drWr != MaxTick);
            assert(orbEntry->nvmIssueReadyTime != MaxTick);
            assert(orbEntry->dramRdDevTime != MaxTick);
            assert(orbEntry->nvmRdDevTime != MaxTick);
            assert(orbEntry->dramWrDevTime != MaxTick);

            long long int  stateTick1 =
                            (orbEntry->nvWait - orbEntry->drRd);
            assert(stateTick1 > 0);
            stats.timeInDramRead += stateTick1;

            stats.totNumPktsDrRd++;
            stats.drRdDevTime += orbEntry->dramRdDevTime;

            long long int  stateQT1 = stateTick1 -
                                      (orbEntry->dramRdDevTime);
            assert(stateQT1 >= 0);
            stats.drRdQingTime += stateQT1;

            long long int  stateTick2 =
            (orbEntry->nvRd - orbEntry->nvWait);

            assert(stateTick2 >= 0);
            stats.timeInWaitingToIssueNvmRead += stateTick2;

            if (stateTick2 > 0) {
                stats.totNumPktsNvmRdWait++;
            }

            long long int  stateTick3 =
                            (orbEntry->drWr - orbEntry->nvRd);
            assert(stateTick3 > 0);
            stats.timeInNvmRead += stateTick3;

            stats.totNumPktsNvmRd++;
            stats.nvRdDevTime += orbEntry->nvmRdDevTime;

            long long int  stateQT2 = stateTick2 + stateTick3 -
                                      (orbEntry->nvmRdDevTime);
            assert(stateQT2 >= 0);
            stats.nvmRdQingTime += stateQT2;

            long long int  stateTick4 =
                (orbEntry->dccPkt->readyTime - curTick());
            assert(stateTick4 > 0);
            stats.timeInDramWrite += stateTick4;

            stats.totNumPktsDrWr++;
            stats.drWrDevTime += orbEntry->dramWrDevTime;

            long long int  stateQT3 = stateTick4 -
                                      (orbEntry->dramWrDevTime);
            assert(stateQT3 >= 0);
            stats.drWrQingTime += stateQT3;

            stats.numMisses++;
            stats.numWrMisses++;
        }
    }
}

bool
DcacheCtrl::resumeConflictingReq(reqBufferEntry* orbEntry)
{
    bool conflictFound = false;

    if (orbEntry->owPkt->isWrite()) {
        isInWriteQueue.erase(orbEntry->owPkt->getAddr());
    }

    logStatsDcache(orbEntry);

    for (auto e = confReqBuffer.begin(); e != confReqBuffer.end(); ++e) {

        auto entry = *e;

        if (returnIndexDC(entry.second->getAddr(), entry.second->getSize())
            == orbEntry->indexDC) {

                conflictFound = true;

                Addr confAddr = entry.second->getAddr();

                reqBuffer.erase(orbEntry->owPkt->getAddr());

                delete orbEntry->owPkt;

                delete orbEntry->dccPkt;

                delete orbEntry;

                handleRequestorPkt(entry.second);

                reqBuffer.at(confAddr)->arrivalTick = entry.first;

                confReqBuffer.erase(e);

                checkConflictInCRB(reqBuffer.at(confAddr));

                if (pktDramRead[0].empty() && !stallRds) {
                    assert(!dramReadEvent.scheduled());
                    schedule(dramReadEvent, std::max(nextReqTime, curTick()));
                } else {
                    assert(dramReadEvent.scheduled() || stallRds);
                }

                pktDramRead[0].push_back(reqBuffer.at(confAddr)->dccPkt);

                if (pktDramRead[0].size() > maxDrRdEv) {
                    maxDrRdEv = pktDramRead[0].size();
                    stats.maxDrRdEvQ = pktDramRead[0].size();
                }

                break;
        }

    }

    if (!conflictFound) {

        reqBuffer.erase(orbEntry->owPkt->getAddr());

        delete orbEntry->owPkt;

        delete orbEntry->dccPkt;

        delete orbEntry;
    }

    return conflictFound;
}

Tick
DcacheCtrl::earliestDirtyLineInDrRdResp()
{
    for (int i=0; i<addrDramRespReady.size(); i++) {
        if (reqBuffer.at(addrDramRespReady.at(i))->handleDirtyLine) {
            return reqBuffer.at(addrDramRespReady.at(i))->dccPkt->readyTime;
        }
    }
    return MaxTick;
}

bool
DcacheCtrl::recvTimingReq(PacketPtr pkt)
{
    // This is where we enter from the outside world

    DPRINTF(DcacheCtrl, "recvTimingReq: request %s addr %lld size %d\n",
            pkt->cmdString(), pkt->getAddr(), pkt->getSize());

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    panic_if(!(pkt->isRead() || pkt->isWrite()),
             "Should only see read and writes at memory controller\n");

    // Calc avg gap between requests
    if (prevArrival != 0) {
        stats.totGap += curTick() - prevArrival;
    }
    prevArrival = curTick();

    // What type of media does this packet access?
    // We set a flag to make sure every single packet
    // checks DRAM first.
    // bool is_dram = true;

    // Validate that pkt's address maps to the dram and nvm
    assert(nvm && nvm->getAddrRange().contains(pkt->getAddr()));
    //assert(dram && dram->getAddrRange().contains(pkt->getAddr()));


    // Find out how many memory packets a pkt translates to
    // If the burst size is equal or larger than the pkt size, then a pkt
    // translates to only one memory packet. Otherwise, a pkt translates to
    // multiple memory packets

    Addr addr = pkt->getAddr();

    unsigned burst_size = dram->bytesPerBurst();

    unsigned size = std::min((addr | (burst_size - 1)) + 1,
                             addr + pkt->getSize()) - addr;

    // process merging for writes
    if (!pkt->isRead()) {
        stats.writePktSize[ceilLog2(size)]++;
        stats.writeBursts++;
        stats.requestorWriteAccesses[pkt->requestorId()]++;

        assert(pkt->getSize() != 0);

        bool merged = isInWriteQueue.find(pkt->getAddr()) !=
            isInWriteQueue.end();

        if (merged) {

            stats.mergedWrBursts++;

            accessAndRespond(pkt, frontendLatency, false);

            return true;
        }
    }

    // process forwarding for reads
    bool foundInORB = false;
    bool foundInCRB = false;
    bool foundInNWB = false;

    if (pkt->isRead()) {
        stats.readPktSize[ceilLog2(size)]++;
        stats.readBursts++;
        stats.requestorReadAccesses[pkt->requestorId()]++;

        assert(pkt->getSize() != 0);

        if (isInWriteQueue.find(pkt->getAddr()) != isInWriteQueue.end()) {

            if (!reqBuffer.empty()) {
                for (const auto& e : reqBuffer) {

                    // check if the read is subsumed in the write queue
                    // packet we are looking at
                    if (e.second->validEntry &&
                        e.second->owPkt->isWrite() &&
                        e.second->owPkt->getAddr() <= addr &&
                        ((addr + size) <=
                        (e.second->owPkt->getAddr() +
                        e.second->owPkt->getSize()))) {

                        foundInORB = true;

                        stats.servicedByWrQ++;

                        stats.bytesReadWrQ += burst_size;

                        break;
                    }
                }
            }

            if (!foundInORB && !confReqBuffer.empty()) {
                for (const auto& e : confReqBuffer) {

                    // check if the read is subsumed in the write queue
                    // packet we are looking at
                    if (e.second->isWrite() &&
                        e.second->getAddr() <= addr &&
                        ((addr + size) <=
                        (e.second->getAddr() + e.second->getSize()))) {

                        foundInCRB = true;

                        stats.servicedByWrQ++;

                        stats.bytesReadWrQ += burst_size;

                        break;
                    }
                }
            }

            if (!foundInORB && !foundInCRB && !pktNvmWrite[0].empty()) {
                for (int i=0; i<pktNvmWrite[0].size(); i++) {
                    // check if the read is subsumed in the write queue
                    // packet we are looking at
                    if (pktNvmWrite[0].at(i)->getAddr() <= addr &&
                        ((addr + size) <=
                        (pktNvmWrite[0].at(i)->getAddr() +
                         pktNvmWrite[0].at(i)->getSize()))) {

                        foundInNWB = true;

                        stats.servicedByWrQ++;

                        stats.bytesReadWrQ += burst_size;

                        break;
                    }
                }
            }
        }

        if (foundInORB || foundInCRB || foundInNWB) {

            accessAndRespond(pkt, frontendLatency, false);

            return true;
        }
    }

    // process conflicting requests
    // calculate dram address: ignored for now (because Dsize=Nsize)
    if (checkConflictInDramCache(pkt)) {

        stats.totNumConf++;

        if (confReqBuffer.size()>=crbMaxSize) {

            stats.totNumConfBufFull++;

            retry = true;

            if (pkt->isRead()) {
                stats.numRdRetry++;
            }
            else {
                stats.numWrRetry++;
            }

            return false;
        }

        confReqBuffer.push_back(std::make_pair(curTick(), pkt));

        if (pkt->isWrite()) {
            isInWriteQueue.insert(pkt->getAddr());
        }

        if (confReqBuffer.size() > maxConf) {
            maxConf = confReqBuffer.size();
            stats.maxNumConf = confReqBuffer.size();
        }

        return true;
    }

    // process cases where ORB is full
    if (reqBuffer.size() >= orbMaxSize) {

        retry = true;

        if (pkt->isRead()) {
            stats.numRdRetry++;
        }
        else {
            stats.numWrRetry++;
        }

        return false;
    }

    // if none of the above cases happens,
    // then add the pkt to the outstanding requests buffer

    handleRequestorPkt(pkt);

    if (pkt->isWrite()) {
        isInWriteQueue.insert(pkt->getAddr());
    }

    if (pktDramRead[0].empty() && !stallRds) {

        assert(!dramReadEvent.scheduled());

        schedule(dramReadEvent, std::max(nextReqTime, curTick()));

    } else {
        assert(dramReadEvent.scheduled() || stallRds);
    }

    pktDramRead[0].push_back(reqBuffer.at(pkt->getAddr())->dccPkt);

    if (pktDramRead[0].size() > maxDrRdEv) {
        maxDrRdEv = pktDramRead[0].size();
        stats.maxDrRdEvQ = pktDramRead[0].size();
    }

    return true;
}

void
DcacheCtrl::processDramReadEvent()
{
    if (stallRds) {
        return;
    }

    assert(!pktDramRead[0].empty());

    MemPacketQueue::iterator to_read;

    bool read_found = false;

    bool switched_cmd_type = (busState == DcacheCtrl::WRITE);

    if (switched_cmd_type) {
        stats.wrToRdTurnAround++;
    }

    for (auto queue = pktDramRead.rbegin();
                 queue != pktDramRead.rend(); ++queue) {
        to_read = chooseNext((*queue), switched_cmd_type ?
                                minWriteToReadDataGap() : 0, true);
        if (to_read != queue->end()) {
            // candidate read found
            read_found = true;
            break;
        }
    }

    if (!read_found) {

        schedule(dramReadEvent,
                 std::max(nextReqTime, curTick()+dram->getTBurst()));

        return;
    }

    reqBufferEntry* orbEntry = reqBuffer.at((*to_read)->getAddr());

    // sanity check for the chosen packet
    assert(orbEntry->validEntry);
    assert(orbEntry->dccPkt->isDram());
    assert(orbEntry->dccPkt->isRead());
    assert(orbEntry->state == dramRead);

    if (orbEntry->handleDirtyLine) {
        if (pktNvmWrite[0].size() >= nvm->getMaxPendingWrites()) {
            stallRds = true;
            drainNvmWrite = true;
            if (!overallWriteEvent.scheduled()) {
                    schedule(overallWriteEvent,
                             std::max(nextReqTime, curTick()));
            }
            return;
        }

        if (numDirtyLinesInDrRdRespQ >= nvm->getMaxPendingWrites()) {
            Tick schedTick = earliestDirtyLineInDrRdResp();
            assert(schedTick != MaxTick);
            schedule(dramReadEvent, std::max(nextReqTime, schedTick+1));
            return;
        }

        if (nvm->writeRespQueueFull()) {
            assert(!dramReadEvent.scheduled());
            schedule(dramReadEvent, std::max(nextReqTime,
                                    nvm->writeRespQueueFront()+2));
            return;
        }
    }

    busState = DcacheCtrl::READ;

    assert(packetReady(orbEntry->dccPkt));

    Tick cmd_at = doBurstAccess(orbEntry->dccPkt);

    orbEntry->dramRdDevTime = orbEntry->dccPkt->readyTime - cmd_at;

    // sanity check
    assert(orbEntry->dccPkt->size <= (orbEntry->dccPkt->isDram() ?
                                        dram->bytesPerBurst() :
                                        nvm->bytesPerBurst()));
    assert(orbEntry->dccPkt->readyTime >= curTick());

    if (orbEntry->owPkt->isRead() && orbEntry->isHit) {
        logResponse(DcacheCtrl::READ,
                    orbEntry->dccPkt->requestorId(),
                    orbEntry->dccPkt->qosValue(),
                    orbEntry->owPkt->getAddr(), 1,
                    orbEntry->dccPkt->readyTime - orbEntry->dccPkt->entryTime);
    }

    if (addrDramRespReady.empty()) {
        assert(!respDramReadEvent.scheduled());
        schedule(respDramReadEvent, orbEntry->dccPkt->readyTime);
    }
    else {
        assert(reqBuffer.at(addrDramRespReady.back())->dccPkt->readyTime
                            <= orbEntry->dccPkt->readyTime);

        assert(respDramReadEvent.scheduled());
    }

    addrDramRespReady.push_back(orbEntry->owPkt->getAddr());

    if (addrDramRespReady.size() > maxDrRdRespEv) {
        maxDrRdRespEv = addrDramRespReady.size();
        stats.maxDrRdRespEvQ = addrDramRespReady.size();
    }

    if (orbEntry->handleDirtyLine) {
        numDirtyLinesInDrRdRespQ++;
    }

    //** keep the state as it is, no transition
    orbEntry->state = dramRead;

    pktDramRead[0].erase(to_read);

    if (!pktDramRead[0].empty()) {

        assert(!dramReadEvent.scheduled());

        schedule(dramReadEvent, std::max(nextReqTime, curTick()));
    }

    unsigned rdsNum = pktDramRead[0].size() +
                      pktNvmReadWaitIssue[0].size() +
                      pktNvmRead[0].size();
    unsigned wrsNum = pktNvmWrite[0].size() +
                      pktDramWrite[0].size();

    if ((rdsNum == 0 && wrsNum != 0) ||
        (wrsNum >= writeHighThreshold)) {

        stallRds = true;

        if (!overallWriteEvent.scheduled()) {
            schedule(overallWriteEvent, std::max(nextReqTime, curTick()));
        }
    }
}

void
DcacheCtrl::processRespDramReadEvent()
{
    assert(!addrDramRespReady.empty());

    reqBufferEntry* orbEntry = reqBuffer.at(addrDramRespReady.front());

    // A series of sanity check
    assert(orbEntry->validEntry);
    assert(orbEntry->dccPkt->isDram());
    assert(orbEntry->dccPkt->isRead());
    assert(orbEntry->state == dramRead);
    assert(orbEntry->dccPkt->readyTime == curTick());

    if (orbEntry->handleDirtyLine) {
        handleDirtyCacheLine(orbEntry);
    }

    // A flag which is used for retrying read requests
    // in case one slot in ORB becomes available here
    // (happens only for read hits)
    bool canRetry = false;

    dram->respondEvent(orbEntry->dccPkt->rank);

    // Read Hit
    if (orbEntry->owPkt->isRead() &&
        orbEntry->dccPkt->isDram() &&
        orbEntry->isHit) {
            PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
                                             false,
                                             orbEntry->owPkt->isRead());

            accessAndRespond(orbEntry->owPkt,
                             frontendLatency + backendLatency,
                             false);
            reqBuffer.at(copyOwPkt->getAddr()) =  new reqBufferEntry(
                                orbEntry->validEntry,
                                orbEntry->arrivalTick,
                                orbEntry->tagDC, orbEntry->indexDC,
                                copyOwPkt,
                                orbEntry->dccPkt,
                                orbEntry->state,
                                orbEntry->isHit,
                                orbEntry->conflict,
                                orbEntry->dirtyLineAddr,
                                orbEntry->handleDirtyLine,
                                orbEntry->drRd,
                                orbEntry->drWr,
                                orbEntry->nvWait,
                                orbEntry->nvRd,
                                orbEntry->nvWr,
                                orbEntry->nvmIssueReadyTime,
                                orbEntry->dramRdDevTime,
                                orbEntry->dramWrDevTime,
                                orbEntry->nvmRdDevTime);
            delete orbEntry;

            orbEntry = reqBuffer.at(addrDramRespReady.front());
    }

    // Write Hit
    if (orbEntry->owPkt->isWrite() &&
        orbEntry->dccPkt->isRead() &&
        orbEntry->dccPkt->isDram() &&
        orbEntry->isHit) {
            // This is a write request in initial read state.
            // Delete its dcc packet which is read and create
            // a new one which is write.
            delete orbEntry->dccPkt;

            orbEntry->dccPkt = dram->decodePacket(orbEntry->owPkt,
                                                  orbEntry->owPkt->getAddr(),
                                                  orbEntry->owPkt->getSize(),
                                                  false, true);
            orbEntry->dccPkt->entryTime = orbEntry->arrivalTick;

            // pass the second argument "false" to
            // indicate a write access to dram
            dram->setupRank(orbEntry->dccPkt->rank, false);

            //** transition to dramWrite
            orbEntry->state = dramWrite;
            orbEntry->drWr = curTick();

            pktDramWrite[0].push_back(orbEntry->dccPkt);

            if (pktDramWrite[0].size() >= (orbMaxSize*dramWrDrainPerc)) {
                stallRds = true;
                drainDramWrite = true;
                if (!overallWriteEvent.scheduled()) {
                    schedule(overallWriteEvent,
                             std::max(nextReqTime, curTick()));
                }
            }

            unsigned rdsNum = pktDramRead[0].size() +
                              pktNvmReadWaitIssue[0].size() +
                              pktNvmRead[0].size();
            unsigned wrsNum = pktNvmWrite[0].size() +
                              pktDramWrite[0].size();

            if ((rdsNum == 0 && wrsNum != 0) ||
                (wrsNum >= writeHighThreshold)) {

                stallRds = true;

                if (!overallWriteEvent.scheduled()) {
                    schedule(overallWriteEvent,
                             std::max(nextReqTime, curTick()));
                }

            }

            if (pktDramWrite[0].size() > maxDrWrEv) {
                maxDrWrEv = pktDramWrite[0].size();
                stats.maxDrWrEvQ = pktDramWrite[0].size();
            }
    }

    // Miss
    if ((orbEntry->owPkt->isRead() &&
         orbEntry->dccPkt->isRead() &&
         orbEntry->dccPkt->isDram() &&
         !orbEntry->isHit) ||
        (orbEntry->owPkt->isWrite() &&
         orbEntry->dccPkt->isRead() &&
         orbEntry->dccPkt->isDram() &&
         !orbEntry->isHit)) {
        // initiate a NVM read
        // delete the current dcc pkt which is dram read.
        delete orbEntry->dccPkt;

        // creating an nvm read dcc-pkt
        orbEntry->dccPkt = nvm->decodePacket(orbEntry->owPkt,
                                             orbEntry->owPkt->getAddr(),
                                             orbEntry->owPkt->getSize(),
                                             true, false);
        orbEntry->dccPkt->entryTime = orbEntry->arrivalTick;

        // pass the second argument "true" to
        // indicate a read access to nvm
        nvm->setupRank(orbEntry->dccPkt->rank, true);

        // ready time will be calculated later in doBurstAccess
        // in processNvmReadEvent
        orbEntry->dccPkt->readyTime = MaxTick;

        //** transition to waitingToIssueNvmRead
        // setting the state to waitingToIssueNvmRead
        orbEntry->state = waitingToIssueNvmRead;
        orbEntry->nvWait = curTick();

        if (pktNvmReadWaitIssue[0].empty() && !stallRds) {
            assert(!waitingToIssueNvmReadEvent.scheduled());
            schedule(waitingToIssueNvmReadEvent, curTick());
        }
        else {
            assert(waitingToIssueNvmReadEvent.scheduled() || stallRds);
        }

        pktNvmReadWaitIssue[0].push_back(orbEntry->dccPkt);

        if (pktNvmReadWaitIssue[0].size() > maxNvRdIssEv) {
            maxNvRdIssEv = pktNvmReadWaitIssue[0].size();
            stats.maxNvRdIssEvQ = pktNvmReadWaitIssue[0].size();
        }
    }

    if (orbEntry->handleDirtyLine) {
        numDirtyLinesInDrRdRespQ--;
    }

    addrDramRespReady.pop_front();

    if (!addrDramRespReady.empty()) {
        assert(reqBuffer.at(addrDramRespReady.front())->dccPkt->readyTime
                >= curTick());
        assert(!respDramReadEvent.scheduled());
        schedule(respDramReadEvent,
                 reqBuffer.at(addrDramRespReady.front())->dccPkt->readyTime);
    } else {

        unsigned rdsNum = pktDramRead[0].size() +
                          pktNvmReadWaitIssue[0].size() +
                          pktNvmRead[0].size();
        unsigned wrsNum = pktNvmWrite[0].size() +
                          pktDramWrite[0].size();

        // if there is nothing left in any queue, signal a drain
        if (drainState() == DrainState::Draining &&
            !wrsNum && !rdsNum &&
            allIntfDrained()) {
            DPRINTF(Drain, "Controller done draining\n");
            signalDrainDone();
        } else if (orbEntry->owPkt->isRead() &&
                   orbEntry->dccPkt->isDram() &&
                   orbEntry->isHit) {
            // check the refresh state and kick the refresh event loop
            // into action again if banks already closed and just waiting
            // for read to complete
            dram->checkRefreshState(orbEntry->dccPkt->rank);
        }
    }

    if (orbEntry->owPkt->isRead() &&
        orbEntry->dccPkt->isDram() &&
        orbEntry->isHit) {
            // Remove the request from the ORB and
            // bring in a conflicting req waiting
            // in the CRB, if any.
            canRetry = !resumeConflictingReq(orbEntry);
    }

    if (retry && canRetry) {
        retry = false;
        port.sendRetryReq();
    }
}

void
DcacheCtrl::processWaitingToIssueNvmReadEvent()
{
    if (stallRds) {
        return;
    }

    if (nvm->readsWaitingToIssue()) {
        assert(!pktNvmReadWaitIssue[0].empty());

        MemPacketQueue::iterator to_read;

        bool read_found = false;

        bool switched_cmd_type = (busState == DcacheCtrl::WRITE);

        if (switched_cmd_type) {
            stats.wrToRdTurnAround++;
        }

        for (auto queue = pktNvmReadWaitIssue.rbegin();
                    queue != pktNvmReadWaitIssue.rend(); ++queue) {
            to_read = chooseNext((*queue), switched_cmd_type ?
                                    minWriteToReadDataGap() : 0, false);
            if (to_read != queue->end()) {
                // candidate read found
                read_found = true;
                break;
            }
        }

        auto e = reqBuffer.at(pktNvmReadWaitIssue[0].front()->getAddr());

        if (read_found) {
            e = reqBuffer.at((*to_read)->getAddr());
        }

        assert(e->validEntry);
        assert(e->state == waitingToIssueNvmRead);
        assert(!e->dccPkt->isDram());
        assert(!e->isHit);

        nvm->processReadPkt(e->dccPkt);

        e->nvmIssueReadyTime = e->dccPkt->readyTime;

        //** transition to nvmread
        e->state = nvmRead;
        e->nvRd = e->dccPkt->readyTime;

        if (pktNvmRead[0].empty()) {
            assert(!nvmReadEvent.scheduled());
            schedule(nvmReadEvent, std::max(nextReqTime,
                                    e->dccPkt->readyTime+1));
        } else if (nvmReadEvent.when() > e->dccPkt->readyTime) {
            // move it sooner in time, to the first read with data
            reschedule(nvmReadEvent, std::max(nextReqTime,
                                    e->dccPkt->readyTime+1));
        } else {
            assert(nvmReadEvent.scheduled());
        }

        pktNvmRead[0].push_back(e->dccPkt);

        if (pktNvmRead[0].size() > maxNvRdEv) {
            maxNvRdEv = pktNvmRead[0].size();
            stats.maxNvRdEvQ = pktNvmRead[0].size();
        }

        if (read_found) {
            pktNvmReadWaitIssue[0].erase(to_read);
        }
        else {
            pktNvmReadWaitIssue[0].erase(pktNvmReadWaitIssue[0].begin());
        }

        unsigned rdsNum = pktDramRead[0].size() +
                        pktNvmReadWaitIssue[0].size() +
                        pktNvmRead[0].size();
        unsigned wrsNum = pktNvmWrite[0].size() +
                        pktDramWrite[0].size();

        if ((rdsNum == 0 && wrsNum != 0) ||
            (wrsNum >= writeHighThreshold)) {

            stallRds = true;

            if (!overallWriteEvent.scheduled()) {
                schedule(overallWriteEvent, std::max(nextReqTime, curTick()));
            }
        }
    }

    else {
        assert(!pktNvmRead[0].empty());
        schedule(waitingToIssueNvmReadEvent, nvmReadEvent.when()+2);
        return;
    }

    if (!waitingToIssueNvmReadEvent.scheduled() &&
        !pktNvmReadWaitIssue[0].empty()) {
        schedule(waitingToIssueNvmReadEvent, curTick());
    }

}

void
DcacheCtrl::processNvmReadEvent()
{
    if (stallRds) {
        return;
    }

    assert(!pktNvmRead[0].empty());

    MemPacketQueue::iterator to_read;

    bool read_found = false;

    bool switched_cmd_type = (busState == DcacheCtrl::WRITE);

    if (switched_cmd_type) {
        stats.wrToRdTurnAround++;
    }

    busState = DcacheCtrl::READ;

    for (auto queue = pktNvmRead.rbegin();
                 queue != pktNvmRead.rend(); ++queue) {
        to_read = chooseNext((*queue), switched_cmd_type ?
                                minWriteToReadDataGap() : 0, false);
        if (to_read != queue->end()) {
            // candidate read found
            read_found = true;
            break;
        }
    }

    reqBufferEntry* e;

    int index = 0;

    if (read_found) {
        e = reqBuffer.at((*to_read)->getAddr());
    }
    else {
        schedule(nvmReadEvent,
                 std::max(nextReqTime, curTick()+nvm->getTBurst()));

        return;
    }

    assert(e->validEntry);
    assert(!e->isHit);
    assert(!e->dccPkt->isDram());
    assert(e->state == nvmRead);

    assert(packetReady(e->dccPkt));

    Tick cmd_at = doBurstAccess(e->dccPkt);

    e->nvmRdDevTime = e->dccPkt->readyTime - cmd_at;

    // sanity check
    assert(e->dccPkt->size <= (e->dccPkt->isDram() ?
                                dram->bytesPerBurst() :
                                nvm->bytesPerBurst()));
    assert(e->dccPkt->readyTime >= curTick());

    if (e->owPkt->isRead() && !e->isHit) {
        logResponse(DcacheCtrl::READ,
                    e->dccPkt->requestorId(),
                    e->dccPkt->qosValue(),
                    e->owPkt->getAddr(), 1,
                    e->dccPkt->readyTime - e->dccPkt->entryTime);
    }

    if (addrNvmRespReady.empty()) {
        assert(!respNvmReadEvent.scheduled());
        schedule(respNvmReadEvent, e->dccPkt->readyTime);
    }
    else {
        assert(reqBuffer.at(addrNvmRespReady.back())->dccPkt->readyTime
                            <= e->dccPkt->readyTime);

        assert(respNvmReadEvent.scheduled());
    }

    addrNvmRespReady.push_back(e->owPkt->getAddr());

    if (addrNvmRespReady.size() > maxNvRdRespEv) {
        maxNvRdRespEv = addrNvmRespReady.size();
        stats.maxNvRdRespEvQ = addrNvmRespReady.size();
    }

    //** keeping the state as it is, no transition
    e->state = nvmRead;

    if (read_found) {
        pktNvmRead[0].erase(to_read);
    }
    else {
        pktNvmRead[0].erase(pktNvmRead[0].begin()+index);
    }

    if (!pktNvmRead[0].empty()) {
        assert(!nvmReadEvent.scheduled());
        auto min = pktNvmRead[0].front();
        //index = 0;
        for (int i=0; i<pktNvmRead[0].size(); i++) {
            if (min->readyTime > pktNvmRead[0].at(i)->readyTime &&
                min->readyTime != pktNvmRead[0].at(i)->readyTime) {
                min = pktNvmRead[0].at(i);
                //index = i;
            }
        }
        Tick maxTick = std::max(nextReqTime, curTick());
        schedule(nvmReadEvent, std::max(maxTick, min->readyTime+1));
    }

    unsigned rdsNum = pktDramRead[0].size() +
                      pktNvmReadWaitIssue[0].size() +
                      pktNvmRead[0].size();
    unsigned wrsNum = pktNvmWrite[0].size() +
                      pktDramWrite[0].size();

    if (//(rdsNum == 0 && wrsNum != 0 && wrsNum >= writeLowThreshold) ||
        (rdsNum == 0 && wrsNum != 0) ||
        (wrsNum >= writeHighThreshold)) {

        stallRds = true;

        if (!overallWriteEvent.scheduled()) {
            schedule(overallWriteEvent, std::max(nextReqTime, curTick()));
        }
    }
}

void
DcacheCtrl::processRespNvmReadEvent()
{
    assert(!addrNvmRespReady.empty());

    reqBufferEntry* orbEntry = reqBuffer.at(addrNvmRespReady.front());

    // A series of sanity check
    assert(orbEntry->validEntry);
    assert(orbEntry->dccPkt->isRead());
    assert(!orbEntry->dccPkt->isDram());
    assert(orbEntry->state == nvmRead);
    assert(!orbEntry->isHit);
    assert(orbEntry->dccPkt->readyTime == curTick());

    // Read miss from dram cache, now is available
    // to send the response back to requestor
    if (orbEntry->owPkt->isRead() && !orbEntry->isHit) {

        PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
                                         false,
                                         orbEntry->owPkt->isRead());

        accessAndRespond(orbEntry->owPkt,
                         frontendLatency + backendLatency,
                         false);
        reqBuffer.at(copyOwPkt->getAddr()) =  new reqBufferEntry(
                            orbEntry->validEntry,
                            orbEntry->arrivalTick,
                            orbEntry->tagDC, orbEntry->indexDC,
                            copyOwPkt,
                            orbEntry->dccPkt,
                            orbEntry->state,
                            orbEntry->isHit,
                            orbEntry->conflict,
                            orbEntry->dirtyLineAddr,
                            orbEntry->handleDirtyLine,
                            orbEntry->drRd,
                            orbEntry->drWr,
                            orbEntry->nvWait,
                            orbEntry->nvRd,
                            orbEntry->nvWr,
                            orbEntry->nvmIssueReadyTime,
                            orbEntry->dramRdDevTime,
                            orbEntry->dramWrDevTime,
                            orbEntry->nvmRdDevTime);
        delete orbEntry;
        orbEntry = reqBuffer.at(addrNvmRespReady.front());

    }

    // There has been a DRAM cache miss,
    // initiate a DRAM write to bring it to DRAM cache
    delete orbEntry->dccPkt;

    // creating a new dram write dcc-pkt
    orbEntry->dccPkt = dram->decodePacket(orbEntry->owPkt,
                                            orbEntry->owPkt->getAddr(),
                                            orbEntry->owPkt->getSize(),
                                            false,
                                            true);
    orbEntry->dccPkt->entryTime = orbEntry->arrivalTick;

    // pass the second argument "false" to
    // indicate a write access to dram
    dram->setupRank(orbEntry->dccPkt->rank, false);

    //** transition to dramWrite
    // update the state of the orb entry
    orbEntry->state = dramWrite;
    orbEntry->drWr = curTick();

    assert(orbEntry->dccPkt->isDram());
    assert(orbEntry->dccPkt->isWrite());
    assert(orbEntry->state==dramWrite);
    pktDramWrite[0].push_back(reqBuffer.at(addrNvmRespReady.front())->dccPkt);

    if (pktDramWrite[0].size() >= (orbMaxSize*dramWrDrainPerc)) {

        stallRds = true;

        drainDramWrite = true;

        if (!overallWriteEvent.scheduled()) {
            schedule(overallWriteEvent, std::max(nextReqTime, curTick()));
        }
    }

    unsigned rdsNum = pktDramRead[0].size() +
                      pktNvmReadWaitIssue[0].size() +
                      pktNvmRead[0].size();
    unsigned wrsNum = pktNvmWrite[0].size() +
                      pktDramWrite[0].size();

    if ((rdsNum == 0 && wrsNum != 0) ||
        (wrsNum >= writeHighThreshold)) {

        stallRds = true;

        if (!overallWriteEvent.scheduled()) {
            schedule(overallWriteEvent, std::max(nextReqTime, curTick()));
        }
    }

    if (pktDramWrite[0].size() > maxDrWrEv) {
        maxDrWrEv = pktDramWrite[0].size();
        stats.maxDrWrEvQ = pktDramWrite[0].size();
    }

    addrNvmRespReady.pop_front();

    if (!addrNvmRespReady.empty()) {
        assert(reqBuffer.at(addrNvmRespReady.front())->dccPkt->readyTime
                >= curTick());
        assert(!respNvmReadEvent.scheduled());
        schedule(respNvmReadEvent,
        reqBuffer.at(addrNvmRespReady.front())->dccPkt->readyTime);
    }
}

void
DcacheCtrl::processOverallWriteEvent()
{
    assert(stallRds);

    assert(!pktDramWrite[0].empty() || !pktNvmWrite[0].empty());

    if (drainDramWrite) {

        drWrCounter++;

        MemPacketQueue::iterator to_write;

        bool write_found = false;

        bool switched_cmd_type = (busState == DcacheCtrl::READ);

        if (switched_cmd_type) {
            stats.rdToWrTurnAround++;
        }

        for (auto queue = pktDramWrite.rbegin();
                    queue != pktDramWrite.rend(); ++queue) {
            to_write = chooseNext((*queue), switched_cmd_type ?
                                    minReadToWriteDataGap() : 0, true);
            if (to_write != queue->end()) {
                // candidate write found
                write_found = true;
                break;
            }
        }

        if (!write_found) {

            schedule(overallWriteEvent,
                     std::max(nextReqTime, curTick()+dram->getTBurst()));

            return;
        }

        auto e = reqBuffer.at(pktDramWrite[0].front()->getAddr());

        if (write_found) {
            e = reqBuffer.at((*to_write)->getAddr());
        }

        bool canRetry = false;

        assert(e->validEntry);
        if (e->owPkt->isRead()) {
            assert(!e->isHit);
        }
        assert(e->dccPkt->isDram());
        assert(e->state == dramWrite);
        assert(e->dccPkt->size <=
                                (e->dccPkt->isDram() ?
                                dram->bytesPerBurst() :
                                nvm->bytesPerBurst()) );

        busState = DcacheCtrl::WRITE;

        assert(packetReady(e->dccPkt));

        Tick cmd_at = doBurstAccess(e->dccPkt);

        e->dramWrDevTime = e->dccPkt->readyTime - cmd_at;

        if (e->owPkt->isWrite()) {
            // log the response
            logResponse(DcacheCtrl::WRITE,
                        e->dccPkt->requestorId(),
                        e->dccPkt->qosValue(),
                        e->owPkt->getAddr(), 1,
                        e->dccPkt->readyTime -
                        e->dccPkt->entryTime);
        }

        // Remove the request from the ORB and
        // bring in a conflicting req waiting
        // in the CRB, if any.
        canRetry = !resumeConflictingReq(e);

        if (write_found) {
            pktDramWrite[0].erase(to_write);
        }
        else {
            pktDramWrite[0].erase(pktDramWrite[0].begin());
        }

        if (retry && canRetry) {
            retry = false;
            port.sendRetryReq();
        }

        if (drWrCounter < minWritesPerSwitch && !pktDramWrite[0].empty()) {

            assert(!overallWriteEvent.scheduled());

            drainDramWrite = true;

            stallRds = true;

            schedule(overallWriteEvent, std::max(nextReqTime, curTick()));

            return;
        }
        else if (drainNvmWrite) {

            assert(!pktNvmWrite[0].empty());

            assert(!overallWriteEvent.scheduled());

            drainDramWrite = false;

            stallRds = true;

            schedule(overallWriteEvent, std::max(nextReqTime, curTick()));

            return;
        }
        else if ((pktDramRead[0].empty() && pktNvmReadWaitIssue[0].empty()
                    && pktNvmRead[0].empty()) &&
                 (!pktDramWrite[0].empty() || !pktNvmWrite[0].empty())) {

            assert(!overallWriteEvent.scheduled());

            drainDramWrite = false;

            drainNvmWrite = false;

            stallRds = true;

            drWrCounter = 0;

            nvWrCounter = 0;

            schedule(overallWriteEvent, std::max(nextReqTime, curTick()));

            return;

        }
        else {
            drainDramWrite = false;
            stallRds = false;
            drWrCounter = 0;
            nvWrCounter = 0;

            if (!pktDramRead[0].empty() && !dramReadEvent.scheduled()) {
                schedule(dramReadEvent, std::max(nextReqTime, curTick()));
            }
            if (!pktNvmReadWaitIssue[0].empty() &&
                !waitingToIssueNvmReadEvent.scheduled()) {
                schedule(waitingToIssueNvmReadEvent,
                std::max(nextReqTime, curTick()));
            }
            if (!pktNvmRead[0].empty() && !nvmReadEvent.scheduled()) {
                schedule(nvmReadEvent, std::max(nextReqTime, curTick()));
            }
            return;
        }
    }

    if (drainNvmWrite) {

        if (!nvm->writeRespQueueFull()) {

            nvWrCounter++;

            MemPacketQueue::iterator to_write;

            bool write_found = false;

            bool switched_cmd_type = (busState == DcacheCtrl::READ);

            if (switched_cmd_type) {
                stats.rdToWrTurnAround++;
            }

            busState = DcacheCtrl::WRITE;

            for (auto queue = pktNvmWrite.rbegin();
                        queue != pktNvmWrite.rend(); ++queue) {
                to_write = chooseNext((*queue), switched_cmd_type ?
                                        minReadToWriteDataGap() : 0, false);
                if (to_write != queue->end()) {
                    // candidate write found
                    write_found = true;
                    break;
                }
            }

            auto e = pktNvmWrite[0].front();

            if (write_found) {
                e = (*to_write);
            }
            else {

                schedule(overallWriteEvent,
                         std::max(nextReqTime, curTick()+nvm->getTBurst()));

                return;
            }

            // a series of sanity checks
            assert(!e->isDram());
            assert(e->isWrite());
            assert(e->size <= nvm->bytesPerBurst());

            assert(packetReady(e));

            Tick cmd_at = doBurstAccess(e);

            long long int stateTick =
                (e->readyTime - e->entryTime);

            assert(stateTick > 0);

            stats.timeInNvmWrite += stateTick;

            stats.totNumPktsNvmWr++;

            stats.nvWrDevTime += (e->readyTime - cmd_at);

            long long int  stateQT =
                    (cmd_at - e->entryTime);

            assert(stateQT >= 0);

            stats.nvmWrQingTime += stateQT;

            delete e;

            if (write_found) {
                pktNvmWrite[0].erase(to_write);
            }
            else {
                pktNvmWrite[0].erase(pktNvmWrite[0].begin());
            }

            if (nvWrCounter < minWritesPerSwitch && !pktNvmWrite[0].empty()) {
                assert(!overallWriteEvent.scheduled());

                drainNvmWrite = true;

                stallRds = true;

                schedule(overallWriteEvent, std::max(nextReqTime, curTick()));

                return;
            }
            else if ((pktDramRead[0].empty() && pktNvmReadWaitIssue[0].empty()
                        && pktNvmRead[0].empty()) &&
                    (!pktDramWrite[0].empty() || !pktNvmWrite[0].empty())) {

                assert(!overallWriteEvent.scheduled());

                drainDramWrite = false;

                drainNvmWrite = false;

                stallRds = true;

                drWrCounter = 0;

                nvWrCounter = 0;

                schedule(overallWriteEvent, std::max(nextReqTime, curTick()));

                return;

            }
            else {
                drainNvmWrite = false;
                stallRds = false;
                drWrCounter = 0;
                nvWrCounter = 0;

                if (!pktDramRead[0].empty() && !dramReadEvent.scheduled()) {
                    schedule(dramReadEvent, std::max(nextReqTime, curTick()));
                }
                if (!pktNvmReadWaitIssue[0].empty() &&
                    !waitingToIssueNvmReadEvent.scheduled()) {
                    schedule(waitingToIssueNvmReadEvent,
                    std::max(nextReqTime, curTick()));
                }
                if (!pktNvmRead[0].empty() && !nvmReadEvent.scheduled()) {
                    schedule(nvmReadEvent, std::max(nextReqTime, curTick()));
                }
                return;
            }
        }

        if (!overallWriteEvent.scheduled() &&
            !pktNvmWrite[0].empty() &&
            nvm->writeRespQueueFull()) {
                schedule(overallWriteEvent, std::max(nextReqTime,
                            nvm->writeRespQueueFront()+1));
                return;
        }
    }
    else if ((!pktDramWrite[0].empty() && drWrCounter < minDrWrPerSwitch) ||
        (!pktDramWrite[0].empty() && pktNvmWrite[0].empty() &&
        (drWrCounter+nvWrCounter)<minWritesPerSwitch) ||
        (pktDramRead[0].empty() && pktNvmReadWaitIssue[0].empty()
        && pktNvmRead[0].empty()
        && pktNvmWrite[0].empty() && !pktDramWrite[0].empty())) {

        drWrCounter++;

        MemPacketQueue::iterator to_write;

        bool write_found = false;

        bool switched_cmd_type = (busState == DcacheCtrl::READ);

        if (switched_cmd_type) {
            stats.rdToWrTurnAround++;
        }

        for (auto queue = pktDramWrite.rbegin();
                    queue != pktDramWrite.rend(); ++queue) {
            to_write = chooseNext((*queue), switched_cmd_type ?
                                    minReadToWriteDataGap() : 0, true);
            if (to_write != queue->end()) {
                // candidate write found
                write_found = true;
                break;
            }
        }

        if (!write_found) {

            schedule(overallWriteEvent,
                     std::max(nextReqTime, curTick()+dram->getTBurst()));

            return;
        }

        auto e = reqBuffer.at(pktDramWrite[0].front()->getAddr());

        if (write_found) {
            e = reqBuffer.at((*to_write)->getAddr());
        }

        bool canRetry = false;

        assert(e->validEntry);
        if (e->owPkt->isRead()) {
            assert(!e->isHit);
        }
        assert(e->dccPkt->isDram());
        assert(e->state == dramWrite);
        assert(e->dccPkt->size <=
                                (e->dccPkt->isDram() ?
                                dram->bytesPerBurst() :
                                nvm->bytesPerBurst()) );

        busState = DcacheCtrl::WRITE;

        assert(packetReady(e->dccPkt));

        Tick cmd_at = doBurstAccess(e->dccPkt);

        e->dramWrDevTime = e->dccPkt->readyTime - cmd_at;

        if (e->owPkt->isWrite()) {
            // log the response
            logResponse(DcacheCtrl::WRITE,
                        e->dccPkt->requestorId(),
                        e->dccPkt->qosValue(),
                        e->owPkt->getAddr(), 1,
                        e->dccPkt->readyTime -
                        e->dccPkt->entryTime);
        }

        // Remove the request from the ORB and
        // bring in a conflicting req waiting
        // in the CRB, if any.
        canRetry = !resumeConflictingReq(e);

        if (write_found) {
            pktDramWrite[0].erase(to_write);
        }
        else {
            pktDramWrite[0].erase(pktDramWrite[0].begin());
        }

        if (retry && canRetry) {
            retry = false;
            port.sendRetryReq();
        }
    }

    else if ((!pktNvmWrite[0].empty() && nvWrCounter < minNvWrPerSwitch) ||
             (!pktNvmWrite[0].empty() && pktDramWrite[0].empty() &&
             (drWrCounter+nvWrCounter)<minWritesPerSwitch) ||
        (pktDramRead[0].empty() && pktNvmReadWaitIssue[0].empty()
        && pktNvmRead[0].empty()
        && pktDramWrite[0].empty() && !pktNvmWrite[0].empty())) {

        if (!nvm->writeRespQueueFull()) {

            nvWrCounter++;

            MemPacketQueue::iterator to_write;

            bool write_found = false;

            bool switched_cmd_type = (busState == DcacheCtrl::READ);

            if (switched_cmd_type) {
                stats.rdToWrTurnAround++;
            }

            busState = DcacheCtrl::WRITE;

            for (auto queue = pktNvmWrite.rbegin();
                        queue != pktNvmWrite.rend(); ++queue) {
                to_write = chooseNext((*queue), switched_cmd_type ?
                                        minReadToWriteDataGap() : 0, false);
                if (to_write != queue->end()) {
                    // candidate write found
                    write_found = true;
                    break;
                }
            }

            auto e = pktNvmWrite[0].front();

            if (write_found) {
                e = (*to_write);
            }
            else {

                schedule(overallWriteEvent,
                         std::max(nextReqTime, curTick()+nvm->getTBurst()));

                return;
            }

            // a series of sanity checks
            assert(!e->isDram());
            assert(e->isWrite());
            assert(e->size <= nvm->bytesPerBurst());

            assert(packetReady(e));

            Tick cmd_at = doBurstAccess(e);

            long long int stateTick =
                (e->readyTime - e->entryTime);

            assert(stateTick > 0);

            stats.timeInNvmWrite += stateTick;

            stats.totNumPktsNvmWr++;

            stats.nvWrDevTime += (e->readyTime - cmd_at);

            long long int  stateQT =
                    (cmd_at - e->entryTime);

            assert(stateQT >= 0);

            stats.nvmWrQingTime += stateQT;

            delete e;

            if (write_found) {
                pktNvmWrite[0].erase(to_write);
            }
            else {
                pktNvmWrite[0].erase(pktNvmWrite[0].begin());
            }
        }

        if (!overallWriteEvent.scheduled() &&
            !pktNvmWrite[0].empty() &&
            nvm->writeRespQueueFull()) {
                schedule(overallWriteEvent, std::max(nextReqTime,
                            nvm->writeRespQueueFront()+1));
                return;
        }
    }

    if (
            !overallWriteEvent.scheduled() &&
            (
                (
                    (!pktDramRead[0].empty() || !pktNvmReadWaitIssue[0].empty()
                        || !pktNvmRead[0].empty()) &&
                    (!pktDramWrite[0].empty() || !pktNvmWrite[0].empty()) &&
                    (drWrCounter + nvWrCounter < minWritesPerSwitch)
                ) ||
                (
                    (pktDramRead[0].empty() && pktNvmReadWaitIssue[0].empty()
                        && pktNvmRead[0].empty()) &&
                    (!pktDramWrite[0].empty() || !pktNvmWrite[0].empty())
                )
            )
        ) {
        stallRds = true;
        schedule(overallWriteEvent, std::max(nextReqTime, curTick()));
    }
    else {
        stallRds = false;

        drWrCounter = 0;

        nvWrCounter = 0;

        if (!pktDramRead[0].empty() && !dramReadEvent.scheduled()) {
            schedule(dramReadEvent, std::max(nextReqTime, curTick()));
        }
        if (!pktNvmReadWaitIssue[0].empty() &&
            !waitingToIssueNvmReadEvent.scheduled()) {
            schedule(waitingToIssueNvmReadEvent,
            std::max(nextReqTime, curTick()));
        }
        if (!pktNvmRead[0].empty() && !nvmReadEvent.scheduled()) {
            schedule(nvmReadEvent, std::max(nextReqTime, curTick()));
        }
    }
}


void
DcacheCtrl::processRespondEvent()
{

}

void
DcacheCtrl::accessAndRespond(PacketPtr pkt, Tick static_latency, bool in_dram)
{
    DPRINTF(DcacheCtrl, "Responding to Address %lld.. \n",pkt->getAddr());

    bool needsResponse = pkt->needsResponse();
    // do the actual memory access which also turns the packet into a
    // response
    if (in_dram && dram && dram->getAddrRange().contains(pkt->getAddr())) {
        dram->access(pkt);
    } else if (!in_dram && nvm &&
                nvm->getAddrRange().contains(pkt->getAddr())) {
        nvm->access(pkt);
    } else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
    }

    // turn packet around to go back to requestor if response expected
    if (needsResponse) {
        // access already turned the packet into a response
        assert(pkt->isResponse());
        // response_time consumes the static latency and is charged also
        // with headerDelay that takes into account the delay provided by
        // the xbar and also the payloadDelay that takes into account the
        // number of data beats.
        Tick response_time = curTick() + static_latency + pkt->headerDelay +
                             pkt->payloadDelay;
        // Here we reset the timing of the packet before sending it out.
        pkt->headerDelay = pkt->payloadDelay = 0;

        // queue the packet in the response queue to be sent out after
        // the static latency has passed
        port.schedTimingResp(pkt, response_time);
    } else {
        // @todo the packet is going to be deleted, and the MemPacket
        // is still having a pointer to it
        pendingDelete.reset(pkt);
    }

    DPRINTF(DcacheCtrl, "Done\n");

    return;
}

void
DcacheCtrl::pruneBurstTick()
{
    auto it = burstTicks.begin();
    while (it != burstTicks.end()) {
        auto current_it = it++;
        if (curTick() > *current_it) {
            DPRINTF(DcacheCtrl, "Removing burstTick for %d\n", *current_it);
            burstTicks.erase(current_it);
        }
    }
}

Tick
DcacheCtrl::getBurstWindow(Tick cmd_tick)
{
    // get tick aligned to burst window
    Tick burst_offset = cmd_tick % commandWindow;
    return (cmd_tick - burst_offset);
}

Tick
DcacheCtrl::verifySingleCmd(Tick cmd_tick, Tick max_cmds_per_burst)
{
    // start with assumption that there is no contention on command bus
    Tick cmd_at = cmd_tick;

    // get tick aligned to burst window
    Tick burst_tick = getBurstWindow(cmd_tick);

    // verify that we have command bandwidth to issue the command
    // if not, iterate over next window(s) until slot found
    while (burstTicks.count(burst_tick) >= max_cmds_per_burst) {
        DPRINTF(DcacheCtrl, "Contention found on command bus at %d\n",
                burst_tick);
        burst_tick += commandWindow;
        cmd_at = burst_tick;
    }

    // add command into burst window and return corresponding Tick
    burstTicks.insert(burst_tick);
    return cmd_at;
}

Tick
DcacheCtrl::verifyMultiCmd(Tick cmd_tick, Tick max_cmds_per_burst,
                         Tick max_multi_cmd_split)
{
    // start with assumption that there is no contention on command bus
    Tick cmd_at = cmd_tick;

    // get tick aligned to burst window
    Tick burst_tick = getBurstWindow(cmd_tick);

    // Command timing requirements are from 2nd command
    // Start with assumption that 2nd command will issue at cmd_at and
    // find prior slot for 1st command to issue
    // Given a maximum latency of max_multi_cmd_split between the commands,
    // find the burst at the maximum latency prior to cmd_at
    Tick burst_offset = 0;
    Tick first_cmd_offset = cmd_tick % commandWindow;
    while (max_multi_cmd_split > (first_cmd_offset + burst_offset)) {
        burst_offset += commandWindow;
    }
    // get the earliest burst aligned address for first command
    // ensure that the time does not go negative
    Tick first_cmd_tick = burst_tick - std::min(burst_offset, burst_tick);

    // Can required commands issue?
    bool first_can_issue = false;
    bool second_can_issue = false;
    // verify that we have command bandwidth to issue the command(s)
    while (!first_can_issue || !second_can_issue) {
        bool same_burst = (burst_tick == first_cmd_tick);
        auto first_cmd_count = burstTicks.count(first_cmd_tick);
        auto second_cmd_count = same_burst ? first_cmd_count + 1 :
                                   burstTicks.count(burst_tick);

        first_can_issue = first_cmd_count < max_cmds_per_burst;
        second_can_issue = second_cmd_count < max_cmds_per_burst;

        if (!second_can_issue) {
            DPRINTF(DcacheCtrl, "Contention (cmd2) found on "
                                "command bus at %d\n",
                    burst_tick);
            burst_tick += commandWindow;
            cmd_at = burst_tick;
        }

        // Verify max_multi_cmd_split isn't violated when command 2 is shifted
        // If commands initially were issued in same burst, they are
        // now in consecutive bursts and can still issue B2B
        bool gap_violated = !same_burst &&
             ((burst_tick - first_cmd_tick) > max_multi_cmd_split);

        if (!first_can_issue || (!second_can_issue && gap_violated)) {
            DPRINTF(DcacheCtrl, "Contention (cmd1) found on "
                                "command bus at %d\n",
                    first_cmd_tick);
            first_cmd_tick += commandWindow;
        }
    }

    // Add command to burstTicks
    burstTicks.insert(burst_tick);
    burstTicks.insert(first_cmd_tick);

    return cmd_at;
}

bool
DcacheCtrl::inReadBusState(bool next_state) const
{
    // check the bus state
    if (next_state) {
        // use busStateNext to get the state that will be used
        // for the next burst
        return (busStateNext == DcacheCtrl::READ);
    } else {
        return (busState == DcacheCtrl::READ);
    }
}

bool
DcacheCtrl::inWriteBusState(bool next_state) const
{
    // check the bus state
    if (next_state) {
        // use busStateNext to get the state that will be used
        // for the next burst
        return (busStateNext == DcacheCtrl::WRITE);
    } else {
        return (busState == DcacheCtrl::WRITE);
    }
}

Tick
DcacheCtrl::doBurstAccess(MemPacket* dcc_pkt)
{
    // first clean up the burstTick set, removing old entries
    // before adding new entries for next burst
    pruneBurstTick();

    // When was command issued?
    Tick cmd_at;

    // Issue the next burst and update bus state to reflect
    // when previous command was issued
    if (dcc_pkt->isDram()) {
        std::tie(cmd_at, nextBurstAt) =
                 dram->doBurstAccess(dcc_pkt, nextBurstAt);//, queue);

        // Update timing for NVM ranks if NVM is configured on this channel
        if (nvm)
            nvm->addRankToRankDelay(cmd_at);

    } else {
        std::tie(cmd_at, nextBurstAt) =
                 nvm->doBurstAccess(dcc_pkt, nextBurstAt);

        // Update timing for NVM ranks if NVM is configured on this channel
        if (dram)
            dram->addRankToRankDelay(cmd_at);

    }

    DPRINTF(DcacheCtrl, "Access to %lld, ready at %lld next burst at %lld.\n",
            dcc_pkt->addr, dcc_pkt->readyTime, nextBurstAt);

    // Update the minimum timing between the requests, this is a
    // conservative estimate of when we have to schedule the next
    // request to not introduce any unecessary bubbles. In most cases
    // we will wake up sooner than we have to.
    nextReqTime = nextBurstAt - (dram ? dram->commandOffset() :
                                        nvm->commandOffset());


    // Update the common bus stats
    if (dcc_pkt->pkt != nullptr) {
        if (dcc_pkt->isRead()) {
            // Update latency stats
            stats.requestorReadTotalLat[dcc_pkt->requestorId()] +=
                dcc_pkt->readyTime - dcc_pkt->entryTime;
            stats.requestorReadBytes[dcc_pkt->requestorId()] += dcc_pkt->size;
        } else {
            stats.requestorWriteBytes[dcc_pkt->requestorId()] += dcc_pkt->size;
            stats.requestorWriteTotalLat[dcc_pkt->requestorId()] +=
                dcc_pkt->readyTime - dcc_pkt->entryTime;
        }
    }
    return cmd_at;
}

void
DcacheCtrl::processNextReqEvent()
{

}

bool
DcacheCtrl::packetReady(MemPacket* pkt)
{
    return (pkt->isDram() ?
        dram->burstReady(pkt) : nvm->burstReadyDCache(pkt));
}

Tick
DcacheCtrl::minReadToWriteDataGap()
{
    Tick dram_min = dram ?  dram->minReadToWriteDataGap() : MaxTick;
    Tick nvm_min = nvm ?  nvm->minReadToWriteDataGap() : MaxTick;
    return std::min(dram_min, nvm_min);
}

Tick
DcacheCtrl::minWriteToReadDataGap()
{
    Tick dram_min = dram ? dram->minWriteToReadDataGap() : MaxTick;
    Tick nvm_min = nvm ?  nvm->minWriteToReadDataGap() : MaxTick;
    return std::min(dram_min, nvm_min);
}

MemPacketQueue::iterator
DcacheCtrl::chooseNext(MemPacketQueue& queue, Tick extra_col_delay,
                        bool is_dram)
{
    // This method does the arbitration between requests.

    MemPacketQueue::iterator ret = queue.end();

    if (!queue.empty()) {
        if (queue.size() == 1) {
            // available rank corresponds to state refresh idle
            MemPacket* mem_pkt = *(queue.begin());
            if (packetReady(mem_pkt)) {
                ret = queue.begin();
                DPRINTF(DcacheCtrl, "Single request, going to a free rank\n");
            } else {
                DPRINTF(DcacheCtrl, "Single request, going to a busy rank\n");
            }
        } else if (memSchedPolicy == Enums::fcfs) {
            // check if there is a packet going to a free rank
            for (auto i = queue.begin(); i != queue.end(); ++i) {
                MemPacket* mem_pkt = *i;
                if (packetReady(mem_pkt)) {
                    ret = i;
                    break;
                }
            }
        } else if (memSchedPolicy == Enums::frfcfs) {
            ret = chooseNextFRFCFS(queue, extra_col_delay, is_dram);
        } else {
            panic("No scheduling policy chosen\n");
        }
    }
    return ret;
}

MemPacketQueue::iterator
DcacheCtrl::chooseNextFRFCFS(MemPacketQueue& queue, Tick extra_col_delay,
                            bool is_dram)
{
    auto selected_pkt_it = queue.end();
    Tick col_allowed_at = MaxTick;

    // time we need to issue a column command to be seamless
    const Tick min_col_at = std::max(nextBurstAt + extra_col_delay, curTick());

    if (is_dram) {
        std::tie(selected_pkt_it, col_allowed_at) =
                 dram->chooseNextFRFCFS(queue, min_col_at);
    } else {
        std::tie(selected_pkt_it, col_allowed_at) =
                 nvm->chooseNextFRFCFSDCache(queue, min_col_at);
    }

    if (selected_pkt_it == queue.end()) {
        DPRINTF(DcacheCtrl, "%s no available packets found\n", __func__);
    }

    return selected_pkt_it;
}

Addr
DcacheCtrl::burstAlign(Addr addr, bool is_dram) const
{
    if (is_dram)
        return (addr & ~(Addr(dram->bytesPerBurst() - 1)));
    else
        return (addr & ~(Addr(nvm->bytesPerBurst() - 1)));
}

DcacheCtrl::CtrlStats::CtrlStats(DcacheCtrl &_ctrl)
    : Stats::Group(&_ctrl),
    ctrl(_ctrl),

    ADD_STAT(readReqs, "Number of read requests accepted"),
    ADD_STAT(writeReqs, "Number of write requests accepted"),

    ADD_STAT(readBursts,
             "Number of controller read bursts, "
             "including those serviced by the write queue"),
    ADD_STAT(writeBursts,
             "Number of controller write bursts, "
             "including those merged in the write queue"),
    ADD_STAT(servicedByWrQ,
             "Number of controller read bursts serviced by the write queue"),
    ADD_STAT(mergedWrBursts,
             "Number of controller write bursts merged with an existing one"),

    //ADD_STAT(neitherReadNorWriteReqs,
    //         "Number of requests that are neither read nor write"),

    ADD_STAT(avgRdQLen, "Average read queue length when enqueuing"),
    ADD_STAT(avgWrQLen, "Average write queue length when enqueuing"),

    ADD_STAT(numRdRetry, "Number of times read queue was full causing retry"),
    ADD_STAT(numWrRetry, "Number of times write queue was full causing retry"),

    ADD_STAT(readPktSize, "Read request sizes (log2)"),
    ADD_STAT(writePktSize, "Write request sizes (log2)"),

    ADD_STAT(rdToWrTurnAround, "Read to write switch"),
    ADD_STAT(wrToRdTurnAround, "Write to read switch)"),

    //ADD_STAT(rdQLenPdf, "What read queue length does an incoming req see"),
    //ADD_STAT(wrQLenPdf, "What write queue length does an incoming req see"),

    //ADD_STAT(rdPerTurnAround,
    //         "Reads before turning the bus around for writes"),
    //ADD_STAT(wrPerTurnAround,
    //         "Writes before turning the bus around for reads"),

    ADD_STAT(bytesReadWrQ, "Total number of bytes read from write queue"),
    ADD_STAT(bytesReadSys, "Total read bytes from the system interface side"),
    ADD_STAT(bytesWrittenSys,
             "Total written bytes from the system interface side"),

    ADD_STAT(avgRdBWSys, "Average system read bandwidth in MiByte/s"),
    ADD_STAT(avgWrBWSys, "Average system write bandwidth in MiByte/s"),

    ADD_STAT(totGap, "Total gap between requests"),
    ADD_STAT(avgGap, "Average gap between requests"),

    ADD_STAT(requestorReadBytes,
            "Per-requestor bytes read from memory"),
    ADD_STAT(requestorWriteBytes,
            "Per-requestor bytes write to memory"),
    ADD_STAT(requestorReadRate,
             "Per-requestor bytes read from memory rate (Bytes/sec)"),
    ADD_STAT(requestorWriteRate,
             "Per-requestor bytes write to memory rate (Bytes/sec)"),
    ADD_STAT(requestorReadAccesses,
             "Per-requestor read serviced memory accesses"),
    ADD_STAT(requestorWriteAccesses,
             "Per-requestor write serviced memory accesses"),
    ADD_STAT(requestorReadTotalLat,
             "Per-requestor read total memory access latency"),
    ADD_STAT(requestorWriteTotalLat,
             "Per-requestor write total memory access latency"),
    ADD_STAT(requestorReadAvgLat,
             "Per-requestor read average memory access latency"),
    ADD_STAT(requestorWriteAvgLat,
             "Per-requestor write average memory access latency"),

    ADD_STAT(numHits,
            "Total number of hits on DRAM cache"),
    ADD_STAT(numMisses,
            "Total number of misses on DRAM cache"),
    ADD_STAT(numRdHits,
            "Total number of read hits on DRAM cache"),
    ADD_STAT(numWrHits,
            "Total number of write hits on DRAM cache"),
    ADD_STAT(numRdMisses,
            "Total number of read misses on DRAM cache"),
    ADD_STAT(numWrMisses,
            "Total number of write misses on DRAM cache"),
    ADD_STAT(numColdMisses,
            "Total number of misses on DRAM cache due to"
            " first reference to a cache block"),
    ADD_STAT(numHotMisses,
            "Total number of misses on DRAM cache that are not cold miss"),
    ADD_STAT(numWrBacks,
            "Total number of write backs from DRAM cache to main memory"),
    ADD_STAT(totNumConf,
            "Total number of packets conflicted on DRAM cache"),
    ADD_STAT(totNumConfBufFull,
            "Total number of packets conflicted and couldn't "
            "enter confBuffer"),

    ADD_STAT(timeInDramRead,
             "Total time spent in dram read state in ns"),
    ADD_STAT(timeInDramWrite,
            "Total time spent in dram write state in ns"),
    ADD_STAT(timeInWaitingToIssueNvmRead,
            "Total time spent in waitingToIssueNvmRead state in ns"),
    ADD_STAT(timeInNvmRead,
            "Total time spent in nvmRead state in ns"),
    ADD_STAT(timeInNvmWrite,
            "Total time spent in nvmWrite state in ns"),

    ADD_STAT(drRdQingTime,
            "Total time spent as DRAM read queuing time in ns"),
    ADD_STAT(drWrQingTime,
            "Total time spent as DRAM write queuing time in ns"),
    ADD_STAT(nvmRdQingTime,
            "Total time spent as NVM read queuing time in ns"),
    ADD_STAT(nvmWrQingTime,
            "Total time spent as NVM write queuing time in ns"),

    ADD_STAT(drRdDevTime,
            "Total time spent as DRAM read device time in Ticks"),
    ADD_STAT(drWrDevTime,
            "Total time spent as DRAM write device time in Ticks"),
    ADD_STAT(nvRdDevTime,
            "Total time spent as NVM read device time in Ticks"),
    ADD_STAT(nvWrDevTime,
            "Total time spent as NVM write device time in Ticks"),

    ADD_STAT(totNumPktsDrRd,
            "Total number of packets enterted to Dram read state"),
    ADD_STAT(totNumPktsDrWr,
            "Total number of packets enterted to Dram write state"),
    ADD_STAT(totNumPktsNvmRdWait,
            "Total number of packets enterted to NVM waitToIssue state"),
    ADD_STAT(totNumPktsNvmRd,
            "Total number of packets enterted to NVM read state"),
    ADD_STAT(totNumPktsNvmWr,
            "Total number of packets enterted to NVM write state"),

    ADD_STAT(maxNumConf,
            "Maximum number of packets conflicted on DRAM cache"),
    ADD_STAT(maxDrRdEvQ,
            "Maximum number of packets in DrRdEvent concurrently"),
    ADD_STAT(maxDrRdRespEvQ,
            "Maximum number of packets in DrRdRespEvent concurrently"),
    ADD_STAT(maxDrWrEvQ,
            "Maximum number of packets in DrWrEvent concurrently"),
    ADD_STAT(maxNvRdIssEvQ,
            "Maximum number of packets in NvRdIssEvent concurrently"),
    ADD_STAT(maxNvRdEvQ,
            "Maximum number of packets in NvRdEvent concurrently"),
    ADD_STAT(maxNvRdRespEvQ,
            "Maximum number of packets in NvRdRespEvent concurrently"),
    ADD_STAT(maxNvWrEvQ,
            "Maximum number of packets in NvWrEvent concurrently")

{
}

void
DcacheCtrl::CtrlStats::regStats()
{
    using namespace Stats;

    assert(ctrl.system());
    const auto max_requestors = ctrl.system()->maxRequestors();

    avgRdQLen.precision(2);
    avgWrQLen.precision(2);

    readPktSize.init(ceilLog2(ctrl.system()->cacheLineSize()) + 1);
    writePktSize.init(ceilLog2(ctrl.system()->cacheLineSize()) + 1);

    avgRdBWSys.precision(2);
    avgWrBWSys.precision(2);
    avgGap.precision(2);

    // per-requestor bytes read and written to memory
    requestorReadBytes
        .init(max_requestors)
        .flags(nozero | nonan);

    requestorWriteBytes
        .init(max_requestors)
        .flags(nozero | nonan);

    // per-requestor bytes read and written to memory rate
    requestorReadRate
        .flags(nozero | nonan)
        .precision(12);

    requestorReadAccesses
        .init(max_requestors)
        .flags(nozero);

    requestorWriteAccesses
        .init(max_requestors)
        .flags(nozero);

    requestorReadTotalLat
        .init(max_requestors)
        .flags(nozero | nonan);

    requestorReadAvgLat
        .flags(nonan)
        .precision(2);

    requestorWriteRate
        .flags(nozero | nonan)
        .precision(12);

    requestorWriteTotalLat
        .init(max_requestors)
        .flags(nozero | nonan);

    requestorWriteAvgLat
        .flags(nonan)
        .precision(2);

    for (int i = 0; i < max_requestors; i++) {
        const std::string requestor = ctrl.system()->getRequestorName(i);
        requestorReadBytes.subname(i, requestor);
        requestorReadRate.subname(i, requestor);
        requestorWriteBytes.subname(i, requestor);
        requestorWriteRate.subname(i, requestor);
        requestorReadAccesses.subname(i, requestor);
        requestorWriteAccesses.subname(i, requestor);
        requestorReadTotalLat.subname(i, requestor);
        requestorReadAvgLat.subname(i, requestor);
        requestorWriteTotalLat.subname(i, requestor);
        requestorWriteAvgLat.subname(i, requestor);
    }

    // Formula stats
    avgRdBWSys = (bytesReadSys / 1000000) / simSeconds;
    avgWrBWSys = (bytesWrittenSys / 1000000) / simSeconds;

    avgGap = totGap / (readReqs + writeReqs);

    requestorReadRate = requestorReadBytes / simSeconds;
    requestorWriteRate = requestorWriteBytes / simSeconds;
    requestorReadAvgLat = requestorReadTotalLat / requestorReadAccesses;
    requestorWriteAvgLat = requestorWriteTotalLat / requestorWriteAccesses;
}

void
DcacheCtrl::recvFunctional(PacketPtr pkt)
{
    if (dram && dram->getAddrRange().contains(pkt->getAddr())) {
        // rely on the abstract memory
        dram->functionalAccess(pkt);
    } else if (nvm && nvm->getAddrRange().contains(pkt->getAddr())) {
        // rely on the abstract memory
        nvm->functionalAccess(pkt);
   } else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
   }
}

Port &
DcacheCtrl::getPort(const std::string &if_name, PortID idx)
{
    if (if_name != "port") {
        return QoS::MemCtrl::getPort(if_name, idx);
    } else {
        return port;
    }
}

bool
DcacheCtrl::allIntfDrained() const
{
   // ensure dram is in power down and refresh IDLE states
   bool dram_drained = !dram || dram->allRanksDrained();
   // No outstanding NVM writes
   // All other queues verified as needed with calling logic
   bool nvm_drained = !nvm || nvm->allRanksDrained();
   return (dram_drained && nvm_drained);
}

DrainState
DcacheCtrl::drain()
{
    // if there is anything in any of our internal queues, keep track
    // of that as well
    unsigned rdsNum = pktDramRead[0].size() +
                      pktNvmReadWaitIssue[0].size() +
                      pktNvmRead[0].size();
    unsigned wrsNum = pktNvmWrite[0].size() +
                      pktDramWrite[0].size();
    if (!(!wrsNum && !rdsNum &&
          addrDramRespReady.empty() &&
          allIntfDrained())) {

        DPRINTF(Drain, "Memory controller not drained, write: %d, read: %d,"
                " resp: %d\n", wrsNum, rdsNum,
                addrDramRespReady.size());

        // the only queue that is not drained automatically over time
        // is the write queue, thus kick things into action if needed

        if (dram)
            dram->drainRanks();

        return DrainState::Draining;
    } else {
        return DrainState::Drained;
    }
}

void
DcacheCtrl::drainResume()
{
    if (!isTimingMode && system()->isTimingMode()) {
        // if we switched to timing mode, kick things into action,
        // and behave as if we restored from a checkpoint
        startup();
        dram->startup();
    } else if (isTimingMode && !system()->isTimingMode()) {
        // if we switch from timing mode, stop the refresh events to
        // not cause issues with KVM
        if (dram)
            dram->suspend();
    }

    // update the mode
    isTimingMode = system()->isTimingMode();
}

DcacheCtrl::MemoryPort::MemoryPort(const std::string& name, DcacheCtrl& _ctrl)
    : QueuedResponsePort(name, &_ctrl, queue), queue(_ctrl, *this, true),
      ctrl(_ctrl)
{ }

AddrRangeList
DcacheCtrl::MemoryPort::getAddrRanges() const
{
    AddrRangeList ranges;
    if (ctrl.dram) {
        DPRINTF(DRAM, "Pushing DRAM ranges to port\n");
        ranges.push_back(ctrl.dram->getAddrRange());
    }
    if (ctrl.nvm) {
        DPRINTF(NVM, "Pushing NVM ranges to port\n");
        ranges.push_back(ctrl.nvm->getAddrRange());
    }
    return ranges;
}

void
DcacheCtrl::MemoryPort::recvFunctional(PacketPtr pkt)
{
    pkt->pushLabel(ctrl.name());

    if (!queue.trySatisfyFunctional(pkt)) {
        // Default implementation of SimpleTimingPort::recvFunctional()
        // calls recvAtomic() and throws away the latency; we can save a
        // little here by just not calculating the latency.
        ctrl.recvFunctional(pkt);
    }

    pkt->popLabel();
}

Tick
DcacheCtrl::MemoryPort::recvAtomic(PacketPtr pkt)
{
    return ctrl.recvAtomic(pkt);
}

Tick
DcacheCtrl::MemoryPort::recvAtomicBackdoor(
        PacketPtr pkt, MemBackdoorPtr &backdoor)
{
    return ctrl.recvAtomicBackdoor(pkt, backdoor);
}

bool
DcacheCtrl::MemoryPort::recvTimingReq(PacketPtr pkt)
{
    // pass it to the memory controller
    return ctrl.recvTimingReq(pkt);
}
