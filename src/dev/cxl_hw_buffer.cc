#include "dev/cxl_hw_buffer.hh"

#include <cstring>

#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/CXLBUF.hh"
#include "debug/CXLBUFEARLY.hh"
#include "mem/packet_access.hh"
#include "sim/serialize.hh"
#include "sim/system.hh"

namespace gem5
{

CxlHardwareBuffer::CxlHardwareBufferStats::CxlHardwareBufferStats(
    statistics::Group *parent, unsigned num_ranks, unsigned max_slots)
    : statistics::Group(parent),
      ADD_STAT(mpscOccupancy, statistics::units::Count::get(),
               "Per-receiver-rank MPSC slot occupancy sampled when a new "
               "slot is allocated (index = receiver MPI rank)"),
      ADD_STAT(mpscResidencyCycles, statistics::units::Cycle::get(),
               "Per-receiver-rank residency (cycles) a message spends in the "
               "MPSC queue, enqueue->dequeue (index = receiver MPI rank)")
{
    // One occupancy distribution per receiver MPI rank. Index i is the MPSC
    // queue owned by rank i; occupancy ranges over [0, max_slots) slots.
    mpscOccupancy.init(num_ranks, 0, max_slots, 1);
    // Residency histogram per rank. The fixed range only affects bucket
    // resolution -- ::mean is exact regardless -- so this is a tunable
    // default; widen/narrow after a first run shows the real magnitude.
    mpscResidencyCycles.init(num_ranks, 0, 100000, 1000);
}

CxlHardwareBuffer::CxlHardwareBuffer(const Params &p)
    : memory::SimpleMemory(p),
      transferLatency(p.transfer_latency),
      numEndpoints(p.num_endpoints),
      segmentSize(p.segment_size),
      slotSize(p.slot_size),
      mpscSize(p.mpsc_size),
      allRanksInitialized(false),
      backingSize(p.backing_size),
      backingChunkSize(p.backing_chunk_size),
      backingLatency(p.backing_latency),
      stats(this, p.num_endpoints, p.mpsc_size / p.slot_size)
{
    // Initialize Hardware Backing Store
    overflowBackingStore.resize(backingSize);
    uint32_t num_chunks = backingSize / backingChunkSize;
    for (uint32_t i = 0; i < num_chunks; ++i) {
        freeChunks.push(i);
    }
    overflowStates.resize(numEndpoints);

    // Per-receiver residency tracking (transient; rebuilt on restore).
    mpscEnqTick.resize(numEndpoints);
    mpscLastHead.resize(numEndpoints, 0);

    // Initialize MPSC tails
    mpsc_tails.resize(numEndpoints, slotSize);
    // start at slotSize just like hw_emu.c

    // Populate rank offsets if provided from Python (for checkpoint restore)
    for (size_t i = 0; i < p.rank_offsets.size(); ++i) {
        rankBaseOffsets[i] = p.rank_offsets[i];
    }

    // If all offsets were provided at construction, mark as initialized
    if (rankBaseOffsets.size() >= numEndpoints) {
        allRanksInitialized = true;
        printf("CXL_HW_BUFFER: All %d ranks pre-initialized from "
               "checkpoint restore.\n", numEndpoints);
    }
}

uint32_t
CxlHardwareBuffer::getRankFromOffset(Addr offset)
{
    // This uses the explicit handshake method.
    // If the offset falls within a known rank's segment, return that rank.
    for (const auto& kv : rankBaseOffsets) {
        if (offset >= kv.second && offset < kv.second + segmentSize) {
            return kv.first;
        }
    }
    // Fallback if not explicitly initialized yet:
    // assume linear allocation (Rank = Offset / SegmentSize)
    return offset / segmentSize;
}

void
CxlHardwareBuffer::access(PacketPtr pkt)
{
    // ===== EXPERIMENT START: CXLBUFEARLY tracing =====
    DPRINTF(CXLBUFEARLY, "access(): %s paddr=0x%lx offset=0x%lx size=%u "
            "isWrite=%d hasData=%d isRead=%d\n",
            pkt->cmdString(), pkt->getAddr(),
            (Addr)(pkt->getAddr() - range.start()), pkt->getSize(),
            pkt->isWrite(), pkt->hasData(), pkt->isRead());
    // ===== EXPERIMENT END: CXLBUFEARLY tracing =====

    // For non-write packets or packets without data, just do normal
    // memory access (reads, clean evictions, invalidations, etc.)
    // Currently this relies on the software to correctly read from
    // the circular buffer. Which is an actual memory backed
    // circular buffer
    if (!pkt->isWrite() || !pkt->hasData()) {
        if (allRanksInitialized && pkt->isRead()) {
            static uint64_t num_reads = 0;
            num_reads++;
            if (num_reads % 100000 == 0) {
                DPRINTF(CXLBUF, "CXL Hardware Buffer: Processed %llu reads "
                        "so far (polling active)\n",
                        (unsigned long long)num_reads);
            }

            Addr offset = pkt->getAddr() - range.start();
            uint32_t rank = getRankFromOffset(offset);
            if (rank < numEndpoints) {
                Addr base = rankBaseOffsets[rank];
                [[maybe_unused]] Addr segment_offset = offset - base;
            }
        }
        memory::AbstractMemory::access(pkt);
        return;
    }

    Addr offset = pkt->getAddr() - range.start();
    unsigned size = pkt->getSize();

    // Bounds check
    if (offset + size > range.size()) {
        warn("CXL Hardware Buffer: write out of bounds at offset 0x%lx, "
             "size %d. Ignoring.\n",
             offset, size);
        pkt->makeResponse();
        return;
    }

    // ================================================================
    // HANDSHAKE DETECTION & ROUTING MODE
    // ================================================================

    // Attempt discovery once if we just booted from a checkpoint
    if (!allRanksInitialized) {
        discoverRanksFromMemory();
        if (rankBaseOffsets.size() >= numEndpoints) {
            allRanksInitialized = true;
        }
    }

    // Check for handshake writes (initialization or re-initialization)
    bool is_handshake = false;
    Addr base_offset = 0;
    uint32_t rank = 0;

    if (pkt->hasData()) {
        uint8_t *data = pkt->getPtr<uint8_t>();
        Addr page_base = offset - (offset % 4096);

        // The BTL writes its handshake magic at (segment_base +
        // HANDSHAKE_MAGIC_OFFSET). The segment is NOT guaranteed to be
        // page-aligned -- in practice my_segment lands at page+8 (the
        // opal_shmem mapping), so the magic sits at page+16. We therefore
        // probe a few candidate offsets within the page, but anchor the
        // segment base at the magic location MINUS HANDSHAKE_MAGIC_OFFSET.
        // Rounding down to the page (the old behavior) records the base 8
        // bytes too low, which shifts every slot field +8 in our coordinates
        // (e.g. flag@17 is seen at 25) and corrupts all routing.
        for (Addr pgoff = 0; pgoff <= 24; pgoff += 8) {
            Addr target_addr = page_base + pgoff;
            if (offset <= target_addr &&
                (offset + size) >= (target_addr + 4)) {
                Addr data_idx = target_addr - offset;
                uint32_t val;
                std::memcpy(&val, &data[data_idx], 4);
                if ((val & 0xFFFF0000) == 0xC0010000) {
                    rank = val & 0xFFFF;
                    if (rank < numEndpoints) {
                        is_handshake = true;
                        base_offset = target_addr - HANDSHAKE_MAGIC_OFFSET;
                        break;
                    }
                }
            }
        }
    }

    if (is_handshake) {
        rankBaseOffsets[rank] = base_offset;
        inform("CXL_HW_BUFFER_INIT: Rank %d initialized at "
               "PA 0x%lx (base offset: 0x%lx)",
               rank, range.start() + base_offset, base_offset);
        DPRINTF(CXLBUF, "CXL Hardware Buffer: Handshake "
                "received. Rank %d mapped to offset 0x%lx\n",
                rank, base_offset);

        if (rankBaseOffsets.size() >= numEndpoints) {
            allRanksInitialized = true;
            inform("CXL_HW_BUFFER: All %d ranks initialized.", numEndpoints);
        }

        // Scan for messages that might have arrived BEFORE this
        // receiver initialized!
        scanForPendingMessages(rank, base_offset);

        // Always do normal write for handshake
        memory::AbstractMemory::access(pkt);
        return;
    }

    // Try to handle via routing logic; if not applicable, do normal write
    if (!handleRoutedWrite(pkt, offset, size)) {
        DPRINTF(CXLBUFEARLY, "access(): handleRoutedWrite returned false for "
                "offset=0x%lx size=%u -> plain AbstractMemory write "
                "(no routing)\n", offset, size);
        memory::AbstractMemory::access(pkt);
    }
}

void
CxlHardwareBuffer::functionalAccess(PacketPtr pkt)
{
    // gem5 functional path: instant read/write that BYPASSES access() and the
    // routing logic. If devdax traffic comes through here on a restore, it
    // would update the backing store silently with no CXLBUF trace -- this is
    // exactly the blind spot we're checking for.
    DPRINTF(CXLBUFEARLY, "functionalAccess(): %s paddr=0x%lx offset=0x%lx "
            "size=%u isWrite=%d hasData=%d\n",
            pkt->cmdString(), pkt->getAddr(),
            (Addr)(pkt->getAddr() - range.start()), pkt->getSize(),
            pkt->isWrite(), pkt->hasData());
    memory::AbstractMemory::functionalAccess(pkt);
}

bool
CxlHardwareBuffer::handleRoutedWrite(PacketPtr pkt, Addr offset,
                                     unsigned size)
{
    // The segment owner is the RECEIVER of the message.
    // In CXL shared memory, the sender writes into the receiver's segment.
    uint32_t receiver = getRankFromOffset(offset);

    Addr base = 0;
    if (rankBaseOffsets.find(receiver) != rankBaseOffsets.end()) {
        base = rankBaseOffsets[receiver];
    } else {
        // This means getRankFromOffset fell back to offset / segmentSize,
        // but the rank isn't actually registered. This is likely an OS
        // filesystem metadata write. Act as passthrough.
        return false;
    }
    uint32_t segment_offset = offset - base;

    // Is this a write into an SPSC queue?
    // SPSC queues start after the MPSC (which is mpscSize).
    if (segment_offset < mpscSize) {
        // This is a write to the MPSC queue by the receiver
        // (e.g. clearing a flag).
        // We intercept flag-clear writes to drain overflow messages.
        uint8_t *data = pkt->getPtr<uint8_t>();
        uint32_t slot_internal_offset = segment_offset % slotSize;

        bool is_flag_clear = false;
        if (slot_internal_offset <= FLAG_OFFSET &&
            (slot_internal_offset + size) > FLAG_OFFSET) {
            uint8_t flag_val = data[FLAG_OFFSET - slot_internal_offset];
            if (flag_val == 0) {
                is_flag_clear = true;
            }
        }

        // Apply the write to memory first so the flag becomes 0
        memory::AbstractMemory::access(pkt);

        if (is_flag_clear) {
            auto &state = overflowStates[receiver];
            bool has_complete_message = false;

            if (state.chunks.size() > 1) {
                has_complete_message = true;
            } else if (state.chunks.size() == 1 &&
                       state.headOffset < state.tailOffset) {
                has_complete_message = true;
            }

            if (has_complete_message) {
                // We have a backlog! We need to place the overflow
                // message into an MPSC slot the receiver will actually
                // reach. Read the receiver's fifo_head from the FIFO
                // control block (stored at offset 0 of the segment as
                // a 64-bit value) and scan forward to find the first
                // free slot.
                int64_t receiver_head = 0;
                std::memcpy(&receiver_head, &pmemAddr[base], sizeof(int64_t));

                // Sanitize: fifo_head should be a multiple of slotSize
                // within [slotSize, mpscSize)
                if (receiver_head < (int64_t)slotSize ||
                    receiver_head >= (int64_t)mpscSize) {
                    receiver_head = slotSize;
                }

                // Scan forward from the receiver's head to find the
                // first free slot it will reach
                uint32_t drain_mpsc_offset = (uint32_t)receiver_head;
                bool found_free = false;
                uint32_t max_slots = mpscSize / slotSize;
                for (uint32_t attempts = 0; attempts < max_slots;
                     ++attempts) {
                    Addr check_flag =
                        base + drain_mpsc_offset + FLAG_OFFSET;
                    if (pmemAddr[check_flag] == 0) {
                        found_free = true;
                        break;
                    }
                    drain_mpsc_offset += slotSize;
                    if (drain_mpsc_offset >= mpscSize) {
                        drain_mpsc_offset = slotSize;
                    }
                }

                if (found_free) {
                    uint32_t source_chunk = state.chunks.front();
                    uint32_t read_offset = state.headOffset;

                    uint64_t backing_addr =
                        (uint64_t)source_chunk * backingChunkSize +
                        read_offset;

                    // Clear the flag in the source buffer temporarily so
                    // we don't copy it
                    uint8_t complete_flag =
                        overflowBackingStore[backing_addr + FLAG_OFFSET];
                    overflowBackingStore[backing_addr + FLAG_OFFSET] = 0;

                    // Copy the entire slot (with the flag cleared)
                    std::memcpy(&pmemAddr[base + drain_mpsc_offset],
                                &overflowBackingStore[backing_addr], slotSize);

                    // Write the flag last to ensure memory ordering
                    pmemAddr[base + drain_mpsc_offset + FLAG_OFFSET] =
                        complete_flag;

                    DPRINTF(CXLBUF, "CXL Hardware Buffer: Draining overflow "
                            "message to Receiver %d MPSC offset 0x%x "
                            "(receiver_head=0x%x)\n",
                            receiver, drain_mpsc_offset,
                            (uint32_t)receiver_head);
                    DPRINTF(CXLBUF, "[CXLBUF-HW-DEBUG] OVERFLOW DRAIN -> "
                            "RECEIVER %d | MPSC offset=0x%x "
                            "(receiver_head=0x%x)\n",
                            receiver, drain_mpsc_offset,
                            (uint32_t)receiver_head);

                    // Only advance mpsc_tails if drain is ahead of it
                    // (circular comparison)
                    uint32_t tail = mpsc_tails[receiver];
                    uint32_t head = receiver_head;

                    uint32_t next_after_drain = drain_mpsc_offset + slotSize;
                    if (next_after_drain >= mpscSize) {
                        next_after_drain = slotSize;
                    }

                    // Distance from head to drain is the 'age' of the
                    // drain slot
                    uint32_t drain_dist =
                        (drain_mpsc_offset + mpscSize - head) % mpscSize;
                    uint32_t tail_dist = (tail + mpscSize - head) % mpscSize;

                    if (drain_dist >= tail_dist) {
                        mpsc_tails[receiver] = next_after_drain;
                    }

                    // Advance head pointer in the overflow queue
                    state.headOffset += slotSize;

                    // Check for chunk exhaustion
                    if (state.chunks.size() > 1 &&
                        state.headOffset == backingChunkSize) {
                        freeChunks.push(source_chunk);
                        state.chunks.pop();
                        state.headOffset = 0;
                    } else if (state.chunks.size() == 1 &&
                               state.headOffset == state.tailOffset) {
                        freeChunks.push(source_chunk);
                        state.chunks.pop();
                        state.headOffset = 0;
                        state.tailOffset = 0;
                        DPRINTF(CXLBUF, "CXL_HW_BUFFER: Receiver %d "
                                "overflow queue fully drained.\n", receiver);
                    }
                }
            }
        }

        return true; // We handled the MPSC write completely
    }

    uint8_t *data = pkt->getPtr<uint8_t>();

    // Calculate sender and slot.
    // Within the receiver's segment, the SPSC region is laid out as:
    //   MPSC(1MB) | SPSC_rank0(1MB) | SPSC_rank1(1MB) | ...
    // So the index into the SPSC region tells us which sender wrote it.
    uint32_t spsc_region_offset = segment_offset - mpscSize;
    uint32_t sender = spsc_region_offset / mpscSize;

    // Bounds check: sender must be a valid rank
    if (sender >= numEndpoints) {
        return false; // Write falls outside valid SPSC queue area
    }
    // assuming SPSC sizes are equal to mpscSize (1MB) as in hw_emu.c
    uint32_t slot_idx = (spsc_region_offset % mpscSize) / slotSize;
    uint32_t slot_internal_offset = (spsc_region_offset % mpscSize) %
                                     slotSize;

    // SPSC queues do NOT have a control block. They are purely raw buffers
    // starting at slot_idx=0. Do not drop slot 0!

    if (slot_idx <= 2) {
        DPRINTF(CXLBUF, "SPSC WRITE: sender=%d, receiver=%d, slot_idx=%d, "
                "offset=%d, size=%d, data[0]=%02x\n",
                sender, receiver, slot_idx, slot_internal_offset,
                size, size > 0 ? data[0] : 0);
    }

    uint32_t key = (sender << 16) | receiver;

    // Apply the write directly to backing memory (SPSC queue).
    // AbstractMemory::access will NOT be called if we return true,
    // so we must do it here.
    std::memcpy(&pmemAddr[offset], data, size);

    // Check if this write encompasses the FLAG_OFFSET
    if (slot_internal_offset <= FLAG_OFFSET &&
        (slot_internal_offset + size) > FLAG_OFFSET) {

        uint8_t flag_val = data[FLAG_OFFSET - slot_internal_offset];

        if (slot_idx <= 2) {
            DPRINTF(CXLBUF, "FLAG WRITE TRACE: sender=%d, receiver=%d, "
                    "slot=%d, offset=%d, size=%d, extracted_flag=%02x\n",
                    sender, receiver, slot_idx, slot_internal_offset,
                    size, flag_val);
        }

        if (flag_val == FLAG_COMPLETE) {
            // A message is ready! Allocate an MPSC slot and transfer.

            // Bounds check for receiver
            if (rankBaseOffsets.find(receiver) == rankBaseOffsets.end()) {
                warn("CXL Hardware Buffer: No base offset for receiver %d. "
                     "Flag write preserved in SPSC backing store.\n",
                     receiver);
                // Return true so we don't fall back to AbstractMemory::access
                // which would overwrite our memcpy.
                if (pkt->needsResponse()) {
                    pkt->makeResponse();
                }
                return true;
            }

            // Allocate a new MPSC slot on the RECEIVER's MPSC queue!
            // But first, check if the target slot is still unread (overflow!)
            uint32_t new_mpsc_offset = mpsc_tails[receiver];
            Addr dest_mpsc_flag_addr =
                rankBaseOffsets[receiver] + new_mpsc_offset + FLAG_OFFSET;
            bool is_slot_full =
                (pmemAddr[dest_mpsc_flag_addr] == FLAG_COMPLETE);
            auto &state = overflowStates[receiver];

            // Start of block for stat
            int64_t receiver_head = 0;
            Addr base_offset_recv = rankBaseOffsets[receiver];
            std::memcpy(&receiver_head, &pmemAddr[base_offset_recv],
                        sizeof(int64_t));

            // Sanitize receiver_head in case it's uninitialized
            if (receiver_head < (int64_t)slotSize ||
                receiver_head >= (int64_t)mpscSize) {
                receiver_head = slotSize;
            }

            uint32_t tail = new_mpsc_offset;
            uint32_t head = (uint32_t)receiver_head;

            // The receiver's head reflects what it has consumed: sample the
            // residency of any messages freed since we last observed it.
            sampleMpscResidency(receiver, head);
            uint32_t slots_in_use = 0;

            if (is_slot_full) {
                // It's completely full (minus control block)
                slots_in_use = (mpscSize / slotSize) - 1;
            } else {
                // Distance from head to tail
                uint32_t dist = (tail + mpscSize - head) % mpscSize;
                slots_in_use = dist / slotSize;
            }
            stats.mpscOccupancy[receiver].sample(slots_in_use);
            // End of block for stat

            if (is_slot_full || !state.chunks.empty()) {
                // OVERFLOW: The MPSC slot is full or we have a backlog.
                activeMappings[key][slot_idx] = 0xFFFFFFFF; // sentinel

                DPRINTF(CXLBUF, "CXL Hardware Buffer: OVERFLOW! MPSC slot "
                        "full for Receiver %d. Sender %d SPSC slot %d -> "
                        "backing store\n", receiver, sender, slot_idx);
                DPRINTF(CXLBUF, "[CXLBUF-HW-DEBUG] OVERFLOW: SENDER %d -> "
                        "RECEIVER %d | spsc_slot_idx=%d "
                        "goes to backing store\n",
                        sender, receiver, slot_idx);
            } else {
                activeMappings[key][slot_idx] = new_mpsc_offset;

                DPRINTF(CXLBUF, "CXL Hardware Buffer: Allocated MPSC slot at "
                        "offset 0x%x for Sender %d -> Receiver %d "
                        "(SPSC slot %d)\n",
                        new_mpsc_offset, sender, receiver, slot_idx);

                // Advance the MPSC tail for future allocations
                uint32_t next_tail = new_mpsc_offset + slotSize;
                if (next_tail >= mpscSize) {
                    next_tail = slotSize; // wrap around, skipping slot 0
                }
                mpsc_tails[receiver] = next_tail;
            }
        }

        uint32_t dest_mpsc_offset = activeMappings[key][slot_idx];
        bool is_overflow = (dest_mpsc_offset == 0xFFFFFFFF);

        // Validate destination is in bounds (skip for overflow)
        if (!is_overflow &&
            rankBaseOffsets.find(receiver) == rankBaseOffsets.end()) {
            warn("CXL Hardware Buffer: No base offset for receiver %d. "
                 "Writing to backing memory directly.\n", receiver);
            return false;
        }

        // Write to the SPSC queue (the sender's local copy in the
        // receiver's segment). The full message is assembled here across
        // many sub-slot writes.
        std::memcpy(&pmemAddr[offset], data, size);

        if (!is_overflow) {
            // Normal path: deliver to the receiver's MPSC queue.
            //
            // A single message is written by the sender as MANY sub-slot
            // writes (header fields + payload), terminated by the COMPLETE
            // flag store. The SPSC->MPSC slot mapping only becomes valid once
            // that final flag write arrives, so we CANNOT mirror writes
            // incrementally -- the earlier payload writes have no destination
            // slot yet (and don't even span FLAG_OFFSET). Instead, when the
            // COMPLETE flag arrives we copy the ENTIRE slot from the SPSC
            // queue into the allocated MPSC slot -- exactly like the overflow
            // path copies into the backing store.
            bool is_complete_flag_write = false;
            if (slot_internal_offset <= FLAG_OFFSET &&
                (slot_internal_offset + size) > FLAG_OFFSET) {
                is_complete_flag_write =
                    (data[FLAG_OFFSET - slot_internal_offset]
                     == FLAG_COMPLETE);
            }

            if (is_complete_flag_write) {
                uint32_t spsc_slot_base = offset - slot_internal_offset;
                Addr mpsc_slot_base =
                    rankBaseOffsets[receiver] + dest_mpsc_offset;

                // Bounds check the full-slot copy destination
                if (mpsc_slot_base + slotSize > range.size()) {
                    warn("CXL Hardware Buffer: MPSC slot copy out of bounds "
                         "(0x%lx + %lu > 0x%lx). Dropping.\n",
                         mpsc_slot_base, (unsigned long)slotSize,
                         range.size());
                    return false;
                }

                // Copy header + payload into the MPSC slot. Keep the MPSC
                // completion flag CLEAR; completeTransfer() sets it after
                // transferLatency so the receiver never observes the flag
                // before the payload is present.
                std::memcpy(&pmemAddr[mpsc_slot_base],
                            &pmemAddr[spsc_slot_base], slotSize);
                pmemAddr[mpsc_slot_base + FLAG_OFFSET] = 0;

                DPRINTF(CXLBUF, "CXL Hardware Buffer: COMPLETE flag detected! "
                        "Scheduling normal transfer for Receiver %d at "
                        "MPSC offset 0x%x\n",
                        receiver, dest_mpsc_offset);
                DPRINTF(CXLBUF, "[CXLBUF-HW-DEBUG] SENDER %d -> RECEIVER %d | "
                        "spsc_slot_idx=%d | dest_mpsc_offset=0x%x\n",
                        sender, receiver, slot_idx, dest_mpsc_offset);

                // Remove mapping since it's complete
                activeMappings[key].erase(slot_idx);

                // Schedule the timed completion (sets MPSC flag, clears SPSC)
                pendingTransfers.push_back(
                    {receiver, dest_mpsc_offset, (uint32_t)offset});
                Event* e = new TransferEvent(
                    this, receiver, dest_mpsc_offset, (uint32_t)offset);
                schedule(e, curTick() + transferLatency);
            }
        } else {
            // OVERFLOW path: write data to SPSC only (already done above).
            // When FLAG_COMPLETE arrives, copy full slot to backing store.
            bool is_complete_flag_write = false;
            if (slot_internal_offset <= FLAG_OFFSET &&
                (slot_internal_offset + size) > FLAG_OFFSET) {
                uint8_t flag_val = data[FLAG_OFFSET - slot_internal_offset];
                if (flag_val == FLAG_COMPLETE) {
                    is_complete_flag_write = true;
                }
            }

            if (is_complete_flag_write) {
                DPRINTF(CXLBUF, "CXL Hardware Buffer: COMPLETE flag on "
                        "OVERFLOW slot! Copying Sender %d -> Receiver %d to "
                        "backing store\n", sender, receiver);

                // Remove mapping since it's complete
                activeMappings[key].erase(slot_idx);

                auto &state = overflowStates[receiver];

                // Allocate a new chunk if needed
                if (state.chunks.empty() ||
                    state.tailOffset == backingChunkSize) {
                    if (freeChunks.empty()) {
                        fatal("CXL Hardware Buffer: Out of Backing Memory!");
                    }
                    state.chunks.push(freeChunks.front());
                    freeChunks.pop();
                    state.tailOffset = 0;
                    if (state.chunks.size() == 1) {
                        state.headOffset = 0;
                    }
                }

                uint32_t target_chunk = state.chunks.back();
                uint64_t backing_addr =
                    (uint64_t)target_chunk * backingChunkSize +
                    state.tailOffset;

                // Copy the FULL slot payload from the SPSC queue
                uint32_t spsc_slot_base = offset - slot_internal_offset;
                std::memcpy(&overflowBackingStore[backing_addr],
                            &pmemAddr[spsc_slot_base], slotSize);

                // Ensure the flag in the backing store is set
                overflowBackingStore[backing_addr + FLAG_OFFSET] =
                    FLAG_COMPLETE;

                state.tailOffset += slotSize;

                // Clear the SPSC flag so the sender can reuse it
                pmemAddr[spsc_slot_base + FLAG_OFFSET] = 0;
            }
        }
    }

    if (pkt->needsResponse()) {
        pkt->makeResponse();
    }
    return true;
}

void
CxlHardwareBuffer::discoverRanksFromMemory()
{
    static bool has_attempted_discovery = false;
    if (has_attempted_discovery)
        return;
    has_attempted_discovery = true;

    if (!pmemAddr) {
        has_attempted_discovery = false;
        return;
    }

    std::map<uint32_t, Addr> discovered;
    Addr page_size = 4096;
    Addr num_pages = range.size() / page_size;

    // The handshake magic lives at (segment_base + HANDSHAKE_MAGIC_OFFSET).
    // The segment is not guaranteed page-aligned (my_segment is at page+8),
    // so the magic can be at page+8 OR page+16. Probe both and anchor the
    // base at the magic location minus HANDSHAKE_MAGIC_OFFSET -- NOT the page.
    for (Addr p = 0; p < num_pages; ++p) {
        Addr page_base = p * page_size;
        for (Addr pgoff = HANDSHAKE_MAGIC_OFFSET; pgoff <= 16; pgoff += 8) {
            Addr magic_addr = page_base + pgoff;
            if (magic_addr + 4 > range.size())
                continue;
            uint32_t val;
            std::memcpy(&val, &pmemAddr[magic_addr], sizeof(uint32_t));
            if ((val & 0xFFFF0000) == 0xC0010000) {
                uint32_t rank = val & 0xFFFF;
                if (rank < numEndpoints &&
                    discovered.find(rank) == discovered.end()) {
                    discovered[rank] = magic_addr - HANDSHAKE_MAGIC_OFFSET;
                }
            }
        }
    }

    if (discovered.size() >= numEndpoints) {
        rankBaseOffsets = discovered;
        allRanksInitialized = true;
    }
}

void
CxlHardwareBuffer::sampleMpscResidency(uint32_t receiver, uint32_t cur_head)
{
    if (receiver >= numEndpoints) {
        return;
    }
    uint32_t &last = mpscLastHead[receiver];
    if (last == 0) {
        // First observation for this rank: anchor without draining (we have no
        // enqueue baseline yet). 0 is never a valid head (slot 0 = control).
        last = cur_head;
        return;
    }

    // Walk the slots the receiver freed since we last looked. Head advances
    // by slotSize and wraps over the MPSC ring; slot 0 is the control block,
    // so it wraps back to slotSize. Bound the walk so a bogus head can't spin.
    auto &enq = mpscEnqTick[receiver];
    unsigned max_iters = (mpscSize / slotSize) + 1;
    while (last != cur_head && max_iters-- > 0) {
        auto it = enq.find(last);
        if (it != enq.end()) {
            Tick resid = curTick() - it->second;
            stats.mpscResidencyCycles[receiver].sample(ticksToCycles(resid));
            enq.erase(it);
        }
        last += slotSize;
        if (last >= mpscSize) {
            last = slotSize;
        }
    }
    // Resync on a bogus/unreachable head so we don't get stuck off-grid.
    last = cur_head;
}

void
CxlHardwareBuffer::completeTransfer(uint32_t receiver,
                                    uint32_t mpsc_offset,
                                    uint32_t spsc_offset)
{
    Addr final_write_offset =
        rankBaseOffsets[receiver] + mpsc_offset + FLAG_OFFSET;

    if (final_write_offset < range.size()) {
        pmemAddr[final_write_offset] = FLAG_COMPLETE;

        // Message is now visible in the MPSC queue: stamp its enqueue time so
        // sampleMpscResidency() can measure how long it sits until consumed.
        if (receiver < numEndpoints) {
            mpscEnqTick[receiver][mpsc_offset] = curTick();
        }

        if (spsc_offset < range.size()) {
            pmemAddr[spsc_offset] = 0;
        }

        Addr base = rankBaseOffsets[receiver];
        uint32_t slot_start_offset = spsc_offset - FLAG_OFFSET;
        uint32_t segment_offset = slot_start_offset - base;
        uint32_t spsc_region_offset = segment_offset - mpscSize;
        uint32_t true_sender = spsc_region_offset / mpscSize;
        uint32_t slot_idx = (spsc_region_offset % mpscSize) / slotSize;
        uint32_t key = (true_sender << 16) | receiver;

        activeMappings[key].erase(slot_idx);
    }

    for (auto it = pendingTransfers.begin();
         it != pendingTransfers.end(); ++it) {
        if (it->receiver == receiver && it->mpsc_offset == mpsc_offset) {
            pendingTransfers.erase(it);
            break;
        }
    }
}

void
CxlHardwareBuffer::serialize(CheckpointOut &cp) const
{
    memory::SimpleMemory::serialize(cp);
    SERIALIZE_SCALAR(allRanksInitialized);
    SERIALIZE_CONTAINER(mpsc_tails);

    std::vector<uint32_t> rbo_keys;
    std::vector<Addr> rbo_vals;
    for (const auto& kv : rankBaseOffsets) {
        rbo_keys.push_back(kv.first);
        rbo_vals.push_back(kv.second);
    }
    SERIALIZE_CONTAINER(rbo_keys);
    SERIALIZE_CONTAINER(rbo_vals);

    std::vector<uint32_t> am_data;
    am_data.push_back(activeMappings.size());
    for (const auto& outer : activeMappings) {
        am_data.push_back(outer.first);
        am_data.push_back(outer.second.size());
        for (const auto& inner : outer.second) {
            am_data.push_back(inner.first);
            am_data.push_back(inner.second);
        }
    }
    SERIALIZE_CONTAINER(am_data);

    uint32_t num_pending = pendingTransfers.size();
    SERIALIZE_SCALAR(num_pending);
    std::vector<uint32_t> pt_receivers;
    std::vector<uint32_t> pt_offsets;
    std::vector<uint32_t> pt_spsc_offsets;
    for (const auto& pt : pendingTransfers) {
        pt_receivers.push_back(pt.receiver);
        pt_offsets.push_back(pt.mpsc_offset);
        pt_spsc_offsets.push_back(pt.spsc_offset);
    }
    SERIALIZE_CONTAINER(pt_receivers);
    SERIALIZE_CONTAINER(pt_offsets);
    SERIALIZE_CONTAINER(pt_spsc_offsets);

    // ---- MPSC overflow subsystem (previously unserialized) ----------------
    // Free-chunk allocator free-list: queue -> vector (queues aren't
    // iterable).
    std::vector<uint32_t> free_chunks_vec;
    {
        std::queue<uint32_t> tmp = freeChunks;
        while (!tmp.empty()) {
            free_chunks_vec.push_back(tmp.front());
            tmp.pop();
        }
    }
    SERIALIZE_CONTAINER(free_chunks_vec);

    // Per-receiver overflow ring state, flattened. Layout:
    //   [num_receivers], then per receiver:
    //     [num_chunks, headOffset, tailOffset, chunk0, chunk1, ...]
    // While walking it, collect the set of allocated chunk indices so we can
    // serialize ONLY their bytes (the rest of the backing store is
    // free/unused).
    std::vector<uint32_t> os_data;
    std::vector<uint32_t> allocated_chunks;
    os_data.push_back(overflowStates.size());
    for (const auto& st : overflowStates) {
        std::queue<uint32_t> tmp = st.chunks;
        os_data.push_back(tmp.size());
        os_data.push_back(st.headOffset);
        os_data.push_back(st.tailOffset);
        while (!tmp.empty()) {
            os_data.push_back(tmp.front());
            allocated_chunks.push_back(tmp.front());
            tmp.pop();
        }
    }
    SERIALIZE_CONTAINER(os_data);

    // Backing-store bytes for allocated chunks only. Empty at a quiescent
    // (post-barrier ROI) checkpoint -> nothing large is written. Each chunk is
    // backingChunkSize bytes; mid-overflow checkpoints will be large here.
    SERIALIZE_CONTAINER(allocated_chunks);
    for (uint32_t ci : allocated_chunks) {
        size_t start = (size_t) ci * backingChunkSize;
        std::vector<uint8_t> chunk_bytes(
            overflowBackingStore.begin() + start,
            overflowBackingStore.begin() + start + backingChunkSize);
        arrayParamOut(cp, csprintf("overflow_chunk_%u", ci), chunk_bytes);
    }
}

void
CxlHardwareBuffer::unserialize(CheckpointIn &cp)
{
    memory::SimpleMemory::unserialize(cp);
    UNSERIALIZE_SCALAR(allRanksInitialized);
    UNSERIALIZE_CONTAINER(mpsc_tails);

    std::vector<uint32_t> rbo_keys;
    std::vector<Addr> rbo_vals;
    UNSERIALIZE_CONTAINER(rbo_keys);
    UNSERIALIZE_CONTAINER(rbo_vals);
    rankBaseOffsets.clear();
    for (size_t i = 0; i < rbo_keys.size(); ++i) {
        rankBaseOffsets[rbo_keys[i]] = rbo_vals[i];
    }

    std::vector<uint32_t> am_data;
    UNSERIALIZE_CONTAINER(am_data);
    activeMappings.clear();
    size_t idx = 0;
    if (!am_data.empty()) {
        uint32_t num_outer = am_data[idx++];
        for (uint32_t o = 0; o < num_outer; ++o) {
            uint32_t key = am_data[idx++];
            uint32_t inner_count = am_data[idx++];
            for (uint32_t i = 0; i < inner_count; ++i) {
                uint32_t spsc_slot = am_data[idx++];
                uint32_t mpsc_off = am_data[idx++];
                activeMappings[key][spsc_slot] = mpsc_off;
            }
        }
    }

    uint32_t num_pending = 0;
    UNSERIALIZE_SCALAR(num_pending);
    pendingTransfers.clear();
    if (num_pending > 0) {
        std::vector<uint32_t> pt_receivers;
        std::vector<uint32_t> pt_offsets;
        std::vector<uint32_t> pt_spsc_offsets;
        UNSERIALIZE_CONTAINER(pt_receivers);
        UNSERIALIZE_CONTAINER(pt_offsets);
        if (cp.sectionExists(gem5::csprintf("%s.pt_spsc_offsets", name()))) {
            UNSERIALIZE_CONTAINER(pt_spsc_offsets);
        } else {
            pt_spsc_offsets.resize(num_pending, 0);
        }
        for (uint32_t i = 0; i < num_pending; ++i) {
            pendingTransfers.push_back(
                {pt_receivers[i], pt_offsets[i], pt_spsc_offsets[i]});
        }
    }

    // ---- MPSC overflow subsystem ----------------------------------------
    // Free-chunk allocator free-list (constructor filled it; replace it).
    std::vector<uint32_t> free_chunks_vec;
    UNSERIALIZE_CONTAINER(free_chunks_vec);
    freeChunks = std::queue<uint32_t>();
    for (uint32_t c : free_chunks_vec) freeChunks.push(c);

    // Per-receiver overflow ring state. Constructor sized overflowStates to
    // numEndpoints; clear each and refill from the flattened os_data.
    for (auto& st : overflowStates) {
        st.chunks = std::queue<uint32_t>();
        st.headOffset = 0;
        st.tailOffset = 0;
    }
    std::vector<uint32_t> os_data;
    UNSERIALIZE_CONTAINER(os_data);
    size_t oidx = 0;
    if (!os_data.empty()) {
        uint32_t num_recv = os_data[oidx++];
        for (uint32_t r = 0; r < num_recv; ++r) {
            uint32_t nchunks = os_data[oidx++];
            uint32_t head = os_data[oidx++];
            uint32_t tail = os_data[oidx++];
            if (r < overflowStates.size()) {
                overflowStates[r].headOffset = head;
                overflowStates[r].tailOffset = tail;
            }
            for (uint32_t k = 0; k < nchunks; ++k) {
                uint32_t ci = os_data[oidx++];
                if (r < overflowStates.size())
                    overflowStates[r].chunks.push(ci);
            }
        }
    }

    // Restore the bytes of the allocated chunks into the backing store.
    std::vector<uint32_t> allocated_chunks;
    UNSERIALIZE_CONTAINER(allocated_chunks);
    for (uint32_t ci : allocated_chunks) {
        std::vector<uint8_t> chunk_bytes;
        arrayParamIn(cp, csprintf("overflow_chunk_%u", ci), chunk_bytes);
        size_t start = (size_t) ci * backingChunkSize;
        if (!chunk_bytes.empty() &&
            start + chunk_bytes.size() <= overflowBackingStore.size()) {
            std::memcpy(&overflowBackingStore[start], chunk_bytes.data(),
                        chunk_bytes.size());
        }
    }
}

void
CxlHardwareBuffer::startup()
{
    if (!pendingTransfers.empty()) {
        auto pending_copy = pendingTransfers;
        for (const auto& pt : pending_copy) {
            completeTransfer(pt.receiver, pt.mpsc_offset, pt.spsc_offset);
        }
    }
}

void
CxlHardwareBuffer::scanForPendingMessages(uint32_t receiver, Addr base)
{
    for (uint32_t sender = 0; sender < numEndpoints; ++sender) {
        if (sender == receiver) continue;

        uint32_t spsc_queue_offset = mpscSize + (sender * mpscSize);
        uint32_t max_slots = mpscSize / slotSize;

        for (uint32_t slot_idx = 0; slot_idx < max_slots; ++slot_idx) {
            uint32_t slot_offset = spsc_queue_offset + (slot_idx * slotSize);
            Addr slot_base = base + slot_offset;
            Addr flag_addr = slot_base + FLAG_OFFSET;

            if (pmemAddr[flag_addr] == FLAG_COMPLETE) {
                // A single FLAG_COMPLETE byte is NOT enough to trust
                // this slot: uninitialised/residue memory (e.g. ext4
                // metadata left behind when the pool is an fsdax file)
                // can carry a stray 0x02 at the flag offset and would be
                // adopted as a forged message, handing the receiver a
                // garbage descriptor (bad len -> memcpy -> crash).
                // Require a coherent header before adopting: the slot's
                // sender_rank field (mca_btl_cxlbuf_hdr_t bytes [0-3])
                // must equal the sender whose SPSC queue we're scanning,
                // and its len field (bytes [20-23]) must be a sane
                // payload size. Real messages satisfy both; residue
                // effectively never does.
                int32_t hdr_sender = 0;
                int32_t hdr_len = 0;
                std::memcpy(&hdr_sender, &pmemAddr[slot_base + 0],
                            sizeof(hdr_sender));
                std::memcpy(&hdr_len, &pmemAddr[slot_base + 20],
                            sizeof(hdr_len));

                bool header_valid =
                    ((uint32_t)hdr_sender == sender) &&
                    (hdr_len >= 0) &&
                    ((uint32_t)hdr_len <= slotSize);

                if (!header_valid) {
                    // Diagnostic: smoking gun for residue/misroute. If
                    // you see this, a flag byte said COMPLETE but the
                    // header did not match -- the slot was NOT a real
                    // message.
                    DPRINTF(CXLBUF, "CXL Hardware Buffer: [INIT SCAN] "
                            "REJECTED slot: Receiver %d, SPSC of Sender %d, "
                            "slot %d -- flag==COMPLETE but hdr_sender=%d "
                            "(expected %d) len=%d (bogus; not adopting)\n",
                            receiver, sender, slot_idx, hdr_sender, sender,
                            hdr_len);
                    continue;
                }

                uint32_t new_mpsc_offset = mpsc_tails[receiver];
                Addr dest_mpsc_addr = base + new_mpsc_offset;

                std::memcpy(&pmemAddr[dest_mpsc_addr],
                            &pmemAddr[slot_base], slotSize);
                pmemAddr[dest_mpsc_addr + FLAG_OFFSET] = FLAG_COMPLETE;
                pmemAddr[flag_addr] = 0;

                DPRINTF(CXLBUF, "CXL Hardware Buffer: [INIT SCAN] Sender "
                        "%d -> Receiver %d found pending message at SPSC "
                        "slot %d (sender_rank=%d, len=%d). Moved to MPSC "
                        "offset 0x%x\n",
                        sender, receiver, slot_idx, hdr_sender, hdr_len,
                        new_mpsc_offset);

                uint32_t next_tail = new_mpsc_offset + slotSize;
                if (next_tail >= mpscSize) {
                    next_tail = slotSize;
                }
                mpsc_tails[receiver] = next_tail;
            }
        }
    }
}

} // namespace gem5
