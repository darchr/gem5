/* Copyright (c) 2016 Mark D. Hill and David A. Wood
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

#include "mem/dram_cache/dram_cache_ctrl.hh"

#include "debug/DRAMCache.hh"
#include "debug/Drain.hh"
#include "enums/MemoryMode.hh"
#include "mem/dram_ctrl.hh"
#include "sim/system.hh"

DRAMCacheCtrl::DRAMCacheCtrl(Params* p) :
    MemObject(p), system(p->system),
    memSidePort(name() + "-memSide", *this),
    slavePort(name() + "-slavePort", *this),
    invPort(name() + "-invPort", *this),
    addrRanges(p->addr_ranges.begin(), p->addr_ranges.end()),
    demandID(system->getMasterId(name() + ".demand")),
    otherID(system->getMasterId(name() + ".other")),
    dirtyList(p->dirty_list), storageCtrl(p->storage_ctrl),
    writebackPolicy(p->writeback_policy),
    sendBackprobes(p->send_backprobes),
    maxOutstanding(p->max_outstanding),
    memSideBlocked(false), memSideBlockedPkt(nullptr),
    memSideBlockedPkt2(nullptr), retrySecondBlockedPacketEvent(this),
    drainWritebacksWaitingForMemEvent(this),
    storageCtrlBlocked(false), storageNeedsUnblock(false),
    trySendStorageBlockedEvent(this),
    size(p->size), banks(p->banks),blockSize(p->block_size),
    dirtyListLatency(10), outstandingRequestQueueSize(0),
    memSideBlockedCycle(Cycles(0)), bankNumber(p->bank_number),
    checkData(p->check_data), dramctrl(p->dram_ctrl)
{
    bankLines = size / blockSize;
}

void
DRAMCacheCtrl::init()
{
    if (!slavePort.isConnected() || !memSidePort.isConnected()) {
        fatal("Ports on %s are not connected\n", name());
    }
    slavePort.sendRangeChange();

    storageCtrl->registerController(this);

    if (dirtyList) {
        dirtyList->registerDirtyList(this, bankNumber);
    }
}

BaseMasterPort&
DRAMCacheCtrl::getMasterPort(const std::string& if_name, PortID idx)
{
    if (if_name == "mem_side") {
        return memSidePort;
    } else if (if_name == "invalidation_port"){
        return invPort;
    } else {
        return MemObject::getMasterPort(if_name, idx);
    }
}

BaseSlavePort&
DRAMCacheCtrl::getSlavePort(const std::string& if_name, PortID idx)
{
    if (if_name == "cpu_side") {
        return slavePort;
    } else {
        return MemObject::getSlavePort(if_name, idx);
    }
}

bool
DRAMCacheCtrl::recvTimingResp(PacketPtr pkt)
{
    // Just forward this along. Really this shouldn't come through this
    // controller.
    DPRINTF(DRAMCache, "Forwarding response to %s\n", pkt->print());

    checkCanonicalData(pkt);

    // When running in KNL mode, it's possible this response is for an Upgrade
    auto it = outstandingRefillForUpgrades.find(pkt->getAddr());
    if (it != outstandingRefillForUpgrades.end()) {
        // This is an upgrade! Copy the data to the original packet and send
        // that one instead of this one.
        PacketPtr orig_pkt = it->second;
        DPRINTF(DRAMCache, "Got response upgrade! %s\n", orig_pkt->print());
        if (orig_pkt->isRead()) {
            // Only need to copy if the request was a read.
            orig_pkt->setData(pkt->getConstPtr<uint8_t>());
        }
        // Copy the responding packet to old_pkt because we need to delete it.
        PacketPtr old_pkt = pkt;
        // This is the packet we need to respond to.
        pkt = orig_pkt;
        pkt->makeTimingResponse();
        DPRINTF(DRAMCache, "Sending snoop response of %s\n", pkt->print());
        // Cleanup our mess
        delete old_pkt->req;
        delete old_pkt;
        outstandingRefillForUpgrades.erase(it);
        invPort.schedTimingSnoopResp(pkt, nextCycle());
    } else {
        slavePort.schedTimingResp(pkt, nextCycle());
    }

    return true;
}

bool
DRAMCacheCtrl::recvInvResp(PacketPtr pkt)
{
    DPRINTF(DRAMCache, "Got inv response for %s\n", pkt->print());

    // If the packet has dirty data, write it through to memory. Obviously,
    // this could fail.
    if (pkt->hasDirtyDataFromCache()) {
        if (memSideBlocked) {
            DPRINTF(DRAMCache, "Memory blocked. Inv. recv failed\n");
            invPort.setWaitingForRetry();
            return false;
        }
        DPRINTF(DRAMCache, "Writing data back to memory\n");
        // Create a deep copy of the packet to write through to mem
        RequestPtr req = new Request(pkt->getAddr(), pkt->getSize(),
                                     0, 0);
        PacketPtr wb_pkt = new Packet(req, MemCmd::WritebackDirty);
        wb_pkt->dataDynamic(new uint8_t[blockSize]);
        wb_pkt->setData(pkt->getConstPtr<uint8_t>());

        // Update what we think the right data is.
        updateCanonicalData(pkt);

        sendMemSideReq(wb_pkt); // Note: If this fails, memory port will deal.
    }

    if (pkt->hasSharers()) {
        DPRINTF(DRAMCache, "Still on chip, so saving to dir!\n");
        fakeDirectory.insert(pkt->getAddr());
    }

    outstandingInvalidates.erase(pkt->getAddr());

    auto it = racyProbes.find(pkt->getAddr());
    if (it != racyProbes.end()) {
        DPRINTF(DRAMCache, "Resending a racy req: %s\n", it->second->print());
        bool res = recvTimingReq(it->second, false);
        if (!res) {
            DPRINTF(DRAMCache, "Putting %#x on queue.\n", it->second);
            internalQueuedPackets.push(it->second);
        }
        racyProbes.erase(it);
    }

    // Just ignore it now that it's done.
    delete pkt->req;
    delete pkt;

    return true;
}

void
DRAMCacheCtrl::recvInvSnoop(PacketPtr pkt)
{
    DPRINTF(DRAMCache, "Got a snoop request for %s\n", pkt->print());
    if (pkt->cmd == MemCmd::ReadSharedReq) {
        // Just a read.
        return;
    }

    chatty_assert(pkt->cmd == MemCmd::UpgradeReq ||
                      pkt->cmd == MemCmd::ReadExReq,
                  "I'm assuming only upgrades");

    auto it = fakeDirectory.find(pkt->getAddr());
    if (it == fakeDirectory.end()) {
        return; // No need to do anything.?
    }

    DPRINTF(DRAMCache, "Found a snoop entry\n");

    refillOnUpgrade++;

    // Either we're now going to fill the DRAM cache, or it's actually there.
    // Either way, there is no need to track this anymore.
    fakeDirectory.erase(it);

    // This is a bit of a hack. There is a possible race condition where we
    // think the address isn't in the cache, but it actually is. This isn't a
    // correctness issue, just a performance one. Let's try just ignoring it.
    if (storageCtrl->checkHit(pkt, getStorageAddr(pkt->getAddr()), false)) {
        DPRINTF(DRAMCache, "actually, this is in the cache. huh\n");
        return;
    }

    // Say that "we" are responding, but it's going to be a long time before
    // we can respond because we first need to refill the DRAM cache.
    pkt->setCacheResponding();

    assert(outstandingRefillForUpgrades.find(pkt->getAddr()) ==
                outstandingRefillForUpgrades.end());

    // Create a new packet so we can respond later. This seems dumb...
    PacketPtr resp_pkt = new Packet(pkt, false, pkt->isRead());

    // Add this packet to the following to track it until we're done.
    outstandingRefillForUpgrades[pkt->getAddr()] = resp_pkt;

    // Create a new packet so we can re-fill the DRAM cache.
    RequestPtr req = new Request(pkt->getAddr(), pkt->getSize(), 0,
                         otherID);
    PacketPtr new_pkt = new Packet(req, MemCmd::ReadReq);
    new_pkt->allocate();

    DPRINTF(DRAMCache, "Sending new packet %s to the DRAM cache\n",
                       new_pkt->print());
    bool res = recvTimingReq(new_pkt, false);
    if (!res) {
        DPRINTF(DRAMCache, "Putting %#x on queue.\n", new_pkt);
        internalQueuedPackets.push(new_pkt);
    }
}

void
DRAMCacheCtrl::recvReqRetry()
{
    DPRINTF(DRAMCache, "Got request retry (memory)\n");

    assert(memSideBlocked);

    memSideBlocked = false;

    assert(memSideBlockedPkt != nullptr);
    PacketPtr pkt;
    if (memSideBlockedPkt2 != nullptr) {
        DPRINTF(DRAMCache, "Trying to send pkt2\n");
        pkt = memSideBlockedPkt2;
        memSideBlockedPkt2 = nullptr;
    } else {
        DPRINTF(DRAMCache, "Trying to send pkt1\n");
        pkt = memSideBlockedPkt;
        memSideBlockedPkt = nullptr;
    }

    DPRINTF(DRAMCache, "Retrying (%p) %s\n", pkt, pkt->print());

    if (sendMemSideReq(pkt)) {
        DPRINTF(DRAMCache, "Packet (%p) retry successfully sent."
            " Popping from waiting.\n", pkt);
        if (memSideBlockedPkt != nullptr) {
            // Memory is still blocked, but we have to schedule a time for this
            // to be called.
            DPRINTF(DRAMCache, "Pkt2 successful, pkt1 still pending.\n");
            memSideBlocked = true;
            schedule(retrySecondBlockedPacketEvent, nextCycle());
        } else {
            // Since this was successful, let's drain anything that's waiting
            if (!writebacksWaitingForMem.empty() &&
                    !drainWritebacksWaitingForMemEvent.scheduled()) {
                DPRINTF(DRAMCache, "Queuing dirty list drain event\n");
                schedule(drainWritebacksWaitingForMemEvent, nextCycle());
            }
            // We also may need to notify the storage controller
            if (storageNeedsUnblock) {
                DPRINTF(DRAMCache, "Sending storage a retry\n");
                storageNeedsUnblock = false;
                storageCtrl->retryResponse();
            }
            if (sendBackprobes) {
                slavePort.trySendRetry();
                invPort.trySendRetry();
            }
        }
    } else {
        DPRINTF(DRAMCache, "Packet (%p) retry unsuccessfully sent.\n", pkt);
    }
}

void
DRAMCacheCtrl::drainWritebacksWaitingForMem()
{
    if (memSideBlocked) {
        // It's possible that the storage controller's response causes memory
        // to be blocked before this gets to execute. If so, memory is blocked
        // again, so this will be scheduled again sometime in the future.
        return;
    }
    assert(!writebacksWaitingForMem.empty());

    PacketPtr pkt = writebacksWaitingForMem.front();
    writebacksWaitingForMem.pop();
    DPRINTF(DRAMCache, "Packet %p popping from waiting.\n", pkt);

    DPRINTF(DRAMCache, "Reprocess failed (%p) %s\n", pkt, pkt->print());
    if (sendMemSideReq(pkt)) {
        // This writeback was successful, so remove it from the queue.
        // NOTE: This queue should only be used for incoming requests, not
        //       writebacks from storage responses.
        DPRINTF(DRAMCache, "%d left in the queue\n",
                           writebacksWaitingForMem.size());
        if (!writebacksWaitingForMem.empty()) {
            schedule(drainWritebacksWaitingForMemEvent, nextCycle());
        } else {
            // We are fully done draining. Remember that we could have stalled
            // the storage responses because of this queue, so let's check to
            // see if we need to send a retry
            if (storageNeedsUnblock) {
                DPRINTF(DRAMCache, "Sending storage a retry from drain WB\n");
                storageNeedsUnblock = false;
                storageCtrl->retryResponse();
            }
        }
    } else {
        DPRINTF(DRAMCache, "Packet failed to memory again\n");
    }
    // Whether or not it succeeds, decrement. If it fails, it's held in the
    // special buffer and will be sent before anything else.
    DPRINTF(DRAMCache, "(D)Decr (%d)\n", outstandingRequestQueueSize);
    decrementOutstandingQueue();
}

void
DRAMCacheCtrl::retryStorageReq()
{
    DPRINTF(DRAMCache, "Got request retry (storage)\n");
    assert(storageCtrlBlocked);

    storageCtrlBlocked = false;

    trySendStorageBlockedHead();
}

// FROM THE CPU SIDE

void
DRAMCacheCtrl::recvFunctional(PacketPtr pkt)
{
    DPRINTF(DRAMCache, "Functional access %s\n", pkt->print());

    Addr block_addr = blockAlign(pkt->getAddr());
    Addr storage_addr = getStorageAddr(block_addr);

    // This updates the DRAM cache storage
    storageCtrl->functionalAccess(pkt, storage_addr);

    if (pkt->isWrite() || !pkt->isResponse()) {
        DPRINTF(DRAMCache, "Functional write forwarded\n");
        memSidePort.sendFunctional(pkt);
    }

    if (pkt->isWrite()) {
        updateCanonicalData(pkt);
    }

    DPRINTF(DRAMCache, "Functional data 0x%llx\n", *pkt->getPtr<uint64_t>());
}

Tick
DRAMCacheCtrl::recvAtomic(PacketPtr pkt)
{
    DPRINTF(DRAMCache, "Atomic access %s\n", pkt->print());
    if (pkt->isWrite()) {
        updateCanonicalData(pkt);
    }

    if (system->getMemoryMode() == Enums::atomic_noncaching) {
        // assert(!checkHit(block_addr));
        return memSidePort.sendAtomic(pkt);
    }

    Addr block_addr = blockAlign(pkt->getAddr());
    Addr storage_addr = getStorageAddr(block_addr);

    // The cache should track whether or not the line has been cached in the
    // DRAM cache, but the classic cache model sucks. So, we are going to just
    // cheat here and drop any clean writebacks that hit in the cache. This
    // greatly simplifies the cache logic.
    if (pkt->cmd == MemCmd::WritebackClean &&
            storageCtrl->checkHit(pkt, getStorageAddr(pkt->getAddr()), false)){
        // This should never have been issued here. just say we handled it.
        cleanSinks++;
        return 1;
    }

    Tick latency = 0;
    bool need_to_delete = false;
    if (pkt->isWrite()) {
        if (pkt->getSize() != blockSize) {
            // small access. Need to get the actual data from backing store
            // Shouldn't be dirty in the storage
            DPRINTF(DRAMCache, "Need to get data from mem for partial req\n");
            assert(!dirtyList->checkDirty(block_addr));
            RequestPtr req = new Request(block_addr, pkt->getSize(), 0,
                                         otherID);
            PacketPtr dram_pkt = new Packet(req, MemCmd::ReadReq);
            dram_pkt->allocate();
            memSidePort.sendAtomic(dram_pkt);

            RequestPtr storage_req = new Request(block_addr, pkt->getSize(),
                                         0, otherID);
            PacketPtr storage_pkt = new Packet(storage_req, MemCmd::ReadReq);
            storage_pkt->allocate();
            storage_pkt->setData(dram_pkt->getConstPtr<uint8_t>());

            delete dram_pkt;
            delete req;

            // write the data from the incoming packet into the new packet
            // that has the full data from memory.
            pkt->writeDataToBlock(storage_pkt->getPtr<uint8_t>(), blockSize);

            // We need to delete this packet later
            need_to_delete = true;

            // Set the original pkt ptr to this ptr so we can do the actual
            // write below.
            pkt = storage_pkt;
        }
        bool mark_dirty = true;
        latency += dirtyListLatency;
        if (!dirtyList || !dirtyList->checkSafe(block_addr)) {
            DPRINTF(DRAMCache, "Hit in dirty list. Need to writeback\n");
            // need to write this data back.
            RequestPtr req = new Request(block_addr, pkt->getSize(), 0,
                                         otherID);
            PacketPtr replace_pkt = new Packet(req, MemCmd::ReplaceReq);
            replace_pkt->allocate();

            // Get the replace packet's data from storage
            bool hit = storageCtrl->accessAtomic(replace_pkt, storage_addr,
                                                 latency);
            if (!hit) {
                // If it was a miss, do the writeback. Here, miss means that
                // the tag in the cache does not match the address of the
                // original packet
                if (replace_pkt->getAddr() == MaxAddr) {
                    DPRINTF(DRAMCache, "Cold miss\n");
                    delete req;
                    delete replace_pkt;
                } else {
                    // Need to send backprobe to the cache for this address!
                    if (sendBackprobes) {
                        sendAtomicBackprobe(replace_pkt->getAddr());
                    }

                    if (replace_pkt->hasDirtyDataFromCache()) {
                        DPRINTF(DRAMCache, "Found dirty data. Need to WB %s\n",
                                           replace_pkt->print());
                       // This is now clean!
                       if (dirtyList) {
                           dirtyList->removeDirty(replace_pkt->getAddr());
                       }
                       storageCtrl->markBlockClean(replace_pkt, storage_addr);

                       replace_pkt->makeResponse();
                       // Write back the data
                       latency += memSidePort.sendAtomic(replace_pkt);
                   } else {
                       // Miss to a clean line. No need to writeback.
                       assert(!dirtyList->checkDirty(pkt->getAddr()));
                       DPRINTF(DRAMCache, "Miss to clean line\n");
                       delete req;
                       delete replace_pkt;
                   }
               }
            } else {
                // Either it was a hit, or it was a cold miss, so there is
                // no need to write anything back to main-memory
                DPRINTF(DRAMCache, "Hit on replace! No need to WB\n");
                panic_if(pkt->cmd == MemCmd::WritebackClean,
                         "Clean WBs should never be found in the cache!\n");
                if (pkt->hasDirtyDataFromCache()) {
                    // If this was dirty, then don't mark it dirty again!
                    mark_dirty = false;
                }
                delete req;
                delete replace_pkt;
            }
        }

        if (pkt->cmd == MemCmd::WritebackDirty &&
            !canInsertDL(block_addr))
        {
            // This is weird, but this actually cleans the victim (in atomic
            // mode). I don't feel like doing this the right way
            assert(dirtyList);
            dirtyList->getVictim(block_addr);
            assert(dirtyList->canInsert(block_addr));
        }

        DPRINTF(DRAMCache, "Writing data to storage %s\n", pkt->print());

        // Now the block is clean in the cache so we can overwrite it
        storageCtrl->accessAtomic(pkt, storage_addr, latency);

        // If we wrote back dirty data, mark it such.
        if (dirtyList && pkt->cmd == MemCmd::WritebackDirty) {
            assert(writebackPolicy != Enums::writethrough);
            if (dirtyList->checkFull(block_addr)) {
                // NOTE: Not tested for a while now. May not work.
                // Must write something back now.
                Addr victim = dirtyList->getVictim(block_addr);
                RequestPtr req = new Request(victim, blockSize, 0,
                                             otherID);
                PacketPtr replace_pkt = new Packet(req, MemCmd::ReplaceReq);
                replace_pkt->allocate();
                Addr victim_storage_addr = getStorageAddr(victim);

                // Get the replace packet's data from storage
                bool hit = storageCtrl->accessAtomic(replace_pkt,
                                                victim_storage_addr, latency);
                // This better be the tag in the cache or something has gone
                // terribly wrong!
                if (!hit) {
                    panic("Line should always be in the cache");
                }
                replace_pkt->makeResponse();

                DPRINTF(DRAMCache, "Writing back %s\n", replace_pkt->print());

                // Write back the data
                latency += memSidePort.sendAtomic(replace_pkt);

                // This is now clean!
                dirtyList->removeDirty(victim);
            }
            if (mark_dirty && !dirtyList->checkDirty(block_addr)) {
                dirtyList->markDirty(block_addr);
            }
        }

        // If we are a writethrough cache, then update the backing store.
        if (writebackPolicy == Enums::writethrough) {
            latency += memSidePort.sendAtomic(pkt);
        }

        if (need_to_delete) {
            delete pkt->req;
            delete pkt;
        }

    } else if (pkt->isRead()) {

        bool hit = storageCtrl->accessAtomic(pkt, storage_addr, latency);
        DPRINTF(DRAMCache, "Storage returned a %s for read\n",
                hit ? "hit" : "miss");
        if (hit) {
            panic_if(pkt->cmd == MemCmd::WritebackClean,
                     "Shouldn't hit for clean wb!");
            // Proactively clean the data. Currently, we're always cleaning
            // This means that when we start the detailed simlulation we'll
            // have more of the cache clean than may be realistic.
            if (writebackPolicy == Enums::adaptive &&
                    dirtyList && pkt->hasDirtyDataFromCache()) {
                // Make a copy of the packet to write back to memory
                RequestPtr req = new Request(pkt->getAddr(), pkt->getSize(),
                                             0, 0);
                PacketPtr wb_pkt = new Packet(req, MemCmd::WritebackDirty);
                wb_pkt->dataDynamic(new uint8_t[blockSize]);
                wb_pkt->setData(pkt->getConstPtr<uint8_t>());

                proactiveCleans++;
                // Mark this line as clean now.
                dirtyList->removeDirty(block_addr);

                DPRINTF(DRAMCache, "Sending proactive clean WB to memory\n");
                memSidePort.sendAtomic(wb_pkt);
                storageCtrl->markBlockClean(wb_pkt,
                                            getStorageAddr(pkt->getAddr()));
            }
            // Set that this packet's response was generated by a cache so we
            // can potential filter out useless clean writebacks later
            pkt->setResponseFromCache();
            pkt->makeResponse();
        } else {
            latency += memSidePort.sendAtomic(pkt);
        }
        checkCanonicalData(pkt);

    } else {
        panic("Unknown packet type");
    }

    return latency;
}

void
DRAMCacheCtrl::atomicClean(Addr start_addr, int regionSize)
{
    for (Addr addr = start_addr;
         addr < start_addr + regionSize;
         addr += blockSize * banks)
    {
        DPRINTF(DRAMCache, "Atomic clean for %#x\n", addr);
        // need to write this data back.
        RequestPtr req = new Request(addr, blockSize, 0, otherID);
        PacketPtr replace_pkt = new Packet(req, MemCmd::ReplaceReq);
        replace_pkt->allocate();

        Tick latency = 0;
        // Get the replace packet's data from storage
        storageCtrl->accessAtomic(replace_pkt, getStorageAddr(addr), latency);
        if (replace_pkt->hasDirtyDataFromCache()) {
            DPRINTF(DRAMCache, "Writing back to DRAM; cleaning miss\n");
            cleaningReplaceDirty++;
            dirtyList->removeDirty(replace_pkt->getAddr());
            // makeResponse will turn this into a writeback request.
            replace_pkt->makeResponse();
            // Note: this pkt's address is the address of the data that
            //       it is holding. I.e., the address of the victim
            memSidePort.sendAtomic(replace_pkt);
            storageCtrl->markBlockClean(replace_pkt, getStorageAddr(addr));
        } else {
            cleaningReplaceWasted++;
            assert(!dirtyList->checkDirty(replace_pkt->getAddr()));
            delete req;
            delete replace_pkt;
        }
    }

}

bool
DRAMCacheCtrl::recvTimingReq(PacketPtr pkt, bool from_cache)
{
    DPRINTF(DRAMCache, "%d outstanding. Got request %s\n",
        outstandingRequestQueueSize, pkt->print());

    // If we have any internal packets that previously failed to send, try to
    // send those first before servicing external requests. I hope this doesn't
    // cause any deadlocks.
    bool servicing_queued_packet = false;
    PacketPtr original_request_packet = pkt;
    if (!internalQueuedPackets.empty()) {
        pkt = internalQueuedPackets.front();
        DPRINTF(DRAMCache, "Servicing internal %#x (%s) first. %d left\n",
                pkt, pkt->print(), internalQueuedPackets.size());
        servicing_queued_packet = true;
    }

    auto it = outstandingInvalidates.find(pkt->getAddr());
    if (it != outstandingInvalidates.end()) {
        panic_if(servicing_queued_packet, "Shouldn't do this?\n");
        DPRINTF(DRAMCache, "Got request that matches invalidate %#x\n",
                          pkt->getAddr());
        // This packet matches something in the probe buffer. There are two
        // possibilities. 1) this is the same packet, in which case we should
        // reply that the probe was successful (nothing to invalidate). 2) this
        // is a different packet which means we're racing. For Racy probes,
        // let's put them on the side and reply when we finally get the
        // response to our probe. Hopefully this doesn't cause deadlock!
        if (it->second != pkt) {
            DPRINTF(DRAMCache, "got a racy probe!\n");
            if (pkt->isWriteback()) {
                // If this is a writeback, just forward it to memory. This
                // *should* be ordered correctly...
                // if blocked, we can't process the request.
                if (memSideBlocked) {
                    DPRINTF(DRAMCache, "Can't accept the writeback\n");
                    // If this function is called from recvInv (e.g., there was
                    // a racy request w.r.t the probe) then we shouldn't set
                    // this. Only call retry on the slavePort if the request
                    // was actually from the cache.
                    if (from_cache) slavePort.setWaitingForRetry();
                    return false;
                }
                numRacyProbeWriteback++;
                DPRINTF(DRAMCache, "Forwarding WB to mem.\n");
                updateCanonicalData(pkt);
                sendMemSideReq(pkt);
                return true;
            } else {
                DPRINTF(DRAMCache, "Queuing this pkt for after inv (%p) %s\n",
                        pkt, pkt->print());
                numRacyProbes++;
                panic_if(racyProbes.find(pkt->getAddr()) != racyProbes.end(),
                         "Multiple requests for the same racy address???\n");
                racyProbes.insert({pkt->getAddr(), pkt});
                return true;
            }
        }
        uselessProbes++;
        // go ahead and erase this pkt since we know it's safe. We'll wait
        // to deal with the racy things until we get this response.
        outstandingInvalidates.erase(pkt->getAddr());
        pkt->makeResponse();
        slavePort.schedTimingResp(pkt, nextCycle());
        return true;
    }

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    panic_if(!(pkt->isRead() || pkt->isWrite()),
             "Should only see read and writes at memory-side cache\n");

    assert(pkt->getSize() == 64);
    assert(pkt->getAddr() == blockAlign(pkt->getAddr()));

    // The cache should track whether or not the line has been cached in the
    // DRAM cache, but the classic cache model sucks. So, we are going to just
    // cheat here and drop any clean writebacks that hit in the cache. This
    // greatly simplifies the cache logic.
    if (pkt->cmd == MemCmd::WritebackClean &&
            storageCtrl->checkHit(pkt, getStorageAddr(pkt->getAddr()), false)){
        // This should never have been issued here. just say we handled it.
        cleanSinks++;
        return true;
    }

    if (outstandingRequestQueueSize >= maxOutstanding) {
        DPRINTF(DRAMCache, "Outstanding request buffer full!\n");
        outstandingQueueBlockedCount++;
        assert(outstandingRequestQueueSize < maxOutstanding+2);
        slavePort.setWaitingForRetry();
        return false;
    }

    if (pkt->isWrite()) {
        updateCanonicalData(pkt);
    }

    // Add the packet to the request queue
    outstandingRequestQueueSize++;
    if (pkt->isWriteback()) {
        // if it's a writeback, then it may need to replace something, so
        // it occupies two slots
        outstandingRequestQueueSize++;
    }

    // For stats...
    if (pkt->isRead()) {
        readPackets++;
    } else if (pkt->isWrite()) {
        if (pkt->cmd == MemCmd::WritebackClean) {
            cleanWBPackets++;
        } else if (pkt->cmd == MemCmd::WritebackDirty) {
            dirtyWBPackets++;
        } else {
            panic("Unexpected write type");
        }
    } else {
        DPRINTF(DRAMCache, "Got bad packet %s\n", pkt->print());
        panic("Unexpected packet type");
    }

    // Both reads and writes must wait to check the dirty list since this needs
    // to process requests in order.
    Cycles lat = (dirtyList == nullptr) ? Cycles(1) : dirtyListLatency;
    schedule(new HandleRequestEvent(this, pkt), clockEdge(lat));

    if (servicing_queued_packet) {
        // We serviced the top packet. There may be more, though...
        internalQueuedPackets.pop();
        // We didn't service the incoming packet, so let's try it now.
        DPRINTF(DRAMCache, "Calling %s again for original pkt\n", __func__);
        return recvTimingReq(original_request_packet);
    }

    return true;
}

bool
DRAMCacheCtrl::checkBlockForReplacement(PacketPtr pkt)
{
    auto it = blockedForReplacement.find(getLineNumber(pkt->getAddr()));
    if (it != blockedForReplacement.end()) {
        // There is an outstanding replacement occuring for this line
        // We need to stall this request until it's done.
        DPRINTF(DRAMCache, "Putting (%p) on blocked queue at %#x, %s\n",
                pkt, getLineNumber(pkt->getAddr()), pkt->print());
        blockedPackets[getLineNumber(pkt->getAddr())].push_back(pkt);
        replacementBlockedCount++;
        return true;
    } else {
        // assert(blockedPackets.find(getLineNumber(pkt->getAddr())) ==
        //            blockedPackets.end());
        warn_if(blockedPackets.find(getLineNumber(pkt->getAddr())) !=
                    blockedPackets.end(),
            "Possible blocked packet consistency violation");
        return false;
    }
}

void
DRAMCacheCtrl::handleRead(PacketPtr pkt)
{
    DPRINTF(DRAMCache, "Handling a read request %s\n", pkt->print());
    missMapReadHit++;
    storageRead++;

    // There's a chance that an in-flight write is now blocking this req.
    // Do NOT need to check when we are draining blocked packets.
    if (checkBlockForReplacement(pkt)) {
        return;
    }

    // This is a new miss, so remove it from the fake directory.
    auto it = fakeDirectory.find(pkt->getAddr());
    if (it != fakeDirectory.end()) {
        DPRINTF(DRAMCache, "removing from fake directory\n");
        fakeDirectory.erase(it);
    }

    // Note: The outstanding queue is decremented whenever this is
    // is successful. Thus, this queuing is taken into account by that.
    trySendStorageReq(pkt);
}

void
DRAMCacheCtrl::handleWrite(PacketPtr pkt)
{
    DPRINTF(DRAMCache, "Handling a writes request %s\n", pkt->print());

    auto it = fakeDirectory.find(pkt->getAddr());
    if (it != fakeDirectory.end()) {
        DPRINTF(DRAMCache, "removing from fake directory\n");
        fakeDirectory.erase(it);
    }

    // There's a chance that an in-flight write is now blocking this req.
    // Do NOT need to check when we are draining blocked packets.
    if (checkBlockForReplacement(pkt)) {
        return;
    }

    Addr block_addr = blockAlign(pkt->getAddr());

    assert(pkt->isWrite());

    // if we get a writback clean, this means that the data is NOT in the cache
    if (pkt->cmd == MemCmd::WritebackClean &&
           storageCtrl->checkHit(pkt, getStorageAddr(pkt->getAddr()), false)) {
        panic("Should never see writebacl clean to lines in the cache");
    }

    // If we are using writethrough, or sending backprobes, or it's safe,
    // then there's no need to send a replace.
    if (writebackPolicy == Enums::writethrough ||
            sendBackprobes ||
            (dirtyList && dirtyList->checkSafe(block_addr))
            ) {
        dirtyListWriteMiss++;
        DPRINTF(DRAMCache, "Line is not dirty.\n");

        if (sendBackprobes) {
            // In this case, this should *always* be a hit in the cache.
            bool hit = storageCtrl->checkHit(pkt,
                                             getStorageAddr(pkt->getAddr()),
                                             false);
            panic_if(!hit, "Should always hit. %s not found in cache\n",
                     pkt->print());
        }

        if (handleWriteback(pkt)) {
            DPRINTF(DRAMCache, "Decr since no need to replace\n");
            DPRINTF(DRAMCache, "Decr (%d)\n", outstandingRequestQueueSize);
            decrementOutstandingQueue();
        }
    } else {
        // It's unsafe to write into the cache. Therefore, read first.
        // If it is unsafe to write: Either we can't tell since there's no
        // dirty list or the dirty list says it may be dirty.
        dirtyListWriteHit++;
        DPRINTF(DRAMCache, "Need to replace for packet %s\n", pkt->print());
        // Need to start a replacement
        sendReplace(pkt);
    }
}

void
DRAMCacheCtrl::sendReplace(PacketPtr orig_pkt)
{
    Addr block_addr = blockAlign(orig_pkt->getAddr());
    // Create a request/packet pair.
    RequestPtr req = new Request(block_addr, orig_pkt->getSize(), 0,
                                 otherID);
    PacketPtr replace_pkt = new Packet(req, MemCmd::ReplaceReq);
    replace_pkt->allocate();
    replace_pkt->pushSenderState(new ReplaceSenderState(orig_pkt, block_addr,
                                                        false));

    // Add this to a set of outstanding writes so we can defensively
    // check for any consistency issues
    // Note: No need to add it if we are already draining it
    auto it = blockedForReplacement.find(getLineNumber(block_addr));
    if (it == blockedForReplacement.end()) {
        DPRINTF(DRAMCache, "Adding %#x to stalled writes. line %#x\n",
            block_addr, getLineNumber(block_addr));
        blockedForReplacement.insert(getLineNumber(block_addr));
    }

    DPRINTF(DRAMCache, "Sending repl. to storage %p\n", replace_pkt);
    storageReadForRepl++;
    trySendStorageReq(replace_pkt);
}

bool
DRAMCacheCtrl::startCleaningReplace(Addr block_addr)
{
    DPRINTF(DRAMCache, "Trying to clean %#x\n", block_addr);

    if (outstandingRequestQueueSize >= maxOutstanding) {
        DPRINTF(DRAMCache, "Outstanding request buffer full!\n");
        outstandingQueueBlockedCleaningCount++;
        assert(outstandingRequestQueueSize < maxOutstanding+3);
        return false;
    }

    auto it = blockedForReplacement.find(getLineNumber(block_addr));
    if (it != blockedForReplacement.end()) {
        // IF there is already a replace request for this block, just die here
        // Maybe we'll do it some other time.
        cleaningReplaceBlockedForReplacement++;
        return true;
    } else {
        DPRINTF(DRAMCache, "Adding %#x to stalled writes. line %#x\n",
            block_addr, getLineNumber(block_addr));
        blockedForReplacement.insert(getLineNumber(block_addr));
    }

    outstandingRequestQueueSize++;

    RequestPtr req = new Request(block_addr, blockSize, 0,
                                 otherID);
    PacketPtr replace_pkt = new Packet(req, MemCmd::ReplaceReq);
    replace_pkt->allocate();
    replace_pkt->pushSenderState(new ReplaceSenderState(nullptr, block_addr,
                                                        true));

    DPRINTF(DRAMCache, "Sending cleaning repl. to storage %p\n", replace_pkt);
    storageReadForRepl++;
    cleaningReplaces++;
    trySendStorageReq(replace_pkt);

    return true;
}

bool
DRAMCacheCtrl::handleWriteback(PacketPtr pkt, bool mark_dirty)
{
    Addr block_addr = blockAlign(pkt->getAddr());

    // Assume that it will successfully send if we don't need to send to mem.
    bool mem_success = true;

    assert(pkt->cmd == MemCmd::WritebackDirty ||
           pkt->cmd == MemCmd::WritebackClean);
    if (pkt->cmd == MemCmd::WritebackDirty) {
        bool can_insert_dl = canInsertDL(block_addr);
        if (shouldWriteThrough(block_addr) || !can_insert_dl) {
            // Create a deep copy of the packet to write through to mem
            RequestPtr req = new Request(pkt->getAddr(), pkt->getSize(),
                                         0, 0);
            PacketPtr wb_pkt = new Packet(req, MemCmd::WritebackClean);
            wb_pkt->dataDynamic(new uint8_t[blockSize]);
            wb_pkt->setData(pkt->getConstPtr<uint8_t>());

            // Swap this with the cache packet.
            // This is needed to keep the clean/dirty data in the cache
            // up to date.
            PacketPtr tmp = pkt;
            pkt = wb_pkt;
            wb_pkt = tmp;

            // Need to remove this from the dirty list only if there is a tag
            // match and it was dirty in the cache.
            if (dirtyList &&
                (dirtyList->checkDirty(block_addr) || !mark_dirty)) {
                dirtyList->removeDirty(block_addr);
            }

            // Also, write-through to memory.
            DPRINTF(DRAMCache, "Writing through to memory.\n");
            writeThroughCount++;
            if (memSideBlocked) {
                DPRINTF(DRAMCache, "Queuing %p request for blocked memory\n",
                                   wb_pkt);
                writebacksWaitingForMem.push(wb_pkt);
                mem_success = false;
            } else {
                sendMemSideReq(wb_pkt);
            }
        } else {
            // Mark this line as dirty
            if (dirtyList && mark_dirty && !dirtyList->checkDirty(block_addr)){
                dirtyList->markDirty(block_addr);
            }
            DPRINTF(DRAMCache, "NOT writing through to memory %s\n",
                    pkt->print());
        }
    }

    // Either way, forward the write to the storage.
    DPRINTF(DRAMCache, "Writing to storage (%s).\n",
            pkt->cmd == MemCmd::WritebackClean ? "clean" : "dirty");
    if (pkt->cmd == MemCmd::WritebackClean) {
        storageCleanWrite++;
    } else {
        storageDirtyWrite++;
    }
    trySendStorageReq(pkt);

    return mem_success;
}

bool
DRAMCacheCtrl::shouldWriteThrough(Addr addr)
{
    if (writebackPolicy == Enums::writethrough) {
        return true;
    } else if (writebackPolicy == Enums::writeback) {
        return false;
    } else if (writebackPolicy == Enums::adaptive) {
        if (dirtyList && dirtyList->checkFull(addr)) {
            // the dirty list cannot accept another entry, so we have to
            // writethrough
            dirtyListFull++;
            return true;
        } else if (memSideBlocked) {
            // Never writethrough if memory is currently blocked
            adaptiveWritebackBlocked++;
            return false;
        } else if (curCycle() - memSideBlockedCycle < 50) {
            // Also, if this was blocked within 10 cycles, don't writethrough
            adaptiveWritebackRecentBlocked++;
            return false;
        } else if (dramctrl->getWriteQueueState()
                    == DRAMCtrl::OverLowThreshold) {
            // The DRAM controller is currently overwhelmed by writes
            // so we should NOT write through.
            adaptiveWritebackWrite++;
            return false;
        } else {
            // Writing through should be fine.
            return true;
        }
    } else {
        panic("Unknown writeback policy");
        return false;
    }

}

bool
DRAMCacheCtrl::sendMemSideReq(PacketPtr pkt)
{
    if (memSideBlocked) {
        DPRINTF(DRAMCache, "Blocking (%p) into pkt2! %s\n", pkt, pkt->print());
        // This means that we are in a strange corner case. What's happened is
        // that we got a replace miss (need to write it back), and also decided
        // that we want to write through. We couldn't have predicted this.
        // This better not happen more than once, though!
        assert(memSideBlockedPkt2 == nullptr);
        memSideBlockedPkt2 = pkt;
        // Memory will call retry before we can go on.
        return false;
    }
    if (!memSidePort.sendTimingReq(pkt)) {
        DPRINTF(DRAMCache, "Sending packet (%p) to memory failed\n", pkt);
        memSideBlockedCount++;
        memSideBlocked = true;
        memSideBlockedCycle = curCycle();
        if (memSideBlockedPkt != nullptr) {
            DPRINTF(DRAMCache, "Setting pkt2. I guess it failed twice\n");
            assert(memSideBlockedPkt2 == nullptr);
            memSideBlockedPkt2 = pkt;
        } else {
            memSideBlockedPkt = pkt;
        }
        return false;
    }
    return true;
}

void
DRAMCacheCtrl::trySendStorageReq(PacketPtr pkt)
{
    Addr block_addr = blockAlign(pkt->getAddr());
    Addr storage_addr = getStorageAddr(block_addr);

    // If the port is blocked, or there are others already waiting, put this
    // in the wait queue. We'll send it later
    if (storageCtrlBlocked || !waitingForStorage.empty()) {
        DPRINTF(DRAMCache, "Storage blocked! Stalling %p\n", pkt);
        DPRINTF(DRAMCache, "%d in wait queue\n",
                waitingForStorage.size());
        waitingForStorage.push(pkt);
        return;
    }

    if (sendStorageReq(pkt, storage_addr)) {
        DPRINTF(DRAMCache, "Packet %p to storage: %#x\n", pkt, storage_addr);
        // Note: be careful here. It's actually possible that pkt is deleted.
        // This is because if sendStorageReq had a write queue hit it calls
        // recvResponse in the same call stack
        DPRINTF(DRAMCache, "Decr (%d)\n", outstandingRequestQueueSize);
        decrementOutstandingQueue();
    } else {
        DPRINTF(DRAMCache, "Sending to storage failed. Queuing (%p) %s\n",
                pkt, pkt->print());
        waitingForStorage.push(pkt);
    }
}

bool
DRAMCacheCtrl::sendStorageReq(PacketPtr pkt, Addr storage_addr)
{
    assert(!storageCtrlBlocked);
    if (storageCtrl->checkQueueFull(pkt, storage_addr)) {
        // The queue needed is full, so this is blocked.
        storageBlockedCount++;
        storageCtrlBlocked = true;
        DPRINTF(DRAMCache, "Sending packet %p to storage failed\n", pkt);
        return false;
    }

    assert(outstandingRequestQueueSize >= 1);
    DPRINTF(DRAMCache, "Sending storage req. for %p. %d outstanding\n",
            pkt, outstandingRequestQueueSize);
    // There is room to insert this into the storage queues
    storageCtrl->recvTimingReq(pkt, storage_addr);

    return true;
}

void
DRAMCacheCtrl::trySendStorageBlockedHead()
{
    assert(!storageCtrlBlocked);
    assert(!waitingForStorage.empty());

    PacketPtr pkt = waitingForStorage.front();

    Addr block_addr = blockAlign(pkt->getAddr());
    Addr storage_addr = getStorageAddr(block_addr);

    DPRINTF(DRAMCache, "Trying to resend (%p) %s\n", pkt, pkt->print());
    if (sendStorageReq(pkt, storage_addr)) {
        DPRINTF(DRAMCache, "Packet %p successfully sent."
            " Popping from waiting.\n", pkt);
        DPRINTF(DRAMCache, "%d left in the queue\n",
                waitingForStorage.size());

        DPRINTF(DRAMCache, "BDecr (%d)\n", outstandingRequestQueueSize);
        decrementOutstandingQueue();

        waitingForStorage.pop();
        if (!waitingForStorage.empty()) {
            schedule(trySendStorageBlockedEvent,
                     clockEdge(Cycles(1)));
        }
    }
    // If it fails, we just keep the pkt on the head of the queue.
    // We don't need to schedule another event, because we know that
    // the storage port will send a retry.
}

bool
DRAMCacheCtrl::canRecvStorageResp(PacketPtr pkt, bool hit, bool dirty)
{
    // This is needed since we poll when there is a write queue hit
    if (storageNeedsUnblock) {
        return false;
    }
    DPRINTF(DRAMCache, "Checking if can respond for %s to %s\n",
            hit ? "hit" : "miss", pkt->print());

    if (memSideBlocked || !writebacksWaitingForMem.empty()) {
        DPRINTF(DRAMCache, "memory is blocked\n");
        if (pkt->cmd == MemCmd::ReplaceReq) {
            ReplaceSenderState *state =
                    dynamic_cast<ReplaceSenderState *>(pkt->senderState);
            assert(state);
            if (dirty) {
                // If this is a replace, and a miss, and the data is dirty,
                // then we need to check to send a memory request.
                DPRINTF(DRAMCache, "Must stall for dirty miss\n");
                storageNeedsUnblock = true;
                responseBlockedCount++;
                return false;
            } else if (!state->cleanReplace &&
                       state->pkt->cmd == MemCmd::WritebackDirty &&
                       (shouldWriteThrough(state->pkt->getAddr()) ||
                           !canInsertDL(state->pkt->getAddr())))
            {
                // This is when we need to write through to memory.
                // Note: this and the above could both be true... then we may
                // be screwed. It may be OK to add one more buffer
                DPRINTF(DRAMCache, "Must stall for writethrough\n");
                storageNeedsUnblock = true;
                responseBlockedCount++;
                return false;
            }
            DPRINTF(DRAMCache, "None of these things happened!\n");
        } else if (!hit) {
            // if it is a read miss, need to forward to DRAM
            DPRINTF(DRAMCache, "Must stall for read miss\n");
            storageNeedsUnblock = true;
            responseBlockedCount++;
            return false;
        }
    }
    DPRINTF(DRAMCache, "Safe...\n");

    // All other cases we don't need memory, so it's ok to recv. the response
    return true;
}

void
DRAMCacheCtrl::recvStorageResponse(PacketPtr pkt, bool hit)
{
    DPRINTF(DRAMCache, "Response from storage (%s) for pkt (%p) %s\n",
            hit ? "hit" : "miss", pkt, pkt->print());
    // Only reads should respond
    assert(pkt->isRead());

    if (pkt->cmd == MemCmd::ReplaceReq) {
        assert(writebackPolicy != Enums::writethrough);
        ReplaceSenderState *state =
            dynamic_cast<ReplaceSenderState *>(pkt->popSenderState());
        assert(state);
        PacketPtr orig_pkt = state->pkt;
        Addr orig_addr = state->origAddr;
        bool clean_replace = state->cleanReplace;
        delete state;
        if (clean_replace) {
            DPRINTF(DRAMCache, "Got replace for clean request %#x\n",
                    orig_addr);
            assert(dirtyList);
            // This was a cleaning replace. We handle it a little differently.
            unblockReplace(blockAlign(orig_addr));
            if (pkt->hasDirtyDataFromCache()) {
                DPRINTF(DRAMCache, "Writing back to DRAM; cleaning miss\n");
                cleaningReplaceDirty++;
                dirtyList->removeDirty(pkt->getAddr());
                // makeResponse will turn this into a writeback request.
                pkt->makeResponse();
                // Note: this pkt's address is the address of the data that
                //       it is holding. I.e., the address of the victim
                sendMemSideReq(pkt);

                // Also need to mark it as clean in the cache
                storageCtrl->queueEmptyWrite(pkt,
                                             getStorageAddr(orig_addr));
                storageCtrl->markBlockClean(pkt,
                                             getStorageAddr(orig_addr));
            } else {
                cleaningReplaceWasted++;
                assert(!dirtyList->checkDirty(pkt->getAddr()));
                delete pkt->req;
                delete pkt;
            }
            return;
        }
        assert(orig_pkt);
        DPRINTF(DRAMCache, "Response to repl. for orig pkt %s\n",
                orig_pkt->print());
        assert(outstandingRequestQueueSize >= 1);
        // The storage controller will set responderHadWritable only if the tag
        // was marked as dirty before. If it is clean, there's no need to write
        bool mark_dirty = true;
        if (!hit) {
            if (pkt->getAddr() == MaxAddr) {
                // This is a cold miss, so there's no need to "write it back"
                realReplaceColdMiss++;
                DPRINTF(DRAMCache, "Cold replace miss, no need to write\n");
                delete pkt->req;
                delete pkt;
            } else {
                // Here, we need to send a backprobe to the cache!
                // May not writeback if there is a WB request in the stalled Q
                bool skip_wb = false;
                if (sendBackprobes) {
                    skip_wb = sendBackprobe(pkt->getAddr());
                }

                if (pkt->hasDirtyDataFromCache()) {
                    if (dirtyList) {
                        // Note: If tracking perfectly, then can just use
                        // checkDirty. If it's dirty in the cache, then we
                        //  should know this in the dirty list too.
                        assert(dirtyList->checkDirty(pkt->getAddr()) ||
                               !dirtyList->checkSafe(pkt->getAddr()));
                    }
                    realReplaceMiss++;
                    // Remove this from the dirty list; we are cleaning
                    if (dirtyList) {
                        dirtyList->removeDirty(pkt->getAddr());
                    }
                    // Note: could remove this from the miss map now
                    //    this currently doesn't make sense because the missmap
                    //    should be immediately updated in handleWriteback

                    DPRINTF(DRAMCache, "Writing back to DRAM; was a miss\n");
                    // makeResponse will turn this into a writeback request.
                    pkt->makeResponse();
                    // Note: this pkt's address is the address of the data that
                    //       it is holding. I.e., the address of the victim
                    if (!skip_wb) {
                        sendMemSideReq(pkt);
                    }
                } else {
                    // We tried to replace a line that was clean, just statitic
                    if (dirtyList) {
                        assert(!dirtyList->checkDirty(pkt->getAddr()));
                    }
                    realReplaceCleanHit++;
                    DPRINTF(DRAMCache, "Tried to replace clean line, skip\n");
                    delete pkt->req;
                    delete pkt;
                }
            }
        } else {
            panic_if(orig_pkt->cmd == MemCmd::WritebackClean,
                     "Clean WBs should never be found in the cache!\n");
            if (pkt->hasDirtyDataFromCache()) {
                // Important: Also don't need to mark as dirty if it's was
                // already dirty in the cache. Note: it's possible that it was
                // previously clean in the cache. In this case, we DO need to
                // add it to the dirty list!
                DPRINTF(DRAMCache, "Not updating dirty list (dirty hit)\n");
                mark_dirty = false;
            }
            // It was actually already there, so no need to do anything.
            realReplaceHit++;
            delete pkt->req;
            delete pkt;
        }
        // Now, we can send the write on with no problem
        if (!handleWriteback(orig_pkt, mark_dirty)) {
            // Something was added to the memory queue, add to outstanding
            outstandingRequestQueueSize++;
            assert(outstandingRequestQueueSize < maxOutstanding+3);
        }

        unblockReplace(blockAlign(orig_pkt->getAddr()));

        return;
    }

    if (hit) {
        realReadHit++;
        // great! Now we should respond to the request.

        // response_time consumes the static latency and is charged also
        // with headerDelay that takes into account the delay provided by
        // the xbar and also the payloadDelay that takes into account the
        // number of data beats.
        Tick response_time = curTick() + pkt->headerDelay + pkt->payloadDelay;

        // Here we reset the timing of the packet before sending it out.
        pkt->headerDelay = pkt->payloadDelay = 0;

        checkCanonicalData(pkt);

        // If this block is in the dirty list, then we can proactively
        // clean it so we don't have to read it at some later time.
        if (dirtyList &&  pkt->hasDirtyDataFromCache()) {
            couldClean++;
            // Probably shouldn't try to end it if the memory is already
            // blocked, but we probably should if the dirty list is full
            // This is an interesting policy choice
            if (shouldWriteThrough(pkt->getAddr())) {
                proactiveCleans++;
                // Create a deep copy of the packet to write through to mem
                RequestPtr req = new Request(pkt->getAddr(), pkt->getSize(),
                                             0, 0);
                PacketPtr wb_pkt = new Packet(req, MemCmd::WritebackDirty);
                wb_pkt->dataDynamic(new uint8_t[blockSize]);
                wb_pkt->setData(pkt->getConstPtr<uint8_t>());

                // Mark this line as clean now.
                dirtyList->removeDirty(pkt->getAddr());

                // Also have to update the tag that we did this proactive WB
                storageCtrl->queueEmptyWrite(wb_pkt,
                                             getStorageAddr(pkt->getAddr()));
                storageCtrl->markBlockClean(wb_pkt,
                                             getStorageAddr(pkt->getAddr()));

                DPRINTF(DRAMCache, "Sending proactive clean WB to memory\n");
                sendMemSideReq(wb_pkt);
            } else {
                // Not proactively cleaning. Update the dirty list tags with
                // this tag. TODO
            }
        }

        assert(pkt->needsResponse());
        pkt->makeResponse();

        // Set that this packet's response was generated by a cache so we
        // can potential filter out useless clean writebacks later
        pkt->setResponseFromCache();

        // queue the packet in the response queue to be sent out after
        // the static latency has passed
        slavePort.schedTimingResp(pkt, response_time, true);
    } else {
        realReadMiss++;
        // Since this was a miss, we need to forward it to memory.
        // NOTE: for now this could buffer an infinite number of things
        //       We currently don't track anything here.
        sendMemSideReq(pkt);
    }
}

void
DRAMCacheCtrl::unblockReplace(Addr block_addr) {
    // Check to see if there are any packets blocked on this requests
    // and schedule an event to drain them if there are.
    DPRINTF(DRAMCache, "Trying to unblock for %#x. Lno: %#x\n", block_addr,
                       getLineNumber(block_addr));
    auto it = blockedPackets.find(getLineNumber(block_addr));
    if (it != blockedPackets.end()) {
        DPRINTF(DRAMCache, "Scheduling drain for line %#x. %d pkts\n",
                getLineNumber(block_addr), (it->second).size());
        schedule(new DrainBlockedPacketsEvent(this,
                                              getLineNumber(block_addr),
                                              it->second),
                 clockEdge(Cycles(1)));
         // This needs to be removed at this point, not later
         blockedPackets.erase(getLineNumber(block_addr));
    }
    DPRINTF(DRAMCache, "Removing %#x (%#x) from stalled writes\n",
            block_addr, getLineNumber(block_addr));
    // Go ahead and remove this from the main checking structure
    // This will allow us to check in handleRequest and after
    // TODO: I feel like this could cause a consistency issue if a
    //       request comes in while we're draining this... but oh well
    // Previously, we only did this if there were no blocked packets
    // and then erased this entry when the drain was done
    assert(blockedForReplacement.count(getLineNumber(block_addr)) == 1);
    blockedForReplacement.erase(getLineNumber(block_addr));
}

bool
DRAMCacheCtrl::sendBackprobe(Addr addr)
{
    DPRINTF(DRAMCache, "Sending a backprobe for %#x\n", addr);
    panic_if(outstandingInvalidates.find(addr)
                != outstandingInvalidates.end(), "Matches outstanding inv.\n");

    probes++;

    bool skip_wb = false;

    // It's possible that there is a matching WB in the stalled packet queue.
    // We need to check there first and forward it to memory.
    auto it = blockedPackets.find(getLineNumber(addr));
    if (it != blockedPackets.end()) {
        DPRINTF(DRAMCache, "trying to send a backprobe for a blocked pkt %d"
                           " are stalled\n", it->second.size());
        // NOTE: can't use for ( : ) because I need the iterator for erasing
        for (auto pkt_it = it->second.begin();
             pkt_it !=it->second.end();
             pkt_it++) {
            DPRINTF(DRAMCache, "Blocked pkt %s\n", (*pkt_it)->print());
            if ((*pkt_it)->getAddr() == addr) {
                DPRINTF(DRAMCache, "Found a matching packet in the blocked\n");
                // Found a packet with the same address.
                if ((*pkt_it)->isWriteback()) {
                    DPRINTF(DRAMCache, "Forwarding WB to mem\n");
                    // Send the writeback straight to memory. I hope this works
                    bool res M5_VAR_USED = sendMemSideReq((*pkt_it));
                    if (!res) DPRINTF(DRAMCache, "This failed :(\n");

                    // If we found a WB here, then there is no need to WB
                    // what's in the cache.
                    skip_wb = true;

                    // Remove this from the queue.
                    it->second.erase(pkt_it);
                    // If the queue is now empty, I think we hsould remove the
                    // line from the blocked packets.
                    if (it->second.empty()) {
                        DPRINTF(DRAMCache, "removing %#x from blocked\n",
                                            it->first);
                        blockedPackets.erase(it);
                    }
                    break;
                } else {
                    // We found a read request in the blocked queue.
                    // I think this means we don't need to send the probe
                    // Thus, let's just say "yes, do the writeback"
                    DPRINTF(DRAMCache, "Skipping probe %#x\n", addr);
                    return false;
                }
            }
        }

    }

    RequestPtr inv_req = new Request(addr, blockSize, 0, otherID);
    PacketPtr inv_pkt = new Packet(inv_req, MemCmd::ReadOwnerReq);
    inv_pkt->allocate();
    outstandingInvalidates.insert({addr, inv_pkt});
    invPort.schedTimingReq(inv_pkt, nextCycle());

    return skip_wb;
}

void
DRAMCacheCtrl::sendAtomicBackprobe(Addr addr)
{
    DPRINTF(DRAMCache, "Sending a backprobe for %#x\n", addr);
    panic_if(outstandingInvalidates.find(addr)
                != outstandingInvalidates.end(), "Matches outstanding inv.\n");

    probes++;

    RequestPtr inv_req = new Request(addr, blockSize, 0, otherID);
    PacketPtr inv_pkt = new Packet(inv_req, MemCmd::ReadReq);
    outstandingInvalidates.insert({addr, inv_pkt});
    invPort.sendAtomic(inv_pkt);
}

void
DRAMCacheCtrl::DrainBlockedPacketsEvent::process()
{
    DPRINTF(DRAMCache, "Processing blocked packets for %#x\n", lineNumber);
    DPRINTF(DRAMCache, "Total packets: %d\n", packetQueue.size());
    if (packetQueue.size() == 0) {
        warn("Extra drain event?");
        return;
    }
    PacketPtr pkt = packetQueue.front();
    DPRINTF(DRAMCache, "Processing blocked packet (%p)\n", pkt);
    if (pkt->isRead()) {
        ctrl->handleRead(pkt);
    } else {
        ctrl->handleWrite(pkt);
    }
    packetQueue.pop_front();
    DPRINTF(DRAMCache, "Total packets: %d\n", packetQueue.size());

    if (!packetQueue.empty()) {
        ctrl->schedule(this, ctrl->clockEdge(Cycles(1)));
    } else {
        delete this;
    }
}

DrainState
DRAMCacheCtrl::drain()
{
    // if there is anything in any of our internal queues, keep track
    // of that as well
    if (outstandingRequestQueueSize > 0 || writebacksWaitingForMem.size() > 0
            || memSideBlocked) {
        DPRINTF(Drain, "DRAMCache ctrl not drained, requests: %d, blkd: %d\n",
                outstandingRequestQueueSize, writebacksWaitingForMem.size());
        // I think everything will automatically be drained.
        return DrainState::Draining;
    } else {
        return DrainState::Drained;
    }
}

void
DRAMCacheCtrl::regStats()
{
    MemObject::regStats();

    memSideBlockedCount
        .name(name() + ".memSideBlockedCount")
        .desc("Number of times a request was blocked because of mem")
        ;
    storageBlockedCount
        .name(name() + ".storageBlockedCount")
        .desc("Number of times a request was blocked because of storage")
        ;
    replacementBlockedCount
        .name(name() + ".replacementBlockedCount")
        .desc("Number of times a request was blocked because of a repl.")
        ;
    outstandingQueueBlockedCount
        .name(name() + ".outstandingQueueBlockedCount")
        .desc("Number of times a request was blocked because outstanding queue"
              " was full")
        ;
    responseBlockedCount
        .name(name() + ".responseBlockedCount")
        .desc("Number of times a response from storage was blocked because of"
              " memory")
        ;

    readPackets
        .name(name() + ".readPackets")
        .desc("Number of read requests")
        ;
    cleanWBPackets
        .name(name() + ".cleanWBPackets")
        .desc("Number of clean write back requests")
        ;
    dirtyWBPackets
        .name(name() + ".dirtyWBPackets")
        .desc("Number of dirty writeback requests")
        ;

    cleaningReplaces
        .name(name() + ".cleaningReplaces")
        .desc("Number of requests to clean a line from DL")
        ;
    outstandingQueueBlockedCleaningCount
        .name(name() + ".outstandingQueueBlockedCleaningCount")
        .desc("Number times a cleaning replace blocked for outstanding")
        ;
    cleaningReplaceBlockedForReplacement
        .name(name() + ".cleaningReplaceBlockedForReplacement")
        .desc("Number times a cleaning replace blocked for current replace")
        ;
    cleaningReplaceDirty
        .name(name() + ".cleaningReplaceDirty")
        .desc("Actally cleaning a dirty line for a clean replace request")
        ;
    cleaningReplaceWasted
        .name(name() + ".cleaningReplaceWasted")
        .desc("The line was clean when tried to do a clean replace")
        ;

    missMapReadMiss
        .name(name() + ".missMapReadMiss")
        .desc("Times the miss map predicted a miss on a read")
        ;
    missMapReadHit
        .name(name() + ".missMapReadHit")
        .desc("Times the miss map predicted a hit on a read")
        ;
    missMapWriteMiss
        .name(name() + ".missMapWriteMiss")
        .desc("Times the miss map predicted a miss on a write")
        ;
    missMapWriteHit
        .name(name() + ".missMapWriteHit")
        .desc("Times the miss map predicted a hit on a write")
        ;

    dirtyListReadMiss
        .name(name() + ".dirtyListReadMiss")
        .desc("Times the dirty list found a miss on a read")
        ;
    dirtyListReadHit
        .name(name() + ".dirtyListReadHit")
        .desc("Times the dirty list found a hit on a read")
        ;
    dirtyListWriteMiss
        .name(name() + ".dirtyListWriteMiss")
        .desc("Times the dirty list found a miss on a write")
        ;
    dirtyListWriteHit
        .name(name() + ".dirtyListWriteHit")
        .desc("Times the dirty list found a hit on a write")
        ;

    realReadMiss
        .name(name() + ".realReadMiss")
        .desc("Times we check the cache and found there was a miss")
        ;
    realReadHit
        .name(name() + ".realReadHit")
        .desc("Times we check the cache and found there was a hit")
        ;
    realReplaceMiss
        .name(name() + ".realReplaceMiss")
        .desc("Times we check the cache on repl. and found there was a miss")
        ;
    realReplaceColdMiss
        .name(name() + ".realReplaceColdMiss")
        .desc("Times we check the cache on repl. and found a cold miss")
        ;
    realReplaceHit
        .name(name() + ".realReplaceHit")
        .desc("Times we check the cache on repl. and found there was a hit")
        ;
    realReplaceCleanHit
        .name(name() + ".realReplaceCleanHit")
        .desc("Times we check the cache on repl. hit to a clean line")
        ;

    writeThroughCount
        .name(name() + ".writeThroughCount")
        .desc("Times writes are sent directly through to memory")
        ;
    dirtyListFull
        .name(name() + ".dirtyListFull")
        .desc("Number of writethroughs because the dirty list was full")
        ;

    adaptiveWritebackWrite
        .name(name() + ".adaptiveWritebackWrite")
        .desc("Times writes are sent the cache storage because of a full WQ")
        ;
    adaptiveWritebackRead
        .name(name() + ".adaptiveWritebackRead")
        .desc("Times writes are sent the cache storage because of a"
              "paritally full WQ and RQ")
        ;
    adaptiveWritebackBlocked
        .name(name() + ".adaptiveWritebackBlocked")
        .desc("Times writes are sent the cache storage because of the"
              "memory port is blocked")
        ;
    adaptiveWritebackRecentBlocked
        .name(name() + ".adaptiveWritebackRecentBlocked")
        .desc("Times writes are sent the cache storage because of the"
              "memory port was recently blocked")
        ;

    storageRead
        .name(name() + ".storageRead")
        .desc("Reads for read requests sent to the storage")
        ;
    storageReadForRepl
        .name(name() + ".storageReadForRepl")
        .desc("Reads for potential replacements")
        ;
    storageCleanWrite
        .name(name() + ".storageCleanWrite")
        .desc("Writes from clean WBs")
        ;
    storageDirtyWrite
        .name(name() + ".storageDirtyWrite")
        .desc("Writes from dirty WBs")
        ;

    couldClean
        .name(name() + ".couldClean")
        .desc("Times we read a dirty line, and could make it clean")
        ;
    proactiveCleans
        .name(name() + ".proactiveCleans")
        .desc("Times we read a dirty line, and sent a dirty WB to memory")
        ;

    cleanSinks
        .name(name() + ".cleanSinks")
        .desc("Times we ignored a clean WB because it was a hit")
        ;

    probes
        .name(name() + ".probes")
        .desc("Number of probes sent")
        ;
    uselessProbes
        .name(name() + ".uselessProbes")
        .desc("Number of probes sent that were not found in the cache")
        ;
    numRacyProbes
        .name(name() + ".numRacyProbes")
        .desc("Number of requests that conflict with current probe")
        ;
    numRacyProbeWriteback
        .name(name() + ".numRacyProbeWriteback")
        .desc("Number of WB requests that conflict with current probe")
        ;
    refillOnUpgrade
        .name(name() + ".refillOnUpgrade")
        .desc("Times we had to refill the DRAM cache because of LLC upgrades")
        ;
}

void
DRAMCacheCtrl::updateCanonicalData(PacketPtr pkt)
{
    if (!checkData) {
        return;
    }

    // Update data for writebacks and when we get responses to probes.
    assert(pkt->isWrite() || pkt->isResponse());

    if (pkt->getSize() == blockSize) {
        assert((pkt->getAddr()-bankNumber*blockSize) % (banks*blockSize) == 0);
    }

    auto it = dataCheck.find(pkt->getAddr());
    if (it == dataCheck.end()) {
        dataCheck[pkt->getAddr()] = new uint8_t[64];
        if (pkt->getSize() < 64) {
            memset(dataCheck[pkt->getAddr()], 0, 64);
        }
    }

    DPRINTF(DRAMCache, "Writing data for address %#x\n", pkt->getAddr());
    DDUMP(DRAMCache, pkt->getConstPtr<uint8_t>(), pkt->getSize());

    pkt->writeData(dataCheck[pkt->getAddr()]);
}

void
DRAMCacheCtrl::checkCanonicalData(PacketPtr pkt)
{
    if (!checkData) {
        return;
    }

    assert(pkt->isRead());
    auto it = dataCheck.find(pkt->getAddr());
    if (it == dataCheck.end()) {
        DPRINTF(DRAMCache, "Address %#x uninitialized\n", pkt->getAddr());
        return;
    }

    const uint8_t* good_data = it->second;
    const uint8_t* data = pkt->getConstPtr<uint8_t>();
    for (int i = 0; i < pkt->getSize(); i++) {
        if (good_data[i] != data[i]) {
            DPRINTF(DRAMCache, "Check failed for %#x\n", pkt->getAddr());
            DPRINTF(DRAMCache, "Good data:\n");
            DDUMP(DRAMCache, good_data, pkt->getSize());
            DPRINTF(DRAMCache, "Wrong data:\n");
            DDUMP(DRAMCache, pkt->getConstPtr<uint8_t>(), pkt->getSize());
            panic("Data doesn't match!");
        }
    }
}

DRAMCacheCtrl*
DRAMCacheCtrlParams::create()
{
    return new DRAMCacheCtrl(this);
}
