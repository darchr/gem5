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

/**
 * @file
 * A dormant, externally-triggered prefetcher for the CXL host-MPSC stash
 * model (docs/cacheable_mpsc_plan.md phase 5). Attached to each is_HN SLC.
 *
 * It observes NOTHING from the demand stream -- notify() is a no-op. The
 * CxlHardwareBuffer device pushes delivered-message cache lines into it via
 * externalTrigger(base, bytes); each becomes a HardPFReq the SLC issues (a
 * ReadNoSnp fill via the is_HN Initiate_LoadMiss path), warming the SLC so
 * the receiver's next poll hits in cache. This models the device actively
 * installing the message into the host's cache. The device owns the CXL push
 * latency (it schedules the trigger); the fill from memory is real Ruby time.
 */
#ifndef __MEM_CACHE_PREFETCH_CXL_STASH_HH__
#define __MEM_CACHE_PREFETCH_CXL_STASH_HH__

#include <deque>

#include "base/addr_range.hh"
#include "base/types.hh"
#include "mem/cache/prefetch/base.hh"
#include "mem/packet.hh"

namespace gem5
{

struct CxlStashPrefetcherParams;

namespace prefetch
{

class CxlStash;

/**
 * Runtime registry: find the stash prefetcher whose reserved pool contains
 * `pa`, or nullptr. Lets the CxlHardwareBuffer device reach the SLC-owned
 * stash by ring PA without a SimObject param (which would form a
 * config-hierarchy cycle -- the stash is referenced by its SLC already).
 */
CxlStash* lookupCxlStash(Addr pa);

class CxlStash : public Base
{
  public:
    CxlStash(const CxlStashPrefetcherParams &p);
    ~CxlStash();

    /** True if this stash serves the given host PA. */
    bool servesPa(Addr pa) const { return poolRange.contains(pa); }

    /**
     * Push a run of cache lines into the issue queue and wake the owning
     * proxy. Called from the CxlHardwareBuffer device (C++-to-C++, no
     * simulated time -- the device already modeled the push latency by
     * scheduling this call). [base, base+bytes) is split into block-aligned
     * lines.
     */
    void externalTrigger(Addr base, unsigned bytes);

    // Dormant: never react to the demand stream.
    void notify(const CacheAccessProbeArg &acc,
                const PrefetchInfo &pfi) override {}

    PacketPtr getPacket() override;
    Tick nextPrefetchReadyTime() const override;

  private:
    /** Line addresses waiting to be issued as prefetches. */
    std::deque<Addr> pending;
    /** Cap on `pending`; excess pushes are dropped (performance-only). */
    const unsigned maxPending;
    /** The reserved MPSC pool this stash serves (for registry lookup). */
    const AddrRange poolRange;

    struct CxlStashStats : public statistics::Group
    {
        CxlStashStats(statistics::Group *parent);
        statistics::Scalar linesTriggered;
        statistics::Scalar linesIssued;
        statistics::Scalar linesDropped;
    } stashStats;
};

} // namespace prefetch
} // namespace gem5

#endif // __MEM_CACHE_PREFETCH_CXL_STASH_HH__
