#include "mem/policy_manager.hh"

#include "base/trace.hh"
#include "debug/PolicyManager.hh"
#include "debug/Drain.hh"
#include "mem/dram_interface.hh"
#include "sim/sim_exit.hh"
#include "sim/system.hh"

namespace gem5
{

namespace memory
{

PolicyManager::PolicyManager(const PolicyManagerParams &p):
    AbstractMemory(p),
    port(name() + ".port", *this),
    locReqPort(name() + ".loc_req_port", *this),
    farReqPort(name() + ".far_req_port", *this),
    locBurstSize(p.loc_burst_size),
    farBurstSize(p.far_burst_size),
    locMemPolicy(p.loc_mem_policy),
    locMem(p.loc_mem),
    dramCacheSize(p.dram_cache_size),
    blockSize(p.block_size),
    addrSize(p.addr_size),
    orbMaxSize(p.orb_max_size), orbSize(0),
    crbMaxSize(p.crb_max_size), crbSize(0),
    extreme(p.extreme),
    alwaysHit(p.always_hit), alwaysDirty(p.always_dirty),
    bypassDcache(p.bypass_dcache),
    frontendLatency(p.static_frontend_latency),
    backendLatency(p.static_backend_latency),
    tRP(p.tRP),
    tRCD_RD(p.tRCD_RD),
    tRL(p.tRL),
    numColdMisses(0),
    cacheWarmupRatio(p.cache_warmup_ratio),
    resetStatsWarmup(false),
    prevArrival(0),
    retryLLC(false), retryLLCFarMemWr(false),
    retryTagCheck(false), retryLocMemRead(false), retryFarMemRead(false),
    retryLocMemWrite(false), retryFarMemWrite(false),
    maxConf(0),
    tagCheckEvent([this]{ processTagCheckEvent(); }, name()),
    locMemReadEvent([this]{ processLocMemReadEvent(); }, name()),
    locMemWriteEvent([this]{ processLocMemWriteEvent(); }, name()),
    farMemReadEvent([this]{ processFarMemReadEvent(); }, name()),
    farMemWriteEvent([this]{ processFarMemWriteEvent(); }, name()),
    polManStats(*this)
{
    panic_if(orbMaxSize<8, "ORB maximum size must be at least 8.\n");

    locMem->setPolicyManager(this);

    tagMetadataStore.resize(dramCacheSize/blockSize);

}

Tick
PolicyManager::recvAtomic(PacketPtr pkt)
{
    if (!getAddrRange().contains(pkt->getAddr())) {
        panic("Can't handle address range for packet %s\n", pkt->print());
    }

    DPRINTF(PolicyManager, "recvAtomic: %s %d\n",
                     pkt->cmdString(), pkt->getAddr());

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    // do the actual memory access and turn the packet into a response
    access(pkt);

    if (pkt->hasData()) {
        // this value is not supposed to be accurate, just enough to
        // keep things going, mimic a closed page
        // also this latency can't be 0
        // panic("Can't handle this process --> implement accessLatency() "
        //         "according to your interface. pkt: %s\n", pkt->print());
        return accessLatency();
    }

    return 0;
}

Tick
PolicyManager::recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &backdoor)
{
    Tick latency = recvAtomic(pkt);
    getBackdoor(backdoor);
    return latency;
}

void
PolicyManager::recvFunctional(PacketPtr pkt)
{
    bool found;

    if (getAddrRange().contains(pkt->getAddr())) {
        // rely on the abstract memory
        functionalAccess(pkt);
        found = true;
    } else {
        found = false;
    }

    panic_if(!found, "Can't handle address range for packet %s\n",
             pkt->print());
}

Tick
PolicyManager::accessLatency()
{
    // THIS IS FOR DRAM ONLY!
    return (tRP + tRCD_RD + tRL);
}

bool
PolicyManager::findInORB(Addr addr)
{
    for (const auto& e : ORB) {
        if (e.second->owPkt->getAddr() == addr) {
            return true;
        }
    }

    return false;
}

void
PolicyManager::init()
{
   if (!port.isConnected()) {
        fatal("Policy Manager %s is unconnected!\n", name());
    } else if (!locReqPort.isConnected()) {
        fatal("Policy Manager %s is unconnected!\n", name());
    } else if (!farReqPort.isConnected()) {
        fatal("Policy Manager %s is unconnected!\n", name());
    } else {
        port.sendRangeChange();
        //reqPort.recvRangeChange();
    }
}

bool
PolicyManager::recvTimingReq(PacketPtr pkt)
{
    if (bypassDcache) {
        return farReqPort.sendTimingReq(pkt);
    }

    // This is where we enter from the outside world
    DPRINTF(PolicyManager, "recvTimingReq: request %s addr 0x%x-> %d size %d\n",
            pkt->cmdString(), pkt->getAddr(), pkt->getAddr(), pkt->getSize());
    
    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    panic_if(!(pkt->isRead() || pkt->isWrite()),
             "Should only see read and writes at memory controller\n");
    assert(pkt->getSize() != 0);

    // Calc avg gap between requests
    if (prevArrival != 0) {
        polManStats.totGap += curTick() - prevArrival;
    }
    prevArrival = curTick();

    // Find out how many memory packets a pkt translates to
    // If the burst size is equal or larger than the pkt size, then a pkt
    // translates to only one memory packet. Otherwise, a pkt translates to
    // multiple memory packets

    const Addr base_addr = pkt->getAddr();
    Addr addr = base_addr;
    uint32_t burst_size = locBurstSize;
    unsigned size = std::min((addr | (burst_size - 1)) + 1,
                    base_addr + pkt->getSize()) - addr;

    // check merging for writes
    if (pkt->isWrite()) {

        // polManStats.writePktSize[ceilLog2(size)]++;

        bool merged = isInWriteQueue.find((addr & ~(Addr(locBurstSize - 1)))) !=
            isInWriteQueue.end();
        
        bool mergedInLocMemFB = locMem->checkFwdMrgeInFB(pkt->getAddr());

        assert(!(mergedInLocMemFB && merged));

        if (merged) {

            polManStats.mergedWrBursts++;
            polManStats.mergedWrPolManWB++;

            DPRINTF(PolicyManager, "merged in policy manager write back buffer: %lld\n", pkt->getAddr());

            // farMemCtrl->accessInterface(pkt);

            // sendRespondToRequestor(pkt, frontendLatency);

            // return true;
        } else if (mergedInLocMemFB) {

            polManStats.mergedWrBursts++;
            polManStats.mergedWrLocMemFB++;

            DPRINTF(PolicyManager, "merged in DRAM cache flush buffer: %lld\n", pkt->getAddr());

            // farMemCtrl->accessInterface(pkt);

            // sendRespondToRequestor(pkt, frontendLatency);

            // return true;
        }

    }

    // check forwarding for reads
    bool foundInORB = false;
    bool foundInCRB = false;
    bool foundInFarMemWrite = false;
    bool foundInLocMemFB = false;

    if (pkt->isRead()) {

        if (isInWriteQueue.find(pkt->getAddr()) != isInWriteQueue.end()) {

            if (!ORB.empty()) {
                for (const auto& e : ORB) {

                    // check if the read is subsumed in the write queue
                    // packet we are looking at
                    if (e.second->validEntry &&
                        e.second->owPkt->isWrite() &&
                        e.second->owPkt->getAddr() <= addr &&
                        ((addr + size) <=
                        (e.second->owPkt->getAddr() +
                        e.second->owPkt->getSize()))) {

                        foundInORB = true;

                        polManStats.servicedByWrQ++;

                        polManStats.bytesReadWrQ += burst_size;

                        break;
                    }
                }
            }

            if (!foundInORB && !CRB.empty()) {
                for (const auto& e : CRB) {

                    // check if the read is subsumed in the write queue
                    // packet we are looking at
                    if (e.second->isWrite() &&
                        e.second->getAddr() <= addr &&
                        ((addr + size) <=
                        (e.second->getAddr() + e.second->getSize()))) {

                        foundInCRB = true;

                        polManStats.servicedByWrQ++;

                        polManStats.bytesReadWrQ += burst_size;

                        break;
                    }
                }
            }

            if (!foundInORB && !foundInCRB && !pktFarMemWrite.empty()) {
                for (const auto& e : pktFarMemWrite) {
                    // check if the read is subsumed in the write queue
                    // packet we are looking at
                    if (e.second->getAddr() <= addr &&
                        ((addr + size) <=
                        (e.second->getAddr() +
                         e.second->getSize()))) {

                        foundInFarMemWrite = true;

                        polManStats.servicedByWrQ++;

                        polManStats.bytesReadWrQ += burst_size;

                        break;
                    }
                }
            }
        }

        if (locMem->checkFwdMrgeInFB(pkt->getAddr())) {
            foundInLocMemFB = true;

            polManStats.servicedByFB++;

            polManStats.bytesReadWrQ += burst_size;
        }

        if (foundInORB || foundInCRB || foundInFarMemWrite || foundInLocMemFB) {
            DPRINTF(PolicyManager, "FW: %lld\n", pkt->getAddr());
            
        //     polManStats.readPktSize[ceilLog2(size)]++;

        //     farMemCtrl->accessInterface(pkt);

        //     sendRespondToRequestor(pkt, frontendLatency);

        //     return true;
        }
    }

    // process conflicting requests.
    // conflicts are checked only based on Index of DRAM cache
    if (checkConflictInDramCache(pkt)) {

        polManStats.totNumConf++;

        if (CRB.size()>=crbMaxSize) {

            DPRINTF(PolicyManager, "CRBfull: %lld\n", pkt->getAddr());
            DPRINTF(PolicyManager, "CRBfull: %lld\n", pkt->getAddr());

            polManStats.totNumCRBFull++;

            retryLLC = true;

            if (pkt->isRead()) {
                polManStats.numRdRetry++;
            }
            else {
                polManStats.numWrRetry++;
            }
            return false;
        }

        CRB.push_back(std::make_pair(curTick(), pkt));
        DPRINTF(PolicyManager, "CRB PB: %d: %s\n", pkt->getAddr(), pkt->cmdString());

        DPRINTF(PolicyManager, "CRB PB: %d: %s\n", pkt->getAddr(), pkt->cmdString());

        if (pkt->isWrite()) {
            isInWriteQueue.insert(pkt->getAddr());
        }

        if (CRB.size() > maxConf) {
            maxConf = CRB.size();
            polManStats.maxNumConf = CRB.size();
        }
        return true;
    }
    // check if ORB or FMWB is full and set retry
    if (pktFarMemWrite.size() >= (orbMaxSize / 2)) {

        DPRINTF(PolicyManager, "FMWBfull: %lld\n", pkt->getAddr());
        DPRINTF(PolicyManager, "FMWBfull: %lld\n", pkt->getAddr());

        retryLLCFarMemWr = true;

        if (pkt->isRead()) {
            polManStats.numRdRetry++;
        }
        else {
            polManStats.numWrRetry++;
        }
        return false;
    }

    if (ORB.size() >= orbMaxSize) {

        DPRINTF(PolicyManager, "ORBfull: addr %lld\n", pkt->getAddr());
        DPRINTF(PolicyManager, "ORBfull: %lld\n", pkt->getAddr());

        polManStats.totNumORBFull++;

        retryLLC = true;

        if (pkt->isRead()) {
            polManStats.numRdRetry++;
        }
        else {
            polManStats.numWrRetry++;
        }
        return false;
    }

    // if none of the above cases happens,
    // add it to the ORB
    handleRequestorPkt(pkt);

    if (pkt->isWrite()) {
        isInWriteQueue.insert(pkt->getAddr());
    }

    setNextState(ORB.at(pkt->getAddr()));

    handleNextState(ORB.at(pkt->getAddr()));

    DPRINTF(PolicyManager, "Policy manager accepted packet 0x%x %d\n", pkt->getAddr(), pkt->getAddr());

    return true;
}

void
PolicyManager::processTagCheckEvent()
{
    // sanity check for the chosen packet
    auto orbEntry = ORB.at(pktTagCheck.front());
    assert(orbEntry->validEntry);
    assert(orbEntry->state == tagCheck);
    assert(!orbEntry->issued);

    PacketPtr tagCheckPktPtr;

    if (orbEntry->owPkt->isRead()) {
        tagCheckPktPtr = getPacket(pktTagCheck.front(),
                                blockSize,
                                MemCmd::ReadReq);
    } else {
        assert(orbEntry->owPkt->isWrite());
        tagCheckPktPtr = getPacket(pktTagCheck.front(),
                                blockSize,
                                MemCmd::WriteReq);
    }

    tagCheckPktPtr->isTagCheck = true;
    tagCheckPktPtr->owIsRead = orbEntry->owPkt->isRead();
    tagCheckPktPtr->isHit = orbEntry->isHit;
    tagCheckPktPtr->isDirty = orbEntry->prevDirty;
    assert(!tagCheckPktPtr->hasDirtyData);
    tagCheckPktPtr->dirtyLineAddr = orbEntry->dirtyLineAddr;

    if (extreme) {
        tagCheckPktPtr->isDirty = alwaysDirty;
        tagCheckPktPtr->isHit = alwaysHit;
    }

    if (tagCheckPktPtr->owIsRead && !tagCheckPktPtr->isHit && !tagCheckPktPtr->isDirty) {
        assert(tagCheckPktPtr->dirtyLineAddr == -1);
    }

    if (tagCheckPktPtr->owIsRead && !tagCheckPktPtr->isHit && tagCheckPktPtr->isDirty) {
        assert(tagCheckPktPtr->dirtyLineAddr != -1);
    }

    if (locReqPort.sendTimingReq(tagCheckPktPtr)) {
        DPRINTF(PolicyManager, "tag check req sent for adr: %lld\n", tagCheckPktPtr->getAddr());
        orbEntry->state = waitingTCtag;
        orbEntry->issued = true;
        orbEntry->tagCheckIssued = curTick();
        pktTagCheck.pop_front();
        polManStats.sentTagCheckPort++;
    } else {
        DPRINTF(PolicyManager, "sending tag check failed for adr: %lld\n", tagCheckPktPtr->getAddr());
        retryTagCheck = true;
        delete tagCheckPktPtr;
        polManStats.failedTagCheckPort++;
    }

    if (!pktTagCheck.empty() && !tagCheckEvent.scheduled() && !retryTagCheck) {
        schedule(tagCheckEvent, curTick()+1000);
    }
}

void
PolicyManager::processLocMemReadEvent()
{
    // sanity check for the chosen packet
    auto orbEntry = ORB.at(pktLocMemRead.front());
    assert(orbEntry->validEntry);
    assert(orbEntry->state == locMemRead);
    assert(!orbEntry->issued);

    PacketPtr rdLocMemPkt = getPacket(pktLocMemRead.front(),
                                   blockSize,
                                   MemCmd::ReadReq);

    if (locReqPort.sendTimingReq(rdLocMemPkt)) {
        DPRINTF(PolicyManager, "loc mem read is sent : %lld\n", rdLocMemPkt->getAddr());
        orbEntry->state = waitingLocMemReadResp;
        orbEntry->issued = true;
        orbEntry->locRdIssued = curTick();
        pktLocMemRead.pop_front();
        polManStats.sentLocRdPort++;
    } else {
        DPRINTF(PolicyManager, "loc mem read sending failed: %lld\n", rdLocMemPkt->getAddr());
        retryLocMemRead = true;
        delete rdLocMemPkt;
        polManStats.failedLocRdPort++;
    }

    if (!pktLocMemRead.empty() && !locMemReadEvent.scheduled() && !retryLocMemRead) {
        schedule(locMemReadEvent, curTick()+1000);
    }
}

void
PolicyManager::processLocMemWriteEvent()
{
    // sanity check for the chosen packet
    auto orbEntry = ORB.at(pktLocMemWrite.front());
    assert(orbEntry->validEntry);
    assert(orbEntry->state == locMemWrite);
    assert(!orbEntry->issued);

    PacketPtr wrLocMemPkt = getPacket(pktLocMemWrite.front(),
                                   blockSize,
                                   MemCmd::WriteReq);
    assert(!wrLocMemPkt->isTagCheck);

    if (locReqPort.sendTimingReq(wrLocMemPkt)) {
        DPRINTF(PolicyManager, "loc mem write is sent : %lld\n", wrLocMemPkt->getAddr());
        orbEntry->state = waitingLocMemWriteResp;
        orbEntry->issued = true;
        orbEntry->locWrIssued = curTick();
        pktLocMemWrite.pop_front();
        polManStats.sentLocWrPort++;
    } else {
        DPRINTF(PolicyManager, "loc mem write sending failed: %lld\n", wrLocMemPkt->getAddr());
        retryLocMemWrite = true;
        delete wrLocMemPkt;
        polManStats.failedLocWrPort++;
    }

    if (!pktLocMemWrite.empty() && !locMemWriteEvent.scheduled() && !retryLocMemWrite) {
        schedule(locMemWriteEvent, curTick()+1000);
    }
}

void
PolicyManager::processFarMemReadEvent()
{
    // sanity check for the chosen packet
    auto orbEntry = ORB.at(pktFarMemRead.front());
    assert(orbEntry->validEntry);
    assert(orbEntry->state == farMemRead);
    assert(!orbEntry->issued);

    PacketPtr rdFarMemPkt = getPacket(pktFarMemRead.front(),
                                      blockSize,
                                      MemCmd::ReadReq);

    if (farReqPort.sendTimingReq(rdFarMemPkt)) {
        DPRINTF(PolicyManager, "far mem read is sent : %lld\n", rdFarMemPkt->getAddr());
        orbEntry->state = waitingFarMemReadResp;
        orbEntry->issued = true;
        orbEntry->farRdIssued = curTick();
        pktFarMemRead.pop_front();
        polManStats.sentFarRdPort++;
    } else {
        DPRINTF(PolicyManager, "far mem read sending failed: %lld\n", rdFarMemPkt->getAddr());
        retryFarMemRead = true;
        delete rdFarMemPkt;
        polManStats.failedFarRdPort++;
    }

    if (!pktFarMemRead.empty() && !farMemReadEvent.scheduled() && !retryFarMemRead) {
        schedule(farMemReadEvent, curTick()+1000);
    }
}

void
PolicyManager::processFarMemWriteEvent()
{
    PacketPtr wrFarMemPkt = getPacket(pktFarMemWrite.front().second->getAddr(),
                                      blockSize,
                                      MemCmd::WriteReq);
    DPRINTF(PolicyManager, "FarMemWriteEvent: request %s addr %#x\n",
            wrFarMemPkt->cmdString(), wrFarMemPkt->getAddr());

    if (farReqPort.sendTimingReq(wrFarMemPkt)) {
        DPRINTF(PolicyManager, "far mem write is sent : %lld\n", wrFarMemPkt->getAddr());
        pktFarMemWrite.pop_front();
        polManStats.sentFarWrPort++;
    } else {
        DPRINTF(PolicyManager, "far mem write sending failed: %lld\n", wrFarMemPkt->getAddr());
        retryFarMemWrite = true;
        delete wrFarMemPkt;
        polManStats.failedFarWrPort++;
    }

    if (!pktFarMemWrite.empty() && !farMemWriteEvent.scheduled() && !retryFarMemWrite) {
        schedule(farMemWriteEvent, curTick()+1000);
    } else {
        if (drainState() == DrainState::Draining && pktFarMemWrite.empty() &&
            ORB.empty()) {
            DPRINTF(Drain, "PolicyManager done draining in farMemWrite\n");
            signalDrainDone();
        }
    }

    if (retryLLCFarMemWr && pktFarMemWrite.size()< (orbMaxSize / 2)) {

        DPRINTF(PolicyManager, "retryLLCFarMemWr sent\n");

        retryLLCFarMemWr = false;

        port.sendRetryReq();
    }
}

bool
PolicyManager::locMemRecvTimingResp(PacketPtr pkt)
{
    DPRINTF(PolicyManager, "locMemRecvTimingResp : %d: %s\n", pkt->getAddr(), pkt->cmdString());

    if (locMemPolicy == enums::Rambus && !pkt->isTagCheck && pkt->hasDirtyData) {
        DPRINTF(PolicyManager, "locMemRecvTimingResp: rd miss data async %d:\n", pkt->getAddr());
        assert(pkt->owIsRead);
        assert(!pkt->isHit);
        handleDirtyCacheLine(pkt->dirtyLineAddr);
        delete pkt;
        return true;
    }

    if (!findInORB(pkt->getAddr())) {
        std::cout << "!findInORB: " << pkt->getAddr() << " / " << pkt->cmdString() << "\n";
        std::cout << "+++++++++++++++++++++\n+++++++++++++++++++++\n+++++++++++++++++++++\n";
    }
    
    auto orbEntry = ORB.at(pkt->getAddr());

    if(pkt->isTagCheck) {

        assert(orbEntry->pol == enums::Rambus);
        assert(orbEntry->state == waitingTCtag);

        if (pkt->hasDirtyData) {
            assert(orbEntry->owPkt->isRead());
            assert(!orbEntry->isHit);
            if (!orbEntry->prevDirty) { // clean
                assert(orbEntry->dirtyLineAddr == -1);
                assert(!orbEntry->handleDirtyLine);
                orbEntry->handleDirtyLine = true;
                orbEntry->dirtyLineAddr = pkt->dirtyLineAddr;
            } else { // dirty
                assert(orbEntry->dirtyLineAddr != -1);
                assert(orbEntry->handleDirtyLine);
            }
        }

        // Rd Miss Dirty
        if (orbEntry->owPkt->isRead() && !orbEntry->isHit && orbEntry->prevDirty) {
            assert(pkt->hasDirtyData);
            assert(orbEntry->handleDirtyLine);
            assert(orbEntry->dirtyLineAddr != -1);
        }

        // if (!(orbEntry->owPkt->isRead() && orbEntry->isHit)) {
        //     orbEntry->tagCheckExit = curTick();
        // }
        orbEntry->tagCheckExit = curTick();

        if (orbEntry->owPkt->isRead() && orbEntry->isHit) {
            orbEntry->state = waitingLocMemReadResp;
        }
        
    } else {
        if (pkt->isRead()) {

            if (orbEntry->pol == enums::CascadeLakeNoPartWrs || 
                orbEntry->pol == enums::Oracle ||
                orbEntry->pol ==  enums::BearWriteOpt)
            {
                    assert(orbEntry->state == waitingLocMemReadResp);
                    if (orbEntry->handleDirtyLine) {
                        assert(!orbEntry->isHit);
                        handleDirtyCacheLine(orbEntry->dirtyLineAddr);
                    }
                    orbEntry->locRdExit = curTick();
            }

            if (orbEntry->pol == enums::Rambus) {
                assert(orbEntry->state == waitingLocMemReadResp);
                assert(orbEntry->isHit);
                assert(!pkt->hasDirtyData);
                assert(orbEntry->dirtyLineAddr == -1);
                orbEntry->locRdExit = curTick();
                orbEntry->state = locRdRespReady;

                // else {
                //     // just a null data, which resembles the bubble on the data bus for this case in Rambus-baseline
                //     assert(pkt->owIsRead && !pkt->isHit && !pkt->isDirty && !pkt->hasDirtyData);
                //     return true;
                // }
            }

        }
        else {
            assert(pkt->isWrite());

            if (orbEntry->pol == enums::CascadeLakeNoPartWrs || 
                orbEntry->pol == enums::Oracle ||
                orbEntry->pol ==  enums::BearWriteOpt)
            {
                assert(orbEntry->state == waitingLocMemWriteResp);
                orbEntry->locWrExit = curTick();
            }

            if (orbEntry->pol == enums::Rambus) {
                if (orbEntry->state == waitingLocMemWriteResp) {
                    assert(orbEntry->owPkt->isRead());
                    assert(!orbEntry->isHit);
                    orbEntry->locWrExit = curTick();
                }
                if (orbEntry->state == waitingTCtag) {
                    assert(orbEntry->owPkt->isWrite());
                    orbEntry->tagCheckExit = curTick();
                }
            }


        }
    }

    // IMPORTANT:
    // orbEntry should not be used as the passed argument in setNextState and
    // handleNextState functions, reason: it's possible that orbEntry may be
    // deleted and updated, which will not be reflected here in the scope of
    // current lines since it's been read at line #475.
    setNextState(ORB.at(pkt->getAddr()));

    handleNextState(ORB.at(pkt->getAddr()));

    delete pkt;

    return true;
}

bool
PolicyManager::farMemRecvTimingResp(PacketPtr pkt)
{
    if (bypassDcache) {
        port.schedTimingResp(pkt, curTick());
        return true;
    }

    DPRINTF(PolicyManager, "farMemRecvTimingResp : %lld , %s \n", pkt->getAddr(), pkt->cmdString());

    if (pkt->isRead()) {

        auto orbEntry = ORB.at(pkt->getAddr());

        DPRINTF(PolicyManager, "farMemRecvTimingResp : continuing to far read resp: %d\n",
        orbEntry->owPkt->isRead());

        assert(orbEntry->state == waitingFarMemReadResp);

        orbEntry->farRdExit = curTick();

        // IMPORTANT:
        // orbEntry should not be used as the passed argument in setNextState and
        // handleNextState functions, reason: it's possible that orbEntry may be
        // deleted and updated, which will not be reflected here in the scope of
        // current lines since it's been read at line #508.
        setNextState(ORB.at(pkt->getAddr()));

        // The next line is absolutely required since the orbEntry will
        // be deleted and renewed within setNextState()
        // orbEntry = ORB.at(pkt->getAddr());

        handleNextState(ORB.at(pkt->getAddr()));

        delete pkt;
    }
    else {
        assert(pkt->isWrite());
        delete pkt;
    }

    return true;
}

void
PolicyManager::locMemRecvReqRetry()
{
    // assert(retryLocMemRead || retryLocMemWrite);
    bool schedTC = false;
    bool schedRd = false;
    bool schedWr = false;

    if (retryTagCheck) {

        if (!tagCheckEvent.scheduled() && !pktTagCheck.empty()) {
            assert(locMemPolicy == enums::Rambus);
            schedule(tagCheckEvent, curTick());
        }
        retryTagCheck = false;
        schedTC = true;
    }

    if (retryLocMemRead) {

        if (!locMemReadEvent.scheduled() && !pktLocMemRead.empty()) {
            schedule(locMemReadEvent, curTick());
        }
        retryLocMemRead = false;
        schedRd = true;
    }

    if (retryLocMemWrite) {
        if (!locMemWriteEvent.scheduled() && !pktLocMemWrite.empty()) {
            schedule(locMemWriteEvent, curTick());
        }
        retryLocMemWrite = false;
        schedWr = true;
    }
    if (!schedTC && !schedRd && !schedWr) {
            // panic("Wrong local mem retry event happend.\n");

            // TODO: there are cases where none of retryLocMemRead and retryLocMemWrite
            // are true, yet locMemRecvReqRetry() is called. I should fix this later.
            if (locMemPolicy == enums::Rambus && !tagCheckEvent.scheduled() && !pktTagCheck.empty()) {
                schedule(tagCheckEvent, curTick());
            }
            if (!locMemReadEvent.scheduled() && !pktLocMemRead.empty()) {
                schedule(locMemReadEvent, curTick());
            }
            if (!locMemWriteEvent.scheduled() && !pktLocMemWrite.empty()) {
                schedule(locMemWriteEvent, curTick());
            }
    }

    DPRINTF(PolicyManager, "locMemRecvReqRetry: %d, %d , %d \n", schedTC, schedRd, schedWr);
}

void
PolicyManager::farMemRecvReqRetry()
{
    if (bypassDcache) {
        port.sendRetryReq();
        return;
    }

    // assert(retryFarMemRead || retryFarMemWrite);

    bool schedRd = false;
    bool schedWr = false;

    if (retryFarMemRead) {
        if (!farMemReadEvent.scheduled() && !pktFarMemRead.empty()) {
            schedule(farMemReadEvent, curTick());
        }
        retryFarMemRead = false;
        schedRd = true;
    }
    if (retryFarMemWrite) {
        if (!farMemWriteEvent.scheduled() && !pktFarMemWrite.empty()) {
            schedule(farMemWriteEvent, curTick());
        }
        retryFarMemWrite = false;
        schedWr = true;
    }
    // else {
    //     panic("Wrong far mem retry event happend.\n");
    // }
    if (!schedRd && !schedWr) {
        // panic("Wrong local mem retry event happend.\n");

        // TODO: there are cases where none of retryFarMemRead and retryFarMemWrite
        // are true, yet farMemRecvReqRetry() is called. I should fix this later.
        if (!farMemReadEvent.scheduled() && !pktFarMemRead.empty()) {
            schedule(farMemReadEvent, curTick());
        }
        if (!farMemWriteEvent.scheduled() && !pktFarMemWrite.empty()) {
            schedule(farMemWriteEvent, curTick());
        }
    }

    DPRINTF(PolicyManager, "farMemRecvReqRetry: %d , %d \n", schedRd, schedWr);
}

void
PolicyManager::setNextState(reqBufferEntry* orbEntry)
{
    orbEntry->issued = false;
    enums::Policy pol = orbEntry->pol;
    reqState state = orbEntry->state;
    bool isRead = orbEntry->owPkt->isRead();
    bool isHit = orbEntry->isHit;
    //This must be checked for the first 3 policies, later.
    // bool isDirty = checkDirty(orbEntry->owPkt->getAddr());
    bool isDirty = orbEntry->prevDirty;

    ///////////////////////////////////////////////////////////////////////////////////////
    // CascadeLakeNoPartWrs

    // start --> read tag
    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->state == start) {
            orbEntry->state = locMemRead;
            orbEntry->locRdEntered = curTick();
            return;
    }

    // tag ready && read && hit --> DONE
    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingLocMemReadResp &&
        orbEntry->isHit) {
            // done
            // do nothing
            return;
    }

    // tag ready && write --> loc write
    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->owPkt->isWrite() &&
        orbEntry->state == waitingLocMemReadResp) {
            // write it to the DRAM cache
            orbEntry->state = locMemWrite;
            orbEntry->locWrEntered = curTick();
            return;
    }

    // loc read resp ready && read && miss --> far read
    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingLocMemReadResp &&
        !orbEntry->isHit) {

            orbEntry->state = farMemRead;
            orbEntry->farRdEntered = curTick();
            return;
    }

    // far read resp ready && read && miss --> loc write
    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingFarMemReadResp &&
        !orbEntry->isHit) {

            PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
                                             false,
                                             orbEntry->owPkt->isRead());

            accessAndRespond(orbEntry->owPkt,
                             frontendLatency + backendLatency + backendLatency);

            ORB.at(copyOwPkt->getAddr()) = new reqBufferEntry(
                                                orbEntry->validEntry,
                                                orbEntry->arrivalTick,
                                                orbEntry->tagDC,
                                                orbEntry->indexDC,
                                                copyOwPkt,
                                                orbEntry->pol,
                                                orbEntry->state,
                                                orbEntry->issued,
                                                orbEntry->isHit,
                                                orbEntry->conflict,
                                                orbEntry->dirtyLineAddr,
                                                orbEntry->handleDirtyLine,
                                                orbEntry->tagCheckEntered,
                                                orbEntry->tagCheckIssued,
                                                orbEntry->tagCheckExit,
                                                orbEntry->locRdEntered,
                                                orbEntry->locRdIssued,
                                                orbEntry->locRdExit,
                                                orbEntry->locWrEntered,
                                                orbEntry->locWrIssued,
                                                orbEntry->locWrExit,
                                                orbEntry->farRdEntered,
                                                orbEntry->farRdIssued,
                                                orbEntry->farRdExit);
            delete orbEntry;

            orbEntry = ORB.at(copyOwPkt->getAddr());

            polManStats.totPktRespTime += ((curTick() - orbEntry->arrivalTick)/1000);
            polManStats.totPktRespTimeRd += ((curTick() - orbEntry->arrivalTick)/1000);
            
            orbEntry->state = locMemWrite;

            orbEntry->locWrEntered = curTick();

            return;
    }

    // loc write received
    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        // orbEntry->owPkt->isRead() &&
        // !orbEntry->isHit &&
        orbEntry->state == waitingLocMemWriteResp) {
            // done
            // do nothing
            return;
    }

    ////////////////////////////////////////////////////////////////////////
    /// Oracle

    // RD Hit Dirty & Clean, RD Miss Dirty, WR Miss Dirty
    // start --> read loc
    if (pol == enums::Oracle && state == start &&
        ((isRead && isHit) || (isRead && !isHit && isDirty) || (!isRead && !isHit && isDirty)) 
       ) {
            orbEntry->state = locMemRead;
            orbEntry->locRdEntered = curTick();
            return;
    }
    // RD Miss Clean
    // start --> read far
    if (pol == enums::Oracle && state == start &&
        (isRead && !isHit && !isDirty)
       ) {
            orbEntry->state = farMemRead;
            orbEntry->farRdEntered = curTick();
            return;
    }
    // WR Hit Dirty & Clean, WR Miss Clean
    // start --> write loc
    if (pol == enums::Oracle && state == start &&
        ((!isRead && isHit)|| (!isRead && !isHit && !isDirty)) 
       ) {
            orbEntry->state = locMemWrite;
            orbEntry->locWrEntered = curTick();
            return;
    }

    // RD Hit Dirty & Clean
    // start --> read loc --> done
    if (pol == enums::Oracle &&
        isRead && isHit &&
        state == waitingLocMemReadResp ) {
            // done
            // do nothing
            return;
    }

    // RD Miss Dirty:
    // start --> read loc --> read far
    if (pol == enums::Oracle &&
        isRead && !isHit && isDirty &&
        state == waitingLocMemReadResp ) {
            orbEntry->state = farMemRead;
            orbEntry->farRdEntered = curTick();
            return;
    }

    // WR Miss Dirty:
    // start --> read loc --> loc write
    if (pol == enums::Oracle &&
        !isRead && !isHit && isDirty &&
        state == waitingLocMemReadResp) {
            // write it to the DRAM cache
            orbEntry->state = locMemWrite;
            orbEntry->locWrEntered = curTick();
            return;
    }

    // RD Miss Clean & Dirty
    // start --> ... --> far read -> loc write
    if (pol == enums::Oracle && 
        (isRead && !isHit) &&
        state == waitingFarMemReadResp
       ) {
            PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
                                             false,
                                             orbEntry->owPkt->isRead());

            accessAndRespond(orbEntry->owPkt,
                             frontendLatency + backendLatency + backendLatency);

            ORB.at(copyOwPkt->getAddr()) = new reqBufferEntry(
                                                orbEntry->validEntry,
                                                orbEntry->arrivalTick,
                                                orbEntry->tagDC,
                                                orbEntry->indexDC,
                                                copyOwPkt,
                                                orbEntry->pol,
                                                orbEntry->state,
                                                orbEntry->issued,
                                                orbEntry->isHit,
                                                orbEntry->conflict,
                                                orbEntry->dirtyLineAddr,
                                                orbEntry->handleDirtyLine,
                                                orbEntry->tagCheckEntered,
                                                orbEntry->tagCheckIssued,
                                                orbEntry->tagCheckExit,
                                                orbEntry->locRdEntered,
                                                orbEntry->locRdIssued,
                                                orbEntry->locRdExit,
                                                orbEntry->locWrEntered,
                                                orbEntry->locWrIssued,
                                                orbEntry->locWrExit,
                                                orbEntry->farRdEntered,
                                                orbEntry->farRdIssued,
                                                orbEntry->farRdExit);
            delete orbEntry;

            orbEntry = ORB.at(copyOwPkt->getAddr());

            polManStats.totPktRespTime += ((curTick() - orbEntry->arrivalTick)/1000);
            polManStats.totPktRespTimeRd += ((curTick() - orbEntry->arrivalTick)/1000);

            orbEntry->state = locMemWrite;

            orbEntry->locWrEntered = curTick();

            return;
    }

    // loc write received
    if (pol == enums::Oracle &&
        state == waitingLocMemWriteResp) {
            assert (!(isRead && isHit));
            // done
            // do nothing
            return;
    }

    ////////////////////////////////////////////////////////////////////////
    // BEAR Write optimized
    if (orbEntry->pol == enums::BearWriteOpt &&
        orbEntry->state == start && !(orbEntry->owPkt->isWrite() && orbEntry->isHit)) {
            orbEntry->state = locMemRead;
            orbEntry->locRdEntered = curTick();
            DPRINTF(PolicyManager, "set: start -> locMemRead : %d\n", orbEntry->owPkt->getAddr());
            return;
    }

    if (orbEntry->pol == enums::BearWriteOpt &&
        orbEntry->state == start && orbEntry->owPkt->isWrite() && orbEntry->isHit) {
            orbEntry->state = locMemWrite;
            orbEntry->locRdEntered = curTick();
            DPRINTF(PolicyManager, "set: start -> locMemWrite : %d\n", orbEntry->owPkt->getAddr());
            return;
    }

    // tag ready && read && hit --> DONE
    if (orbEntry->pol == enums::BearWriteOpt &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingLocMemReadResp &&
        orbEntry->isHit) {
            DPRINTF(PolicyManager, "set: waitingLocMemReadResp -> NONE : %d\n", orbEntry->owPkt->getAddr());
            
            // done
            // do nothing
            return;
    }

    // tag ready && write --> loc write
    if (orbEntry->pol == enums::BearWriteOpt &&
        orbEntry->owPkt->isWrite() &&
        orbEntry->state == waitingLocMemReadResp) {
            assert(!orbEntry->isHit);
            // write it to the DRAM cache
            orbEntry->state = locMemWrite;
            orbEntry->locWrEntered = curTick();
            DPRINTF(PolicyManager, "set: waitingLocMemReadResp -> locMemWrite : %d\n", orbEntry->owPkt->getAddr());
            return;
    }

    // loc read resp ready && read && miss --> far read
    if (orbEntry->pol == enums::BearWriteOpt &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingLocMemReadResp &&
        !orbEntry->isHit) {
            
            orbEntry->state = farMemRead;
            orbEntry->farRdEntered = curTick();
            DPRINTF(PolicyManager, "set: waitingLocMemReadResp -> farMemRead : %d\n", orbEntry->owPkt->getAddr());
            return;
    }

    // far read resp ready && read && miss --> loc write
    if (orbEntry->pol == enums::BearWriteOpt &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingFarMemReadResp &&
        !orbEntry->isHit) {
            
            PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
                                             false,
                                             orbEntry->owPkt->isRead());

            accessAndRespond(orbEntry->owPkt,
                             frontendLatency + backendLatency + backendLatency);

            ORB.at(copyOwPkt->getAddr()) = new reqBufferEntry(
                                                orbEntry->validEntry,
                                                orbEntry->arrivalTick,
                                                orbEntry->tagDC,
                                                orbEntry->indexDC,
                                                copyOwPkt,
                                                orbEntry->pol,
                                                orbEntry->state,
                                                orbEntry->issued,
                                                orbEntry->isHit,
                                                orbEntry->conflict,
                                                orbEntry->dirtyLineAddr,
                                                orbEntry->handleDirtyLine,
                                                orbEntry->tagCheckEntered,
                                                orbEntry->tagCheckIssued,
                                                orbEntry->tagCheckExit,
                                                orbEntry->locRdEntered,
                                                orbEntry->locRdIssued,
                                                orbEntry->locRdExit,
                                                orbEntry->locWrEntered,
                                                orbEntry->locWrIssued,
                                                orbEntry->locWrExit,
                                                orbEntry->farRdEntered,
                                                orbEntry->farRdIssued,
                                                orbEntry->farRdExit);
            delete orbEntry;

            orbEntry = ORB.at(copyOwPkt->getAddr());

            polManStats.totPktRespTime += ((curTick() - orbEntry->arrivalTick)/1000);
            polManStats.totPktRespTimeRd += ((curTick() - orbEntry->arrivalTick)/1000);

            orbEntry->state = locMemWrite;

            orbEntry->locWrEntered = curTick();

            DPRINTF(PolicyManager, "set: waitingFarMemReadResp -> locMemWrite : %d\n", orbEntry->owPkt->getAddr());

            return;
    }

    // loc write received
    if (orbEntry->pol == enums::BearWriteOpt &&
        // orbEntry->owPkt->isRead() &&
        // !orbEntry->isHit &&
        orbEntry->state == waitingLocMemWriteResp) {
            DPRINTF(PolicyManager, "set: waitingLocMemWriteResp -> NONE : %d\n", orbEntry->owPkt->getAddr());
            
            // done
            // do nothing

            return;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Rambus
    // start --> read tag
    if (orbEntry->pol == enums::Rambus &&
        orbEntry->state == start) {
            orbEntry->state = tagCheck;
            orbEntry->tagCheckEntered = curTick();
            return;
    }

    // tag ready
    // read && hit --> wait for data
    if (orbEntry->pol == enums::Rambus &&
        orbEntry->state == waitingTCtag &&
        orbEntry->owPkt->isRead() && orbEntry->isHit) {
            // orbEntry->state = waitingLocMemReadResp;
            // do nothing
            return;
    }

    // tag ready
    // read && miss --> don't wait for dirty data (MC with FB>0/MD), transition to far read
    if (orbEntry->pol == enums::Rambus &&
        orbEntry->state == waitingTCtag &&
        orbEntry->owPkt->isRead() &&
        !orbEntry->isHit) {
            orbEntry->state = farMemRead;
            orbEntry->farRdEntered = curTick();
            return;
    }

    // tag ready
    // write --> done
    if (orbEntry->pol == enums::Rambus &&
        orbEntry->state == waitingTCtag &&
        orbEntry->owPkt->isWrite()) {
            // done, do nothing and return;
            return;
    }

    // tag ready && read && hit --> DONE
    if (orbEntry->pol == enums::Rambus &&
        orbEntry->state == waitingLocMemReadResp) {
            assert(orbEntry->isHit);
            assert(orbEntry->owPkt->isRead());
            // done
            // do nothing
            return;
    }

    // tag ready && read && hit --> DONE
    if (orbEntry->pol == enums::Rambus &&
        orbEntry->state == locRdRespReady) {
            assert(orbEntry->isHit);
            assert(orbEntry->owPkt->isRead());
            // done
            // do nothing
            return;
    }

    // far read resp ready && read && miss --> loc write
    if (orbEntry->pol == enums::Rambus &&
        orbEntry->state == waitingFarMemReadResp) {

            assert(orbEntry->owPkt->isRead());
            assert(!orbEntry->isHit);

            PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
                                             false,
                                             orbEntry->owPkt->isRead());

            accessAndRespond(orbEntry->owPkt,
                             frontendLatency + backendLatency + backendLatency);

            ORB.at(copyOwPkt->getAddr()) = new reqBufferEntry(
                                                orbEntry->validEntry,
                                                orbEntry->arrivalTick,
                                                orbEntry->tagDC,
                                                orbEntry->indexDC,
                                                copyOwPkt,
                                                orbEntry->pol,
                                                orbEntry->state,
                                                orbEntry->issued,
                                                orbEntry->isHit,
                                                orbEntry->conflict,
                                                orbEntry->dirtyLineAddr,
                                                orbEntry->handleDirtyLine,
                                                orbEntry->tagCheckEntered,
                                                orbEntry->tagCheckIssued,
                                                orbEntry->tagCheckExit,
                                                orbEntry->locRdEntered,
                                                orbEntry->locRdIssued,
                                                orbEntry->locRdExit,
                                                orbEntry->locWrEntered,
                                                orbEntry->locWrIssued,
                                                orbEntry->locWrExit,
                                                orbEntry->farRdEntered,
                                                orbEntry->farRdIssued,
                                                orbEntry->farRdExit);
            delete orbEntry;

            orbEntry = ORB.at(copyOwPkt->getAddr());

            polManStats.totPktRespTime += ((curTick() - orbEntry->arrivalTick)/1000);
            polManStats.totPktRespTimeRd += ((curTick() - orbEntry->arrivalTick)/1000);

            orbEntry->state = locMemWrite;

            orbEntry->locWrEntered = curTick();

            return;
    }

    // loc write received
    if (orbEntry->pol == enums::Rambus &&
        orbEntry->state == waitingLocMemWriteResp) {
            assert(orbEntry->owPkt->isRead());
            assert(!orbEntry->isHit);
            // done
            // do nothing
            return;
    }
}

void
PolicyManager::handleNextState(reqBufferEntry* orbEntry)
{
    ////////////////////////////////////////////////////////////////////////
    // CascadeLakeNoPartWrs

    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->state == locMemRead) {

        // assert(!pktLocMemRead.empty());

        pktLocMemRead.push_back(orbEntry->owPkt->getAddr());

        polManStats.avgLocRdQLenEnq = pktLocMemRead.size();

        if (!locMemReadEvent.scheduled() && !retryLocMemRead) {
            schedule(locMemReadEvent, curTick());
        }
        return;
    }

    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingLocMemReadResp &&
        orbEntry->isHit) {
            // DONE
            // send the respond to the requestor

            PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
                                             false,
                                             orbEntry->owPkt->isRead());

            accessAndRespond(orbEntry->owPkt,
                             frontendLatency + backendLatency);

            ORB.at(copyOwPkt->getAddr()) = new reqBufferEntry(
                                                orbEntry->validEntry,
                                                orbEntry->arrivalTick,
                                                orbEntry->tagDC,
                                                orbEntry->indexDC,
                                                copyOwPkt,
                                                orbEntry->pol,
                                                orbEntry->state,
                                                orbEntry->issued,
                                                orbEntry->isHit,
                                                orbEntry->conflict,
                                                orbEntry->dirtyLineAddr,
                                                orbEntry->handleDirtyLine,
                                                orbEntry->tagCheckEntered,
                                                orbEntry->tagCheckIssued,
                                                orbEntry->tagCheckExit,
                                                orbEntry->locRdEntered,
                                                orbEntry->locRdIssued,
                                                orbEntry->locRdExit,
                                                orbEntry->locWrEntered,
                                                orbEntry->locWrIssued,
                                                orbEntry->locWrExit,
                                                orbEntry->farRdEntered,
                                                orbEntry->farRdIssued,
                                                orbEntry->farRdExit);
            delete orbEntry;

            orbEntry = ORB.at(copyOwPkt->getAddr());

            polManStats.totPktRespTime += ((curTick() - orbEntry->arrivalTick)/1000);
            polManStats.totPktRespTimeRd += ((curTick() - orbEntry->arrivalTick)/1000);

            // clear ORB
            resumeConflictingReq(orbEntry);

            return;
    }

    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == farMemRead) {

            assert(!orbEntry->isHit);

            // do a read from far mem
            pktFarMemRead.push_back(orbEntry->owPkt->getAddr());

            polManStats.avgFarRdQLenEnq = pktFarMemRead.size();

            if (!farMemReadEvent.scheduled() && !retryFarMemRead) {
                schedule(farMemReadEvent, curTick());
            }
            return;

    }

    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->state == locMemWrite) {

            if (orbEntry->owPkt->isRead()) {
                assert(!orbEntry->isHit);
            }

            // do a read from far mem
            pktLocMemWrite.push_back(orbEntry->owPkt->getAddr());

            polManStats.avgLocWrQLenEnq = pktLocMemWrite.size();


            if (!locMemWriteEvent.scheduled() && !retryLocMemWrite) {
                schedule(locMemWriteEvent, curTick());
            }
            return;

    }

    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        // orbEntry->owPkt->isRead() &&
        // !orbEntry->isHit &&
        orbEntry->state == waitingLocMemWriteResp) {
            // DONE
            // clear ORB
            resumeConflictingReq(orbEntry);

            return;
    }

    ////////////////////////////////////////////////////////////////////////
    // Oracle
    if (orbEntry->pol == enums::Oracle &&
        orbEntry->state == locMemRead) {

        pktLocMemRead.push_back(orbEntry->owPkt->getAddr());

        polManStats.avgLocRdQLenEnq = pktLocMemRead.size();
            
        if (!locMemReadEvent.scheduled() && !retryLocMemRead) {
            schedule(locMemReadEvent, curTick());
        }
        return;
    }

    if (orbEntry->pol == enums::Oracle &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingLocMemReadResp &&
        orbEntry->isHit) {
            // DONE
            // send the respond to the requestor

            PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
                                             false,
                                             orbEntry->owPkt->isRead());

            accessAndRespond(orbEntry->owPkt,
                             frontendLatency + backendLatency);

            ORB.at(copyOwPkt->getAddr()) = new reqBufferEntry(
                                                orbEntry->validEntry,
                                                orbEntry->arrivalTick,
                                                orbEntry->tagDC,
                                                orbEntry->indexDC,
                                                copyOwPkt,
                                                orbEntry->pol,
                                                orbEntry->state,
                                                orbEntry->issued,
                                                orbEntry->isHit,
                                                orbEntry->conflict,
                                                orbEntry->dirtyLineAddr,
                                                orbEntry->handleDirtyLine,
                                                orbEntry->tagCheckEntered,
                                                orbEntry->tagCheckIssued,
                                                orbEntry->tagCheckExit,
                                                orbEntry->locRdEntered,
                                                orbEntry->locRdIssued,
                                                orbEntry->locRdExit,
                                                orbEntry->locWrEntered,
                                                orbEntry->locWrIssued,
                                                orbEntry->locWrExit,
                                                orbEntry->farRdEntered,
                                                orbEntry->farRdIssued,
                                                orbEntry->farRdExit);
            delete orbEntry;

            orbEntry = ORB.at(copyOwPkt->getAddr());

            polManStats.totPktRespTime += ((curTick() - orbEntry->arrivalTick)/1000);
            polManStats.totPktRespTimeRd += ((curTick() - orbEntry->arrivalTick)/1000);

            // clear ORB
            resumeConflictingReq(orbEntry);

            return;            
    }

    if (orbEntry->pol == enums::Oracle &&
        orbEntry->state == farMemRead) {

            assert(orbEntry->owPkt->isRead() && !orbEntry->isHit);

            // do a read from far mem
            pktFarMemRead.push_back(orbEntry->owPkt->getAddr());

            polManStats.avgFarRdQLenEnq = pktFarMemRead.size();

            if (!farMemReadEvent.scheduled() && !retryFarMemRead) {
                schedule(farMemReadEvent, curTick());
            }
            return;

    }

    if (orbEntry->pol == enums::Oracle &&
        orbEntry->state == locMemWrite) {

            if (orbEntry->owPkt->isRead()) {
                assert(!orbEntry->isHit);
            }

            // do a read from far mem
            pktLocMemWrite.push_back(orbEntry->owPkt->getAddr());

            polManStats.avgLocWrQLenEnq = pktLocMemWrite.size();
            

            if (!locMemWriteEvent.scheduled() && !retryLocMemWrite) {
                schedule(locMemWriteEvent, curTick());
            }
            return;

    }

    if (orbEntry->pol == enums::Oracle &&
        orbEntry->state == waitingLocMemWriteResp) {
            // DONE
            // clear ORB
            resumeConflictingReq(orbEntry);

            return;
    }

    ////////////////////////////////////////////////////////////////////////
    // BEAR Write Optmized
    if (orbEntry->pol == enums::BearWriteOpt &&
        orbEntry->state == locMemRead) {

        pktLocMemRead.push_back(orbEntry->owPkt->getAddr());

        polManStats.avgLocRdQLenEnq = pktLocMemRead.size();

        if (!locMemReadEvent.scheduled() && !retryLocMemRead) {
            schedule(locMemReadEvent, curTick());
        }
        return;
    }

    if (orbEntry->pol == enums::BearWriteOpt &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingLocMemReadResp &&
        orbEntry->isHit) {
            // DONE
            // send the respond to the requestor

            PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
                                             false,
                                             orbEntry->owPkt->isRead());

            accessAndRespond(orbEntry->owPkt,
                             frontendLatency + backendLatency);

            ORB.at(copyOwPkt->getAddr()) = new reqBufferEntry(
                                                orbEntry->validEntry,
                                                orbEntry->arrivalTick,
                                                orbEntry->tagDC,
                                                orbEntry->indexDC,
                                                copyOwPkt,
                                                orbEntry->pol,
                                                orbEntry->state,
                                                orbEntry->issued,
                                                orbEntry->isHit,
                                                orbEntry->conflict,
                                                orbEntry->dirtyLineAddr,
                                                orbEntry->handleDirtyLine,
                                                orbEntry->tagCheckEntered,
                                                orbEntry->tagCheckIssued,
                                                orbEntry->tagCheckExit,
                                                orbEntry->locRdEntered,
                                                orbEntry->locRdIssued,
                                                orbEntry->locRdExit,
                                                orbEntry->locWrEntered,
                                                orbEntry->locWrIssued,
                                                orbEntry->locWrExit,
                                                orbEntry->farRdEntered,
                                                orbEntry->farRdIssued,
                                                orbEntry->farRdExit);
            delete orbEntry;

            orbEntry = ORB.at(copyOwPkt->getAddr());
            
            polManStats.totPktRespTime += ((curTick() - orbEntry->arrivalTick)/1000);
            polManStats.totPktRespTimeRd += ((curTick() - orbEntry->arrivalTick)/1000);

            // clear ORB
            resumeConflictingReq(orbEntry);

            return;            
    }

    if (orbEntry->pol == enums::BearWriteOpt &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == farMemRead) {

            assert(!orbEntry->isHit);

            // do a read from far mem
            pktFarMemRead.push_back(orbEntry->owPkt->getAddr());

            polManStats.avgFarRdQLenEnq = pktFarMemRead.size();

            if (!farMemReadEvent.scheduled() && !retryFarMemRead) {
                schedule(farMemReadEvent, curTick());
            }
            return;

    }

    if (orbEntry->pol == enums::BearWriteOpt &&
        orbEntry->state == locMemWrite) {

            if (orbEntry->owPkt->isRead()) {
                assert(!orbEntry->isHit);
            }

            // do a read from far mem
            pktLocMemWrite.push_back(orbEntry->owPkt->getAddr());

            polManStats.avgLocWrQLenEnq = pktLocMemWrite.size();
            

            if (!locMemWriteEvent.scheduled() && !retryLocMemWrite) {
                schedule(locMemWriteEvent, curTick());
            }
            return;

    }

    if (orbEntry->pol == enums::BearWriteOpt &&
        // orbEntry->owPkt->isRead() &&
        // !orbEntry->isHit &&
        orbEntry->state == waitingLocMemWriteResp) {
            // DONE
            // clear ORB
            resumeConflictingReq(orbEntry);

            return;
    }

    ////////////////////////////////////////////////////////////////////////
    // Rambus

    if (orbEntry->pol == enums::Rambus &&
        orbEntry->state == tagCheck) {

        pktTagCheck.push_back(orbEntry->owPkt->getAddr());

        polManStats.avgTagCheckQLenEnq = pktTagCheck.size();

        if (!tagCheckEvent.scheduled() && !retryTagCheck) {
            schedule(tagCheckEvent, curTick());
        }
        return;
    }

    if (orbEntry->pol == enums::Rambus &&
        orbEntry->state == waitingLocMemReadResp) {
            return;
    }

    if (orbEntry->pol == enums::Rambus &&
        orbEntry->state == locRdRespReady) {
            assert(orbEntry->owPkt->isRead());
            assert(orbEntry->isHit);
            // DONE
            // send the respond to the requestor

            PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
                                             false,
                                             orbEntry->owPkt->isRead());

            accessAndRespond(orbEntry->owPkt,
                             frontendLatency + backendLatency);

            ORB.at(copyOwPkt->getAddr()) = new reqBufferEntry(
                                                orbEntry->validEntry,
                                                orbEntry->arrivalTick,
                                                orbEntry->tagDC,
                                                orbEntry->indexDC,
                                                copyOwPkt,
                                                orbEntry->pol,
                                                orbEntry->state,
                                                orbEntry->issued,
                                                orbEntry->isHit,
                                                orbEntry->conflict,
                                                orbEntry->dirtyLineAddr,
                                                orbEntry->handleDirtyLine,
                                                orbEntry->tagCheckEntered,
                                                orbEntry->tagCheckIssued,
                                                orbEntry->tagCheckExit,
                                                orbEntry->locRdEntered,
                                                orbEntry->locRdIssued,
                                                orbEntry->locRdExit,
                                                orbEntry->locWrEntered,
                                                orbEntry->locWrIssued,
                                                orbEntry->locWrExit,
                                                orbEntry->farRdEntered,
                                                orbEntry->farRdIssued,
                                                orbEntry->farRdExit);
            delete orbEntry;

            orbEntry = ORB.at(copyOwPkt->getAddr());

            polManStats.totPktRespTime += ((curTick() - orbEntry->arrivalTick)/1000);
            polManStats.totPktRespTimeRd += ((curTick() - orbEntry->arrivalTick)/1000);

            // clear ORB
            resumeConflictingReq(orbEntry);

            return;
    }

    if (orbEntry->pol == enums::Rambus &&
        orbEntry->owPkt->isWrite()) {
            // DONE
            // respond for writes is already sent to the requestor.
            // clear ORB
            assert(orbEntry->state == waitingTCtag);

            resumeConflictingReq(orbEntry);

            return;
    }

    if (orbEntry->pol == enums::Rambus &&
        orbEntry->state == farMemRead) {

            assert(orbEntry->owPkt->isRead());
            assert(!orbEntry->isHit);

            // do a read from far mem
            pktFarMemRead.push_back(orbEntry->owPkt->getAddr());

            polManStats.avgFarRdQLenEnq = pktFarMemRead.size();

            if (!farMemReadEvent.scheduled() && !retryFarMemRead) {
                schedule(farMemReadEvent, curTick());
            }
            return;

    }

    if (orbEntry->pol == enums::Rambus &&
        orbEntry->state == locMemWrite) {

            assert(orbEntry->owPkt->isRead());
            assert(!orbEntry->isHit);

            pktLocMemWrite.push_back(orbEntry->owPkt->getAddr());

            polManStats.avgLocWrQLenEnq = pktLocMemWrite.size();


            if (!locMemWriteEvent.scheduled() && !retryLocMemWrite) {
                schedule(locMemWriteEvent, curTick());
            }
            return;

    }

    if (orbEntry->pol == enums::Rambus &&
        // orbEntry->owPkt->isRead() &&
        // !orbEntry->isHit &&
        orbEntry->state == waitingLocMemWriteResp) {
            // DONE
            // clear ORB
            assert(orbEntry->owPkt->isRead());
            assert(!orbEntry->isHit);
            resumeConflictingReq(orbEntry);

            return;
    }
}

void
PolicyManager::handleRequestorPkt(PacketPtr pkt)
{
    reqBufferEntry* orbEntry = new reqBufferEntry(
                                true, curTick(),
                                returnTagDC(pkt->getAddr(), pkt->getSize()),
                                returnIndexDC(pkt->getAddr(), pkt->getSize()),
                                pkt,
                                locMemPolicy, start,
                                false, false, false,
                                -1, false,
                                MaxTick, MaxTick, MaxTick,
                                MaxTick, MaxTick, MaxTick,
                                MaxTick, MaxTick, MaxTick,
                                MaxTick, MaxTick, MaxTick
                            );
    
    ORB.emplace(pkt->getAddr(), orbEntry);

    DPRINTF(PolicyManager, "handleRequestorPkt added to ORB: adr= %d, index= %d, tag= %d\n", orbEntry->owPkt->getAddr(), orbEntry->indexDC, orbEntry->tagDC);

    polManStats.avgORBLen = ORB.size();
    polManStats.avgTagCheckQLenStrt = countTagCheckInORB();
    polManStats.avgLocRdQLenStrt = countLocRdInORB();
    polManStats.avgFarRdQLenStrt = countFarRdInORB();
    polManStats.avgLocWrQLenStrt = countLocWrInORB();
    polManStats.avgFarWrQLenStrt = countFarWr();

    Addr addr = pkt->getAddr();
    unsigned burst_size = locBurstSize;
    unsigned size = std::min((addr | (burst_size - 1)) + 1,
                              addr + pkt->getSize()) - addr;

    if(pkt->isRead()) {
        polManStats.bytesReadSys += size;
        polManStats.readPktSize[ceilLog2(size)]++;
        polManStats.readReqs++;
    } else {
        polManStats.bytesWrittenSys += size;
        polManStats.writePktSize[ceilLog2(size)]++;
        polManStats.writeReqs++;
    }

    if (pkt->isWrite()) {

        PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
                                             false,
                                             orbEntry->owPkt->isRead());

        accessAndRespond(orbEntry->owPkt,
                         frontendLatency + backendLatency);

        ORB.at(copyOwPkt->getAddr()) = new reqBufferEntry(
                                            orbEntry->validEntry,
                                            orbEntry->arrivalTick,
                                            orbEntry->tagDC,
                                            orbEntry->indexDC,
                                            copyOwPkt,
                                            orbEntry->pol,
                                            orbEntry->state,
                                            orbEntry->issued,
                                            orbEntry->isHit,
                                            orbEntry->conflict,
                                            orbEntry->dirtyLineAddr,
                                            orbEntry->handleDirtyLine,
                                            orbEntry->tagCheckEntered,
                                            orbEntry->tagCheckIssued,
                                            orbEntry->tagCheckExit,
                                            orbEntry->locRdEntered,
                                            orbEntry->locRdIssued,
                                            orbEntry->locRdExit,
                                            orbEntry->locWrEntered,
                                            orbEntry->locWrIssued,
                                            orbEntry->locWrExit,
                                            orbEntry->farRdEntered,
                                            orbEntry->farRdIssued,
                                            orbEntry->farRdExit);
        delete orbEntry;

        orbEntry = ORB.at(copyOwPkt->getAddr());
    }

    checkHitOrMiss(orbEntry);

    if (checkDirty(orbEntry->owPkt->getAddr()) && !orbEntry->isHit) {
        if (extreme && tagMetadataStore.at(orbEntry->indexDC).farMemAddr == -1) {
            // extreme cases are faked. So, this address for cold misses are -1, so we fake it to a random address like 1024.
            orbEntry->dirtyLineAddr = orbEntry->owPkt->getAddr() == 0 ? 64 : (orbEntry->owPkt->getAddr()-64);
        } else {
            orbEntry->dirtyLineAddr = tagMetadataStore.at(orbEntry->indexDC).farMemAddr;
        }
        orbEntry->handleDirtyLine = true;
    }

    if (extreme) {
        orbEntry->prevDirty = alwaysDirty;
    } else {
        orbEntry->prevDirty = tagMetadataStore.at(orbEntry->indexDC).dirtyLine;
    }

    // Updating Tag & Metadata
    tagMetadataStore.at(orbEntry->indexDC).tagDC = orbEntry->tagDC;
    tagMetadataStore.at(orbEntry->indexDC).indexDC = orbEntry->indexDC;
    tagMetadataStore.at(orbEntry->indexDC).validLine = true;

    if (orbEntry->owPkt->isRead()) {
        if (orbEntry->isHit) {
            tagMetadataStore.at(orbEntry->indexDC).dirtyLine =
            tagMetadataStore.at(orbEntry->indexDC).dirtyLine;
        } else {
            tagMetadataStore.at(orbEntry->indexDC).dirtyLine = false;
        }
    } else { // write
        tagMetadataStore.at(orbEntry->indexDC).dirtyLine = true;
        tagMetadataStore.at(orbEntry->indexDC).farMemAddr =
                        orbEntry->owPkt->getAddr();
    }

    // tagMetadataStore.at(orbEntry->indexDC).farMemAddr =
    //                     orbEntry->owPkt->getAddr();

    DPRINTF(PolicyManager, "ORB+: adr= %d, index= %d, tag= %d, cmd= %s, isHit= %d, wasDirty= %d, \n", orbEntry->owPkt->getAddr(), orbEntry->indexDC, orbEntry->tagDC, orbEntry->owPkt->cmdString(), orbEntry->isHit, orbEntry->prevDirty);
}

bool
PolicyManager::checkConflictInDramCache(PacketPtr pkt)
{
    unsigned indexDC = returnIndexDC(pkt->getAddr(), pkt->getSize());

    for (auto e = ORB.begin(); e != ORB.end(); ++e) {
        if (indexDC == e->second->indexDC && e->second->validEntry) {

            e->second->conflict = true;

            return true;
        }
    }
    return false;
}

void
PolicyManager::checkHitOrMiss(reqBufferEntry* orbEntry)
{
    // access the tagMetadataStore data structure to
    // check if it's hit or miss

    bool currValid = tagMetadataStore.at(orbEntry->indexDC).validLine;
    bool currDirty = tagMetadataStore.at(orbEntry->indexDC).dirtyLine;

    if (extreme) {
        orbEntry->isHit = alwaysHit;
        currValid = true;
        currDirty = alwaysDirty;
    } else {
        orbEntry->isHit = currValid && (orbEntry->tagDC == tagMetadataStore.at(orbEntry->indexDC).tagDC);
    }

    if (orbEntry->isHit) {

        polManStats.numTotHits++;

        if (orbEntry->owPkt->isRead()) {
            polManStats.numRdHit++;
            if (currDirty) {
                polManStats.numRdHitDirty++;
            } else {
                polManStats.numRdHitClean++;
            }
        } else {
            polManStats.numWrHit++;
            if (currDirty) {
                polManStats.numWrHitDirty++;
            } else {
                polManStats.numWrHitClean++;
            }
        }

    } else {

        polManStats.numTotMisses++;

        if (currValid) {
            polManStats.numHotMisses++;
        } else {
            polManStats.numColdMisses++;
            numColdMisses++;
        }

        if (orbEntry->owPkt->isRead()) {
            if (currDirty && currValid) {
                polManStats.numRdMissDirty++;
            } else {
                polManStats.numRdMissClean++;
            }
        } else {
            if (currDirty && currValid) {
                polManStats.numWrMissDirty++;
            } else {
                polManStats.numWrMissClean++;
            }

        }
    }

}

bool
PolicyManager::checkDirty(Addr addr)
{
    Addr index = returnIndexDC(addr, blockSize);
    if (extreme) {
        return alwaysDirty;
    } else {
        return (tagMetadataStore.at(index).validLine && tagMetadataStore.at(index).dirtyLine);
    }
}

void
PolicyManager::accessAndRespond(PacketPtr pkt, Tick static_latency)
{
    DPRINTF(PolicyManager, "Responding to Address %d: %s\n", pkt->getAddr(), pkt->cmdString());

    bool needsResponse = pkt->needsResponse();
    // do the actual memory access which also turns the packet into a
    // response
    panic_if(!getAddrRange().contains(pkt->getAddr()),
             "Can't handle address range for packet %s\n", pkt->print());
    access(pkt);

    // turn packet around to go back to requestor if response expected
    assert(needsResponse);
    //if (needsResponse) {
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
    //}
    // else {
    //     // @todo the packet is going to be deleted, and the MemPacket
    //     // is still having a pointer to it
    //     pendingDelete.reset(pkt);
    // }

    DPRINTF(PolicyManager, "Done\n");

    return;
}

PacketPtr
PolicyManager::getPacket(Addr addr, unsigned size, const MemCmd& cmd,
                   Request::FlagsType flags)
{
    // Create new request
    RequestPtr req = std::make_shared<Request>(addr, size, flags,
                                               0);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr)0) << 2);

    // Embed it in a packet
    PacketPtr pkt = new Packet(req, cmd);

    uint8_t* pkt_data = new uint8_t[req->getSize()];

    pkt->dataDynamic(pkt_data);

    if (cmd.isWrite()) {
        std::fill_n(pkt_data, req->getSize(), (uint8_t)0);
    }

    return pkt;
}

void
PolicyManager::sendRespondToRequestor(PacketPtr pkt, Tick static_latency)
{
    PacketPtr copyOwPkt = new Packet(pkt,
                                     false,
                                     pkt->isRead());
    copyOwPkt->makeResponse();

    Tick response_time = curTick() + static_latency + copyOwPkt->headerDelay;
    response_time += copyOwPkt->payloadDelay;
    // Here we reset the timing of the packet before sending it out.
    copyOwPkt->headerDelay = copyOwPkt->payloadDelay = 0;

    // queue the packet in the response queue to be sent out after
    // the static latency has passed
    port.schedTimingResp(copyOwPkt, response_time);

}

bool
PolicyManager::resumeConflictingReq(reqBufferEntry* orbEntry)
{
    DPRINTF(PolicyManager, "resumeConflictingReq: %d: %d \n", curTick(), orbEntry->owPkt->getAddr());

    bool conflictFound = false;

    if (orbEntry->owPkt->isWrite()) {
        isInWriteQueue.erase(orbEntry->owPkt->getAddr());
    }

    logStatsPolMan(orbEntry);

    for (auto e = CRB.begin(); e != CRB.end(); ++e) {

        auto entry = *e;

        if (returnIndexDC(entry.second->getAddr(), entry.second->getSize())
            == orbEntry->indexDC) {

                DPRINTF(PolicyManager, "conf found: %d\n", entry.second->getAddr());

                conflictFound = true;

                Addr confAddr = entry.second->getAddr();

                ORB.erase(orbEntry->owPkt->getAddr());

                delete orbEntry->owPkt;

                delete orbEntry;

                handleRequestorPkt(entry.second);

                ORB.at(confAddr)->arrivalTick = entry.first;

                CRB.erase(e);

                checkConflictInCRB(ORB.at(confAddr));

                setNextState(ORB.at(confAddr));

                handleNextState(ORB.at(confAddr));

                break;
        }

    }

    if (!conflictFound) {
        DPRINTF(PolicyManager, "no conf for: %d\n", orbEntry->owPkt->getAddr());

        ORB.erase(orbEntry->owPkt->getAddr());

        delete orbEntry->owPkt;

        delete orbEntry;

        if (retryLLC) {
            DPRINTF(PolicyManager, "retryLLC: sent\n");
            retryLLC = false;
            port.sendRetryReq();
        } else {
            if (drainState() == DrainState::Draining && ORB.empty() &&
                pktFarMemWrite.empty()) {
                DPRINTF(Drain, "PolicyManager done draining\n");
                signalDrainDone();
            }
        }
    }

    return conflictFound;
}

void
PolicyManager::checkConflictInCRB(reqBufferEntry* orbEntry)
{
    for (auto e = CRB.begin(); e != CRB.end(); ++e) {

        auto entry = *e;

        if (returnIndexDC(entry.second->getAddr(),entry.second->getSize())
            == orbEntry->indexDC) {
                orbEntry->conflict = true;
                break;
        }
    }
}

unsigned
PolicyManager::countTagCheckInORB()
{
    unsigned count =0;
    for (auto i : ORB) {
        if (i.second->state == tagCheck) {
            count++;
        }
    }
    return count;
}

unsigned
PolicyManager::countLocRdInORB()
{
    unsigned count =0;
    for (auto i : ORB) {
        if (i.second->state == locMemRead) {
            count++;
        }
    }
    return count;
}

unsigned
PolicyManager::countFarRdInORB()
{
    unsigned count =0;
    for (auto i : ORB) {
        if (i.second->state == farMemRead) {
            count++;
        }
    }
    return count;
}

unsigned
PolicyManager::countLocWrInORB()
{
    unsigned count =0;
    for (auto i : ORB) {
        if (i.second->state == locMemWrite) {
            count++;
        }
    }
    return count;
}

unsigned
PolicyManager::countFarWr()
{
    return pktFarMemWrite.size();
}

AddrRangeList
PolicyManager::getAddrRanges()
{
    return farReqPort.getAddrRanges();
}

Addr
PolicyManager::returnIndexDC(Addr request_addr, unsigned size)
{
    int index_bits = ceilLog2(dramCacheSize/blockSize);
    int block_bits = ceilLog2(size);
    return bits(request_addr, block_bits + index_bits-1, block_bits);
}

Addr
PolicyManager::returnTagDC(Addr request_addr, unsigned size)
{
    int index_bits = ceilLog2(dramCacheSize/blockSize);
    int block_bits = ceilLog2(size);
    return bits(request_addr, addrSize-1, (index_bits+block_bits));
}

void
PolicyManager::handleDirtyCacheLine(Addr dirtyLineAddr)
{
    DPRINTF(PolicyManager, "handleDirtyCacheLine: %d\n", dirtyLineAddr);
    assert(dirtyLineAddr != -1);

    // create a new request packet
    PacketPtr wbPkt = getPacket(dirtyLineAddr,
                                blockSize,
                                MemCmd::WriteReq);

    pktFarMemWrite.push_back(std::make_pair(curTick(), wbPkt));

    polManStats.avgFarWrQLenEnq = pktFarMemWrite.size();

    if (!farMemWriteEvent.scheduled() && !retryFarMemWrite) {
            schedule(farMemWriteEvent, curTick());
    }

    polManStats.numWrBacks++;
}

void
PolicyManager::logStatsPolMan(reqBufferEntry* orbEntry)
{
    if (locMemPolicy == enums::Rambus) {
        assert(orbEntry->arrivalTick != MaxTick);
        assert(orbEntry->tagCheckEntered != MaxTick);
        assert(orbEntry->tagCheckExit != MaxTick);

        polManStats.totPktLifeTime += ((curTick() - orbEntry->arrivalTick)/1000);
        polManStats.totPktORBTime += ((curTick() - orbEntry->tagCheckEntered)/1000);
        polManStats.totTimeTagCheckRes += ((orbEntry->tagCheckExit - orbEntry->tagCheckEntered)/1000);

        if (orbEntry->owPkt->isRead()) {
            polManStats.totPktLifeTimeRd += ((curTick() - orbEntry->arrivalTick)/1000);
            polManStats.totPktORBTimeRd += ((curTick() - orbEntry->tagCheckEntered)/1000);
            polManStats.totTimeTagCheckResRd += ((orbEntry->tagCheckExit - orbEntry->tagCheckEntered)/1000);
        } else {
            polManStats.totPktRespTime += ((curTick() - orbEntry->arrivalTick)/1000);
            polManStats.totPktRespTimeWr += ((curTick() - orbEntry->arrivalTick)/1000);
            polManStats.totPktLifeTimeWr += ((curTick() - orbEntry->arrivalTick)/1000);
            polManStats.totPktORBTimeWr += ((curTick() - orbEntry->tagCheckEntered)/1000);
            polManStats.totTimeTagCheckResWr += ((orbEntry->tagCheckExit - orbEntry->tagCheckEntered)/1000);
        }

        if (orbEntry->owPkt->isRead() && orbEntry->isHit) {
            assert(orbEntry->locRdExit != MaxTick);
            polManStats.totTimeInLocRead += ((orbEntry->locRdExit - orbEntry->tagCheckEntered)/1000);
        }

        if (orbEntry->owPkt->isRead() && !orbEntry->isHit) {
            assert(orbEntry->farRdExit != MaxTick);
            assert(orbEntry->farRdEntered != MaxTick);
            assert(orbEntry->farRdIssued != MaxTick);
            assert(orbEntry->tagCheckExit != MaxTick);
            assert(orbEntry->locWrExit != MaxTick);
            assert(orbEntry->locWrEntered != MaxTick);

            polManStats.totTimeInFarRead += ((orbEntry->farRdExit - orbEntry->farRdEntered)/1000);
            polManStats.totTimeFarRdtoSend += ((orbEntry->farRdIssued - orbEntry->farRdEntered)/1000);
            polManStats.totTimeFarRdtoRecv += ((orbEntry->farRdExit - orbEntry->farRdIssued)/1000);
            polManStats.totTimeInLocWrite += ((orbEntry->locWrExit - orbEntry->locWrEntered)/1000);
        }  
    }
    else {
        // MUST be updated since they are average, they should be per case
        polManStats.totPktLifeTime += ((curTick() - orbEntry->arrivalTick)/1000);
        polManStats.totPktORBTime += ((curTick() - orbEntry->locRdEntered)/1000);
        polManStats.totTimeFarRdtoSend += ((orbEntry->farRdIssued - orbEntry->farRdEntered)/1000);
        polManStats.totTimeFarRdtoRecv += ((orbEntry->farRdExit - orbEntry->farRdIssued)/1000);
        polManStats.totTimeInLocRead += ((orbEntry->locRdExit - orbEntry->locRdEntered)/1000);
        polManStats.totTimeInLocWrite += ((orbEntry->locWrExit - orbEntry->locWrEntered)/1000);
        polManStats.totTimeInFarRead += ((orbEntry->farRdExit - orbEntry->farRdEntered)/1000);
    }
}


void
PolicyManager::ReqPortPolManager::recvReqRetry()
{
    if (this->name().find("loc_req_port") != std::string::npos) {
        polMan.locMemRecvReqRetry();
    }
    if (this->name().find("far_req_port") != std::string::npos) {
        polMan.farMemRecvReqRetry();
    }
}

bool
PolicyManager::ReqPortPolManager::recvTimingResp(PacketPtr pkt)
{
    // since in the constructor we are appending loc_req_port
    // to the loc mem port, the name should always have the substring
    // irrespective of the configuration names
    if (this->name().find("loc_req_port") != std::string::npos) {
        return polMan.locMemRecvTimingResp(pkt);
    } else if (this->name().find("far_req_port") != std::string::npos) {
        return polMan.farMemRecvTimingResp(pkt);
    } else {
        std::cout << "Port name error, fix it!\n";
        return false;
    }
}

PolicyManager::PolicyManagerStats::PolicyManagerStats(PolicyManager &_polMan)
    : statistics::Group(&_polMan),
    polMan(_polMan),

/////
    ADD_STAT(readReqs, statistics::units::Count::get(),
             "Number of read requests accepted"),
    ADD_STAT(writeReqs, statistics::units::Count::get(),
             "Number of write requests accepted"),

    ADD_STAT(servicedByWrQ, statistics::units::Count::get(),
             "Number of controller read bursts serviced by the write queue"),
    ADD_STAT(servicedByFB, statistics::units::Count::get(),
             "Number of controller read bursts serviced by the flush buffer"),
    ADD_STAT(mergedWrBursts, statistics::units::Count::get(),
             "Number of controller write bursts merged with an existing one"),
    ADD_STAT(mergedWrPolManWB, statistics::units::Count::get(),
                 "Number of controller write bursts merged with an existing one in write back buffer"),
    ADD_STAT(mergedWrLocMemFB, statistics::units::Count::get(),
             "Number of controller write bursts merged with an existing one in flush buffer"),

    ADD_STAT(numRdRetry, statistics::units::Count::get(),
             "Number of times read queue was full causing retry"),
    ADD_STAT(numWrRetry, statistics::units::Count::get(),
             "Number of times write queue was full causing retry"),

    ADD_STAT(readPktSize, statistics::units::Count::get(),
             "Read request sizes (log2)"),
    ADD_STAT(writePktSize, statistics::units::Count::get(),
             "Write request sizes (log2)"),

    ADD_STAT(bytesReadWrQ, statistics::units::Byte::get(),
             "Total number of bytes read from write queue"),
    ADD_STAT(bytesReadSys, statistics::units::Byte::get(),
             "Total read bytes from the system interface side"),
    ADD_STAT(bytesWrittenSys, statistics::units::Byte::get(),
             "Total written bytes from the system interface side"),

    ADD_STAT(avgRdBWSys, statistics::units::Rate<
                statistics::units::Byte, statistics::units::Second>::get(),
             "Average system read bandwidth in Byte/s"),
    ADD_STAT(avgWrBWSys, statistics::units::Rate<
                statistics::units::Byte, statistics::units::Second>::get(),
             "Average system write bandwidth in Byte/s"),

    ADD_STAT(totGap, statistics::units::Tick::get(),
             "Total gap between requests"),
    ADD_STAT(avgGap, statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(),
             "Average gap between requests"),

    ADD_STAT(avgORBLen, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average ORB length"),
    ADD_STAT(avgTagCheckQLenStrt, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average local read queue length"),
    ADD_STAT(avgLocRdQLenStrt, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average local read queue length"),
    ADD_STAT(avgLocWrQLenStrt, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average local write queue length"),
    ADD_STAT(avgFarRdQLenStrt, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average far read queue length"),
    ADD_STAT(avgFarWrQLenStrt, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average far write queue length"),

    ADD_STAT(avgTagCheckQLenEnq, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average local read queue length when enqueuing"),
    ADD_STAT(avgLocRdQLenEnq, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average local read queue length when enqueuing"),
    ADD_STAT(avgLocWrQLenEnq, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average local write queue length when enqueuing"),
    ADD_STAT(avgFarRdQLenEnq, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average far read queue length when enqueuing"),
    ADD_STAT(avgFarWrQLenEnq, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average far write queue length when enqueuing"),

    ADD_STAT(numWrBacks, statistics::units::Count::get(),
            "Total number of write backs from DRAM cache to main memory"),
    ADD_STAT(totNumConf, statistics::units::Count::get(),
            "Total number of packets conflicted on DRAM cache"),
    ADD_STAT(totNumORBFull, statistics::units::Count::get(),
            "Total number of packets ORB full"),
    ADD_STAT(totNumCRBFull, statistics::units::Count::get(),
            "Total number of packets conflicted yet couldn't "
            "enter confBuffer"),

    ADD_STAT(maxNumConf, statistics::units::Count::get(),
            "Maximum number of packets conflicted on DRAM cache"),

    ADD_STAT(sentTagCheckPort, statistics::units::Count::get(),
             "stat"),
    ADD_STAT(failedTagCheckPort, statistics::units::Count::get(),
             "stat"),
    ADD_STAT(sentLocRdPort, statistics::units::Count::get(),
             "stat"),
    ADD_STAT(sentLocWrPort, statistics::units::Count::get(),
             "stat"),
    ADD_STAT(failedLocRdPort, statistics::units::Count::get(),
             "stat"),
    ADD_STAT(failedLocWrPort, statistics::units::Count::get(),
             "stat"),
    // ADD_STAT(recvdRdPort, statistics::units::Count::get(),
    //         "stat"),
    ADD_STAT(sentFarRdPort, statistics::units::Count::get(),
             "stat"),
    ADD_STAT(sentFarWrPort, statistics::units::Count::get(),
             "stat"),
    ADD_STAT(failedFarRdPort, statistics::units::Count::get(),
             "stat"),
    ADD_STAT(failedFarWrPort, statistics::units::Count::get(),
             "stat"),

    ADD_STAT(totPktLifeTime, statistics::units::Tick::get(), "stat"),
    ADD_STAT(totPktLifeTimeRd, statistics::units::Tick::get(), "stat"),
    ADD_STAT(totPktLifeTimeWr, statistics::units::Tick::get(), "stat"),
    ADD_STAT(totPktORBTime, statistics::units::Tick::get(), "stat"),
    ADD_STAT(totPktORBTimeRd, statistics::units::Tick::get(), "stat"),
    ADD_STAT(totPktORBTimeWr, statistics::units::Tick::get(), "stat"),
    ADD_STAT(totPktRespTime, statistics::units::Tick::get(), "stat"),
    ADD_STAT(totPktRespTimeRd, statistics::units::Tick::get(), "stat"),
    ADD_STAT(totPktRespTimeWr,  statistics::units::Tick::get(), "stat"),
    ADD_STAT(totTimeTagCheckRes,  statistics::units::Tick::get(), "stat"),
    ADD_STAT(totTimeTagCheckResRd,  statistics::units::Tick::get(), "stat"),
    ADD_STAT(totTimeTagCheckResWr,  statistics::units::Tick::get(), "stat"),
    ADD_STAT(totTimeInLocRead,  statistics::units::Tick::get(), "stat"),
    ADD_STAT(totTimeInLocWrite,  statistics::units::Tick::get(), "stat"),
    ADD_STAT(totTimeInFarRead,  statistics::units::Tick::get(), "stat"),
    ADD_STAT(totTimeFarRdtoSend,  statistics::units::Tick::get(), "stat"),
    ADD_STAT(totTimeFarRdtoRecv,  statistics::units::Tick::get(), "stat"),
    ADD_STAT(totTimeFarWrtoSend,  statistics::units::Tick::get(), "stat"),

    ADD_STAT(avgPktLifeTime, statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgPktLifeTimeRd, statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgPktLifeTimeWr, statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgPktORBTime, statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgPktORBTimeRd, statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgPktORBTimeWr, statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgPktRespTime, statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgPktRespTimeRd, statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgPktRespTimeWr,  statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgTimeTagCheckRes,  statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgTimeTagCheckResRd,  statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgTimeTagCheckResWr,  statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgTimeInLocRead,  statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgTimeInLocWrite,  statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgTimeInFarRead,  statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgTimeFarRdtoSend,  statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgTimeFarRdtoRecv,  statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),
    ADD_STAT(avgTimeFarWrtoSend,  statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(), "stat"),

    ADD_STAT(numTotHits, statistics::units::Count::get(), "stat"),
    ADD_STAT(numTotMisses, statistics::units::Count::get(), "stat"),
    ADD_STAT(numColdMisses, statistics::units::Count::get(), "stat"),
    ADD_STAT(numHotMisses, statistics::units::Count::get(), "stat"),
    ADD_STAT(numRdMissClean, statistics::units::Count::get(), "stat"),
    ADD_STAT(numRdMissDirty, statistics::units::Count::get(), "stat"),
    ADD_STAT(numRdHit, statistics::units::Count::get(), "stat"),
    ADD_STAT(numWrMissClean, statistics::units::Count::get(), "stat"),
    ADD_STAT(numWrMissDirty, statistics::units::Count::get(), "stat"),
    ADD_STAT(numWrHit, statistics::units::Count::get(), "stat"),
    ADD_STAT(numRdHitDirty, statistics::units::Count::get(), "stat"),
    ADD_STAT(numRdHitClean, statistics::units::Count::get(), "stat"),
    ADD_STAT(numWrHitDirty, statistics::units::Count::get(), "stat"),
    ADD_STAT(numWrHitClean, statistics::units::Count::get(), "stat")

{
}

void
PolicyManager::PolicyManagerStats::regStats()
{
    using namespace statistics;

    avgORBLen.precision(4);
    avgLocRdQLenStrt.precision(2);
    avgLocWrQLenStrt.precision(2);
    avgFarRdQLenStrt.precision(2);
    avgFarWrQLenStrt.precision(2);

    avgLocRdQLenEnq.precision(2);
    avgLocWrQLenEnq.precision(2);
    avgFarRdQLenEnq.precision(2);
    avgFarWrQLenEnq.precision(2);

    avgPktLifeTime.precision(2);
    avgPktLifeTimeRd.precision(2);
    avgPktLifeTimeWr.precision(2);
    avgPktORBTime.precision(2);
    avgPktORBTimeRd.precision(2);
    avgPktORBTimeWr.precision(2);
    avgPktRespTime.precision(2);
    avgPktRespTimeRd.precision(2);
    avgPktRespTimeWr.precision(2);
    avgTimeTagCheckRes.precision(2);
    avgTimeTagCheckResRd.precision(2);
    avgTimeTagCheckResWr.precision(2);
    avgTimeInLocRead.precision(2);
    avgTimeInLocWrite.precision(2);
    avgTimeInFarRead.precision(2);
    avgTimeFarRdtoSend.precision(2);
    avgTimeFarRdtoRecv.precision(2);
    avgTimeFarWrtoSend.precision(2);

    readPktSize.init(ceilLog2(polMan.blockSize) + 1);
    writePktSize.init(ceilLog2(polMan.blockSize) + 1);

    avgRdBWSys.precision(8);
    avgWrBWSys.precision(8);
    avgGap.precision(2);

    // Formula stats
    avgRdBWSys = (bytesReadSys) / simSeconds;
    avgWrBWSys = (bytesWrittenSys) / simSeconds;

    avgPktLifeTime = (totPktLifeTime) / (readReqs + writeReqs);
    avgPktLifeTimeRd = (totPktLifeTimeRd) / (numRdHit + numRdMissClean + numRdMissDirty);
    avgPktLifeTimeWr = (totPktLifeTimeWr) / (numWrHit + numWrMissClean + numWrMissDirty);
    avgPktORBTime = (totPktORBTime) / (readReqs + writeReqs);
    avgPktORBTimeRd = (totPktORBTimeRd) / (numRdHit + numRdMissClean + numRdMissDirty);
    avgPktORBTimeWr = (totPktORBTimeWr) / (numWrHit + numWrMissClean + numWrMissDirty);
    avgPktRespTime = (totPktRespTime) / (readReqs + writeReqs);
    avgPktRespTimeRd = (totPktRespTimeRd) / (numRdHit + numRdMissClean + numRdMissDirty);
    avgPktRespTimeWr = (totPktRespTimeWr) / (numWrHit + numWrMissClean + numWrMissDirty);
    avgTimeTagCheckRes = (totTimeTagCheckRes) / (readReqs + writeReqs);
    avgTimeTagCheckResRd = (totTimeTagCheckResRd) / (numRdHit + numRdMissClean + numRdMissDirty);
    avgTimeTagCheckResWr = (totTimeTagCheckResWr) / (numWrHit + numWrMissClean + numWrMissDirty);
    avgTimeInLocRead = (totTimeInLocRead) / (numRdHit);
    avgTimeInLocWrite = (totTimeInLocWrite) / (numRdMissClean + numRdMissDirty);
    avgTimeInFarRead = (totTimeInFarRead) / (numRdMissClean + numRdMissDirty);
    avgTimeFarRdtoSend = (totTimeFarRdtoSend) / (sentFarRdPort);
    avgTimeFarRdtoRecv = (totTimeFarRdtoRecv) / (sentFarRdPort);
    avgTimeFarWrtoSend = (totTimeFarWrtoSend) / (sentFarWrPort);

    avgGap = totGap / (readReqs + writeReqs);

}

Port &
PolicyManager::getPort(const std::string &if_name, PortID idx)
{
    panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

    // This is the name from the Python SimObject declaration (SimpleMemobj.py)
    if (if_name == "port") {
        return port;
    } else if (if_name == "loc_req_port") {
        return locReqPort;
    } else if (if_name == "far_req_port") {
        return farReqPort;
    } else {
        // pass it along to our super class
        panic("PORT NAME ERROR !!!!\n");
    }
}

DrainState
PolicyManager::drain()
{
    if (!ORB.empty() || !pktFarMemWrite.empty()) {
        DPRINTF(Drain, "DRAM cache is not drained! Have %d in ORB and %d in "
                "writeback queue.\n", ORB.size(), pktFarMemWrite.size());
        return DrainState::Draining;
    } else {
        return DrainState::Drained;
    }
}

void
PolicyManager::serialize(CheckpointOut &cp) const
{
    ScopedCheckpointSection sec(cp, "tagMetadataStore");
    paramOut(cp, "numEntries", tagMetadataStore.size());

    int count = 0;
    for (auto const &entry : tagMetadataStore) {
        ScopedCheckpointSection sec_entry(cp,csprintf("Entry%d", count++));
        if (entry.validLine) {
            paramOut(cp, "validLine", entry.validLine);
            paramOut(cp, "tagDC", entry.tagDC);
            paramOut(cp, "indexDC", entry.indexDC);
            paramOut(cp, "dirtyLine", entry.dirtyLine);
            paramOut(cp, "farMemAddr", entry.farMemAddr);
        } else {
            paramOut(cp, "validLine", entry.validLine);
        }
    }
}

void
PolicyManager::unserialize(CheckpointIn &cp)
{
    ScopedCheckpointSection sec(cp, "tagMetadataStore");
    int num_entries = 0;
    paramIn(cp, "numEntries", num_entries);
    warn_if(num_entries > tagMetadataStore.size(), "Unserializing larger tag "
            "store into a smaller tag store. Stopping when index doesn't fit");
    warn_if(num_entries < tagMetadataStore.size(), "Unserializing smaller "
            "tag store into a larger tag store. Not fully warmed up.");

    for (int i = 0; i < num_entries; i++) {
        ScopedCheckpointSection sec_entry(cp,csprintf("Entry%d", i));

        bool valid = false;
        paramIn(cp, "validLine", valid);
        if (valid) {
            Addr tag = 0;
            Addr index = 0;
            bool dirty = false;
            Addr far_addr = 0;
            paramIn(cp, "tagDC", tag);
            paramIn(cp, "indexDC", index);
            paramIn(cp, "dirtyLine", dirty);
            paramIn(cp, "farMemAddr", far_addr);
            if (index < tagMetadataStore.size()) {
                // Only insert if this entry fits into the current store.
                tagMetadataStore[index].tagDC = tag;
                tagMetadataStore[index].indexDC = index;
                tagMetadataStore[index].validLine = valid;
                tagMetadataStore[index].dirtyLine = dirty;
                tagMetadataStore[index].farMemAddr = far_addr;
            }
        }
    }
}

bool
PolicyManager::recvReadFlushBuffer(Addr addr)
{
    if (pktFarMemWrite.size() < (orbMaxSize / 2)) {
        handleDirtyCacheLine(addr);
        return true;
    }
    return false;
}

} // namespace memory
} // namespace gem5
