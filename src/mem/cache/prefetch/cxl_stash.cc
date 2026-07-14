/*
 * Copyright (c) 2026 The Regents of the University of California
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

#include "mem/cache/prefetch/cxl_stash.hh"

#include <algorithm>

#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/HWPrefetch.hh"
#include "mem/request.hh"
#include "params/CxlStashPrefetcher.hh"

namespace gem5
{

namespace prefetch
{

namespace
{
// All constructed stashes, for PA -> stash lookup by the device. gem5 runs
// one configuration per process, so a file-static registry is sufficient.
std::vector<CxlStash*>&
stashRegistry()
{
    static std::vector<CxlStash*> reg;
    return reg;
}
} // anonymous namespace

CxlStash*
lookupCxlStash(Addr pa)
{
    for (CxlStash* s : stashRegistry()) {
        if (s->servesPa(pa)) {
            return s;
        }
    }
    return nullptr;
}

CxlStash::CxlStash(const CxlStashPrefetcherParams &p)
    : Base(p),
      maxPending(p.max_pending_lines),
      poolRange(p.pool_range),
      stashStats(this)
{
    stashRegistry().push_back(this);
}

CxlStash::~CxlStash()
{
    auto& reg = stashRegistry();
    reg.erase(std::remove(reg.begin(), reg.end(), this), reg.end());
}

void
CxlStash::externalTrigger(Addr base, unsigned bytes)
{
    // Block-align the run and enqueue one line per cache block. blkSize is
    // the Ruby block size (set via setParentInfo from the controller).
    Addr start = base - (base % blkSize);
    Addr end = base + bytes;
    for (Addr a = start; a < end; a += blkSize) {
        if (pending.size() >= maxPending) {
            stashStats.linesDropped++;
            continue;
        }
        pending.push_back(a);
        stashStats.linesTriggered++;
    }
    // Wake the owning RubyPrefetcherProxy so it starts polling getPacket().
    issueCheck();
}

PacketPtr
CxlStash::getPacket()
{
    if (pending.empty()) {
        return nullptr;
    }
    Addr line = pending.front();
    pending.pop_front();

    // A plain read prefetch (HardPFReq, not writable) -> the proxy issues it
    // as a RubyRequestType:LD -> is_HN Initiate_LoadMiss -> ReadNoSnp fill.
    RequestPtr req = std::make_shared<Request>(line, blkSize, 0, requestorId);
    req->taskId(context_switch_task_id::Prefetcher);
    PacketPtr pkt = new Packet(req, MemCmd::HardPFReq);
    pkt->allocate();

    stashStats.linesIssued++;
    DPRINTF(HWPrefetch, "CxlStash issuing stash prefetch for line %#x\n",
            line);
    return pkt;
}

Tick
CxlStash::nextPrefetchReadyTime() const
{
    // Ready immediately when we have work; the device already accounted the
    // CXL push latency by scheduling externalTrigger. Empty => never.
    return pending.empty() ? MaxTick : curTick();
}

CxlStash::CxlStashStats::CxlStashStats(statistics::Group *parent)
    : statistics::Group(parent),
      ADD_STAT(linesTriggered, statistics::units::Count::get(),
               "Cache lines pushed in via externalTrigger"),
      ADD_STAT(linesIssued, statistics::units::Count::get(),
               "Stash prefetch packets handed to the proxy"),
      ADD_STAT(linesDropped, statistics::units::Count::get(),
               "Lines dropped because the pending queue was full")
{
}

} // namespace prefetch
} // namespace gem5
