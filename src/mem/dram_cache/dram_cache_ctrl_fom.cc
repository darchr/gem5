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

#include "mem/dram_cache/dram_cache_ctrl_fom.hh"

#include "debug/DRAMCache.hh"

DRAMCacheCtrlFOM::DRAMCacheCtrlFOM(Params* p) :
    DRAMCacheCtrl(p)
{
    panic_if(writebackPolicy == Enums::writethrough,
        "Fill on miss cache does not support a writethrough policy");
}

bool
DRAMCacheCtrlFOM::recvTimingResp(PacketPtr pkt)
{
    // Just forward this along. Really this shouldn't come through this
    // controller.
    DPRINTF(DRAMCache, "Forwarding response to %s\n", pkt->print());
    assert(outstandingRequestQueueSize >= 1);

    checkCanonicalData(pkt);

    // Actually do the fill!
    RequestPtr req = new Request(pkt->getAddr(), pkt->getSize(),
                                 0, 0);
    PacketPtr fill_pkt = new Packet(req, MemCmd::WritebackClean);
    fill_pkt->dataDynamic(new uint8_t[blockSize]);
    fill_pkt->setData(pkt->getConstPtr<uint8_t>());
    trySendStorageReq(fill_pkt);

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

    unblockReplace(blockAlign(pkt->getAddr()));

    return true;
}

bool
DRAMCacheCtrlFOM::recvTimingReq(PacketPtr pkt, bool from_cache)
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

    if (outstandingRequestQueueSize >= maxOutstanding) {
        DPRINTF(DRAMCache, "Outstanding request buffer full!\n");
        outstandingQueueBlockedCount++;
        assert(outstandingRequestQueueSize < maxOutstanding+2);
        if (from_cache) slavePort.setWaitingForRetry();
        return false;
    }

    if (pkt->isWrite()) {
        updateCanonicalData(pkt);
    }

    // Add the packet to the request queue
    outstandingRequestQueueSize++;
    if (pkt->isWriteback() || pkt->isRead()) {
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

    panic_if(pkt->cmd == MemCmd::ReplaceReq, "HOW???");

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

void
DRAMCacheCtrlFOM::handleRead(PacketPtr pkt)
{
    DPRINTF(DRAMCache, "Handling a read request %s\n", pkt->print());
    missMapReadHit++;
    storageRead++;

    panic_if(pkt->cmd == MemCmd::ReplaceReq, "SERIOUSLY, HOW???");

    // There's a chance that an in-flight write is now blocking this req.
    // Do NOT need to check when we are draining blocked packets.
    if (checkBlockForReplacement(pkt)) {
        // TODO: G Had to check this for the G state.
        storageCtrl->queueEmptyRead(pkt, getStorageAddr(pkt->getAddr()));
        return;
    }

    // This is a new miss, so remove it from the fake directory.
    auto it = fakeDirectory.find(pkt->getAddr());
    if (it != fakeDirectory.end()) {
        DPRINTF(DRAMCache, "removing from fake directory\n");
        fakeDirectory.erase(it);
    }

    sendReplace(pkt);
}

bool
DRAMCacheCtrlFOM::canRecvStorageResp(PacketPtr pkt, bool hit, bool dirty)
{
    // This is needed since we poll when there is a write queue hit
    if (storageNeedsUnblock) {
        return false;
    }

    if (memSideBlocked || !writebacksWaitingForMem.empty()) {
        assert(pkt->cmd == MemCmd::ReplaceReq);

        ReplaceSenderState *state =
                dynamic_cast<ReplaceSenderState *>(pkt->senderState);
        assert(state != nullptr);
        PacketPtr orig_pkt = state->pkt;

        DPRINTF(DRAMCache, "Checking if can respond for %s to orig %s\n",
                hit ? "hit" : "miss", orig_pkt->print());

        if (orig_pkt->isWriteback()) {
            if (!hit && dirty) {
                // If this is a replace, and a miss, and the data is dirty,
                // then we need to check to send a memory request.
                DPRINTF(DRAMCache, "Must stall for dirty miss\n");
                storageNeedsUnblock = true;
                responseBlockedCount++;
                return false;
            } else if (pkt->cmd == MemCmd::WritebackDirty &&
                        shouldWriteThrough(pkt->getAddr())) {
                // This is when we need to write through to memory.
                // Note: this and the above could both be true... then we may
                // be screwed. It may be OK to add one more buffer
                DPRINTF(DRAMCache, "Must stall for writethrough\n");
                storageNeedsUnblock = true;
                responseBlockedCount++;
                return false;
            }
        } else {
            if (!hit) {
                // if it is a read miss, need to forward to DRAM
                DPRINTF(DRAMCache, "Must stall for read miss\n");
                storageNeedsUnblock = true;
                responseBlockedCount++;
                return false;
            }
        }
    }

    // All other cases we don't need memory, so it's ok to recv. the response
    return true;
}

void
DRAMCacheCtrlFOM::recvStorageResponse(PacketPtr pkt, bool hit)
{
    DPRINTF(DRAMCache, "Response from storage (%s) for pkt (%p) %s\n",
            hit ? "hit" : "miss", pkt, pkt->print());
    // Only reads should respond
    assert(pkt->isRead());
    if (pkt->cmd == MemCmd::ReplaceReq) {
        assert(outstandingRequestQueueSize >= 1);
    }
    assert(pkt->cmd == MemCmd::ReplaceReq);

    ReplaceSenderState *state =
            dynamic_cast<ReplaceSenderState *>(pkt->popSenderState());
    assert(state != nullptr);
    PacketPtr orig_pkt = state->pkt;
    delete state;
    DPRINTF(DRAMCache, "Response to repl. for orig pkt %s\n",
            orig_pkt->print());

    if (orig_pkt->isRead()) {
        if (hit) {
            // just need to repond and free the buffers.
            realReadHit++;

            // Copy data from "replace" packet into original packet
            orig_pkt->setData(pkt->getConstPtr<uint8_t>());

            // response_time consumes the static latency and is charged also
            // with headerDelay that takes into account the delay provided by
            // the xbar and also the payloadDelay that takes into account the
            // number of data beats.
            Tick response_time = curTick() + orig_pkt->headerDelay +
                                 orig_pkt->payloadDelay;

            // Here we reset the timing of the packet before sending it out.
            orig_pkt->headerDelay = orig_pkt->payloadDelay = 0;

            checkCanonicalData(orig_pkt);

            // If this block is in the dirty list, then we can proactively
            // clean it so we don't have to read it at some later time.
            if (dirtyList && dirtyList->checkDirty(orig_pkt->getAddr())) {
                couldClean++;
                // Probably shouldn't try to end it if the memory is already
                // blocked, but we probably should if the dirty list is full
                // This is an interesting policy choice
                if (!memSideBlocked ||
                        dirtyList->checkFull(orig_pkt->getAddr())) {
                    proactiveCleans++;
                    // Create a deep copy of the packet to write through to mem
                    RequestPtr req = new Request(orig_pkt->getAddr(),
                                                 orig_pkt->getSize(), 0, 0);
                    PacketPtr wb_pkt = new Packet(req, MemCmd::WritebackDirty);
                    wb_pkt->dataDynamic(new uint8_t[blockSize]);
                    wb_pkt->setData(pkt->getConstPtr<uint8_t>());

                    // Mark this line as clean now.
                    dirtyList->removeDirty(orig_pkt->getAddr());

                    DPRINTF(DRAMCache, "Sending proactive clean WB\n");
                    sendMemSideReq(wb_pkt);
                }
            }

            assert(pkt->needsResponse());
            orig_pkt->makeResponse();

            // Set that this packet's response was generated by a cache so we
            // can potential filter out useless clean writebacks later
            orig_pkt->setResponseFromCache();

            // queue the packet in the response queue to be sent out after
            // the static latency has passed
            slavePort.schedTimingResp(orig_pkt, response_time, true);

            DPRINTF(DRAMCache, "(H)Decr (%d)\n", outstandingRequestQueueSize);
            decrementOutstandingQueue();
            unblockReplace(blockAlign(orig_pkt->getAddr()));

            // Delete the packet we created earlier
            delete pkt->req;
            delete pkt;
        } else {
            if (pkt->getAddr() == MaxAddr) {
                // This is a cold miss, so there's no need to "write it back"
                realReplaceColdMiss++;
                DPRINTF(DRAMCache, "Cold replace miss, no need to write\n");
                delete pkt->req;
                delete pkt;
            } else {
                realReadMiss++;

                // Here, we need to send a backprobe to the cache!
                bool skip_wb = false;
                if (sendBackprobes) {
                    skip_wb = sendBackprobe(pkt->getAddr());
                }

                // Need to writeback the data if it was dirty!
                if (pkt->hasDirtyDataFromCache()) {
                    realReplaceMiss++;
                    // Remove this from the dirty list; we currently cleaning
                    if (dirtyList) {
                        dirtyList->removeDirty(pkt->getAddr());
                    }
                    // Note: could remove this from the miss map now
                    //    this currently doesn't make sense because the missmap
                    //    should be immediately updated in handleWriteback

                    DPRINTF(DRAMCache, "Writing back to DRAM; was miss\n");
                    // makeResponse will turn this into a writeback request.
                    pkt->makeResponse();
                    // Note: this pkt's address is the address of the data that
                    //       it is holding. I.e., the address of the victim
                    if (!skip_wb) {
                        sendMemSideReq(pkt);
                    }
                    // Don't need to delete since we're still using this packet
                } else {
                    // It was cean, so no need to do a writeback, just delete.
                    DPRINTF(DRAMCache, "Tried to replace a clean line\n");
                    delete pkt->req;
                    delete pkt;
                }
            }

            // TODO: G
            // Mark the line as busy (G state)
            storageCtrl->queueEmptyWrite(orig_pkt,
                                         getStorageAddr(orig_pkt->getAddr()));

            // Forward the oringinal request.
            sendMemSideReq(orig_pkt);
        }
    } else {
        assert(orig_pkt->isWriteback());
        // The storage controller will set responderHadWritable only if the tag
        // was marked as dirty before. If it is clean, there's no need to write
        if (!hit) {
            if (pkt->getAddr() == MaxAddr) {
                // This is a cold miss, so there's no need to "write it back"
                realReplaceColdMiss++;
                DPRINTF(DRAMCache, "Cold replace miss, no need to write\n");
                delete pkt->req;
                delete pkt;
            } else {
                // Here, we need to send a backprobe to the cache!
                bool skip_wb = false;
                if (sendBackprobes) {
                    skip_wb = sendBackprobe(pkt->getAddr());
                }

                if (pkt->hasDirtyDataFromCache()) {
                    realReplaceMiss++;
                    // Remove this from the dirty list; we are cleaning
                    if (dirtyList) {
                        dirtyList->removeDirty(pkt->getAddr());
                    }
                    // Note: could remove this from the miss map now
                    //       this currently doesn't make sense because it
                    //       should be immediately updated in handleWriteback

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
                    realReplaceCleanHit++;
                    DPRINTF(DRAMCache, "Tried to replace a clean line here\n");
                    delete pkt->req;
                    delete pkt;
                }
            }
        } else {
            // It was actually already there, so no need to do anything.
            realReplaceHit++;
            delete pkt->req;
            delete pkt;
        }
        // Now, we can send the write on with no problem
        bool success M5_VAR_USED = handleWriteback(orig_pkt);
        assert(success);

        DPRINTF(DRAMCache, "(R)Decr (%d)\n", outstandingRequestQueueSize);

        unblockReplace(blockAlign(orig_pkt->getAddr()));
    }
}

DRAMCacheCtrlFOM*
DRAMCacheCtrlFOMParams::create()
{
    return new DRAMCacheCtrlFOM(this);
}
