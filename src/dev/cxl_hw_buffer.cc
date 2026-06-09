#include "dev/cxl_hw_buffer.hh"

#include <cstring>

#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/CXLBUF.hh"
#include "mem/packet_access.hh"
#include "sim/serialize.hh"
#include "sim/system.hh"

namespace gem5
{

CxlHardwareBuffer::CxlHardwareBuffer(const Params &p)
    : memory::SimpleMemory(p),
      transferLatency(p.transfer_latency),
      numEndpoints(p.num_endpoints),
      segmentSize(p.segment_size),
      slotSize(p.slot_size),
      mpscSize(p.mpsc_size),
      allRanksInitialized(false)
{
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
    // For non-write packets or packets without data, just do normal
    // memory access (reads, clean evictions, invalidations, etc.)
    // Currently this relies on the software to correctly read from
    // the circular buffer. Which is an actual memory backed
    // circular buffer
    if (!pkt->isWrite() || !pkt->hasData()) {
        if (allRanksInitialized && pkt->isRead()) {
            Addr offset = pkt->getAddr() - range.start();
            uint32_t rank = getRankFromOffset(offset);
            if (rank < numEndpoints) {
                Addr base = rankBaseOffsets[rank];
                Addr segment_offset = offset - base;
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
    // PASSTHROUGH MODE: If not all MPI ranks have registered yet,
    // treat the device as simple memory. This allows the kernel to
    // run ndctl, mkfs, mount, etc. without triggering MPI routing.
    // ================================================================
    if (!allRanksInitialized) {
        // If ranks were written during KVM (bypassing our access()),
        // try to discover them from the backing memory.
        // This should only be for when booting from a checkpoint
        discoverRanksFromMemory();

        if (!allRanksInitialized) {
            // Check for an initialization handshake via PIO:
            // A 4-byte write to offset 8 of a page boundary (due to
            // OPAL shmem header), or offset 0 of a page boundary.
            bool is_handshake = false;
            Addr base_offset = 0;
            uint32_t rank = 0;

            if (pkt->hasData()) {
                uint8_t *data = pkt->getPtr<uint8_t>();
                Addr page_base = offset - (offset % 4096);

                // The handshake is a 4-byte value at offset 0, 8, or 16
                // of a page boundary.
                for (Addr pgoff = 0; pgoff <= 16; pgoff += 8) {
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
                                base_offset = page_base;
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
                    inform("CXL_HW_BUFFER: All %d ranks initialized. "
                           "Switching to routing mode.", numEndpoints);
                }
            }
            // Always do normal write in passthrough mode
            memory::AbstractMemory::access(pkt);
            return;
        }
        // If discoverRanksFromMemory() found all ranks, fall through
        // to routing mode below.
    }

    // ================================================================
    // ROUTING MODE: All ranks are initialized; apply MPI routing logic.
    // ================================================================

    // Check for handshake writes (re-initialization)
    bool is_re_handshake = false;
    Addr re_base_offset = 0;
    uint32_t re_rank = 0;

    if (pkt->hasData()) {
        uint8_t *data = pkt->getPtr<uint8_t>();
        Addr page_base = offset - (offset % 4096);

        for (Addr pgoff = 0; pgoff <= 16; pgoff += 8) {
            Addr target_addr = page_base + pgoff;
            if (offset <= target_addr &&
                (offset + size) >= (target_addr + 4)) {
                Addr data_idx = target_addr - offset;
                uint32_t val;
                std::memcpy(&val, &data[data_idx], 4);
                if ((val & 0xFFFF0000) == 0xC0010000) {
                    re_rank = val & 0xFFFF;
                    if (re_rank < numEndpoints) {
                        is_re_handshake = true;
                        re_base_offset = page_base;
                        break;
                    }
                }
            }
        }
    }

    if (is_re_handshake) {
        rankBaseOffsets[re_rank] = re_base_offset;
        DPRINTF(CXLBUF, "CXL Hardware Buffer: Re-handshake. Rank %d "
                "mapped to offset 0x%lx\n", re_rank, re_base_offset);
        memory::AbstractMemory::access(pkt);
        return;
    }

    // Try to handle via routing logic; if not applicable, do normal write
    if (!handleRoutedWrite(pkt, offset, size)) {
        memory::AbstractMemory::access(pkt);
    }
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
        // Not an SPSC write — it's a illegal write to the MPSC directly
        // or some other region (e.g. control metadata).
        return false;
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

    // slot_idx=0 is the control block (cxlbuf_fifo_t). It should not be
    // routed to the MPSC queue as a message, otherwise it consumes the
    // first MPSC slot!
    if (slot_idx == 0) {
        return false;
    }

    if (slot_idx <= 2) {
        DPRINTF(CXLBUF, "SPSC WRITE: sender=%d, receiver=%d, slot_idx=%d, "
                "offset=%d, size=%d, data[0]=%02x\n",
                sender, receiver, slot_idx, slot_internal_offset,
                size, size > 0 ? data[0] : 0);
    }

    uint32_t key = (sender << 16) | receiver;

    // Have we allocated an MPSC slot for this?
    if (activeMappings[key].find(slot_idx) ==
        activeMappings[key].end()) {
        // Check if this is an OS zeroing pass.
        // If all bytes are 0, do not route it.
        // may need to check this logic, is there a chance
        // a message is all 0's?
        bool is_all_zeros = true;
        for (unsigned i = 0; i < size; ++i) {
            if (data[i] != 0) {
                is_all_zeros = false;
                break;
            }
        }
        if (is_all_zeros) {
            return false; // Passthrough to backing memory
        }

        // Allocate a new MPSC slot on the RECEIVER's MPSC queue!
        uint32_t new_mpsc_offset = mpsc_tails[receiver];
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

    uint32_t dest_mpsc_offset = activeMappings[key][slot_idx];

    // Validate destination is in bounds
    if (rankBaseOffsets.find(receiver) == rankBaseOffsets.end()) {
        warn("CXL Hardware Buffer: No base offset for receiver %d. "
             "Writing to backing memory directly.\n", receiver);
        return false;
    }

    // Translate the write directly to the allocated MPSC slot
    Addr final_write_offset = rankBaseOffsets[receiver] +
                               dest_mpsc_offset + slot_internal_offset;

    // Final bounds check on the translated address
    if (final_write_offset + size > range.size()) {
        warn("CXL Hardware Buffer: Translated write out of bounds "
             "(0x%lx + %d > 0x%lx). Writing to original offset.\n",
             final_write_offset, size, range.size());
        return false;
    }

    bool is_complete_flag_write = false;

    // Check if this write encompasses the FLAG_OFFSET
    if (slot_internal_offset <= FLAG_OFFSET &&
        (slot_internal_offset + size) > FLAG_OFFSET) {
        uint8_t flag_val = data[FLAG_OFFSET - slot_internal_offset];

        DPRINTF(CXLBUF, "FLAG WRITE TRACE: sender=%d, receiver=%d, "
                "slot=%d, offset=%d, size=%d, extracted_flag=%02x\n",
                sender, receiver, slot_idx, slot_internal_offset,
                size, flag_val);

        if (flag_val == FLAG_COMPLETE) {
            is_complete_flag_write = true;
            // DO NOT write the complete flag into backing memory yet!
            // We will schedule an event to write it later.
            for (unsigned i = 0; i < size; ++i) {
                if ((slot_internal_offset + i) != FLAG_OFFSET) {
                    pmemAddr[final_write_offset + i] = data[i];
                }
            }
        } else {
            std::memcpy(&pmemAddr[final_write_offset], data, size);
        }
    } else {
        std::memcpy(&pmemAddr[final_write_offset], data, size);
    }

    if (is_complete_flag_write) {
        DPRINTF(CXLBUF, "CXL Hardware Buffer: COMPLETE flag detected! "
                "Scheduling hardware transfer for Receiver %d at "
                "MPSC offset 0x%x\n",
                receiver, dest_mpsc_offset);

        // Remove mapping since it's complete
        activeMappings[key].erase(slot_idx);

        // Track this transfer so it survives checkpoint/restore
        pendingTransfers.push_back({receiver, dest_mpsc_offset});

        // Schedule the event to finalize the message
        Event* e = new TransferEvent(this, receiver, dest_mpsc_offset);
        schedule(e, curTick() + transferLatency);
    }


    // We handled the write ourselves — make the response
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

    // Scan each page boundary (and page boundary + 8) for a valid rank ID
    if (!pmemAddr) {
        // Reset so we can try again once pmemAddr is initialized
        has_attempted_discovery = false;
        return;
    }

    DPRINTF(CXLBUF, "Attempting Discovery\n");

    std::map<uint32_t, Addr> discovered;

    // Scan every 4KB page in the range
    Addr page_size = 4096;
    Addr num_pages = range.size() / page_size;

    for (Addr p = 0; p < num_pages; ++p) {
        Addr page_base = p * page_size;

        // Check offset 8 (OPAL shmem header case)
        if (page_base + 8 + 4 <= range.size()) {
            uint32_t val;
            std::memcpy(&val, &pmemAddr[page_base + 8], sizeof(uint32_t));
            if ((val & 0xFFFF0000) == 0xC0010000) {
                uint32_t rank = val & 0xFFFF;
                if (rank < numEndpoints &&
                    discovered.find(rank) == discovered.end()) {
                    printf("Candidate rank %d found at PA 0x%lx (offset 8): ",
                           rank, (unsigned long)(range.start() + page_base));
                    for (int i = 0; i < 16; ++i) {
                        printf("%02x ", pmemAddr[page_base + i]);
                    }
                    printf("\n");
                    discovered[rank] = page_base;
                }
            }
        }

        // Check offset 0 (standard case)
        if (page_base + 4 <= range.size()) {
            uint32_t val;
            std::memcpy(&val, &pmemAddr[page_base], sizeof(uint32_t));
            if ((val & 0xFFFF0000) == 0xC0010000) {
                uint32_t rank = val & 0xFFFF;
                if (rank < numEndpoints &&
                    discovered.find(rank) == discovered.end()) {
                    printf("Candidate rank %d found at PA 0x%lx (offset 0): ",
                           rank, (unsigned long)(range.start() + page_base));
                    for (int i = 0; i < 16; ++i) {
                        printf("%02x ", pmemAddr[page_base + i]);
                    }
                    printf("\n");
                    discovered[rank] = page_base;
                }
            }
        }
    }


    for (const auto& kv : discovered) {
        inform("Discovered node %d at address %lu",
                kv.first, (unsigned long)kv.second);
    }

    if (discovered.size() >= numEndpoints) {
        rankBaseOffsets = discovered;
        allRanksInitialized = true;
        inform("CXL_HW_BUFFER: Discovered all %d ranks from backing memory:",
               numEndpoints);
        for (const auto& kv : rankBaseOffsets) {
            inform("  CXL_HW_BUFFER_INIT: Rank %d at PA 0x%lx",
                   kv.first,
                   (unsigned long)(range.start() + kv.second));
        }
        inform("CXL_HW_BUFFER: Switching to routing mode.");
    }
}

void
CxlHardwareBuffer::completeTransfer(uint32_t receiver, uint32_t mpsc_offset)
{
    // Write the COMPLETE flag into the receiver's MPSC slot
    Addr final_write_offset = rankBaseOffsets[receiver] + mpsc_offset +
                                FLAG_OFFSET;
    if (final_write_offset < range.size()) {
        pmemAddr[final_write_offset] = FLAG_COMPLETE;
        DPRINTF(CXLBUF, "CXL Hardware Buffer: Transfer completed. "
                "Receiver %d MPSC slot at 0x%x is now visible.\n",
                receiver, mpsc_offset);
    } else {
        warn("CXL Hardware Buffer: completeTransfer out of bounds for "
             "receiver %d at offset 0x%x\n", receiver, mpsc_offset);
    }

    // Remove from pending list
    for (auto it = pendingTransfers.begin(); it != pendingTransfers.end();
         ++it) {
        if (it->receiver == receiver && it->mpsc_offset == mpsc_offset) {
            pendingTransfers.erase(it);
            break;
        }
    }
}

void
CxlHardwareBuffer::serialize(CheckpointOut &cp) const
{
    SERIALIZE_SCALAR(allRanksInitialized);
    SERIALIZE_CONTAINER(mpsc_tails);

    // Flatten rankBaseOffsets map into parallel vectors
    std::vector<uint32_t> rbo_keys;
    std::vector<Addr> rbo_vals;
    for (const auto& kv : rankBaseOffsets) {
        rbo_keys.push_back(kv.first);
        rbo_vals.push_back(kv.second);
    }
    SERIALIZE_CONTAINER(rbo_keys);
    SERIALIZE_CONTAINER(rbo_vals);

    // Flatten activeMappings into a linear representation
    // Format: count of outer entries, then for each: key, inner_count,
    //         then for each inner: spsc_slot, mpsc_offset
    std::vector<uint32_t> am_data;
    am_data.push_back(activeMappings.size());
    for (const auto& outer : activeMappings) {
        am_data.push_back(outer.first);  // key = (sender<<16)|receiver
        am_data.push_back(outer.second.size());
        for (const auto& inner : outer.second) {
            am_data.push_back(inner.first);   // spsc_slot_idx
            am_data.push_back(inner.second);  // mpsc_offset
        }
    }
    SERIALIZE_CONTAINER(am_data);

    // Serialize pending transfers
    uint32_t num_pending = pendingTransfers.size();
    SERIALIZE_SCALAR(num_pending);
    std::vector<uint32_t> pt_receivers;
    std::vector<uint32_t> pt_offsets;
    for (const auto& pt : pendingTransfers) {
        pt_receivers.push_back(pt.receiver);
        pt_offsets.push_back(pt.mpsc_offset);
    }
    SERIALIZE_CONTAINER(pt_receivers);
    SERIALIZE_CONTAINER(pt_offsets);
}

void
CxlHardwareBuffer::unserialize(CheckpointIn &cp)
{
    UNSERIALIZE_SCALAR(allRanksInitialized);
    UNSERIALIZE_CONTAINER(mpsc_tails);

    // Rebuild rankBaseOffsets from parallel vectors
    std::vector<uint32_t> rbo_keys;
    std::vector<Addr> rbo_vals;
    UNSERIALIZE_CONTAINER(rbo_keys);
    UNSERIALIZE_CONTAINER(rbo_vals);
    rankBaseOffsets.clear();
    for (size_t i = 0; i < rbo_keys.size(); ++i) {
        rankBaseOffsets[rbo_keys[i]] = rbo_vals[i];
    }

    // Rebuild activeMappings from linear representation
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

    // Restore pending transfers (will be completed in startup())
    uint32_t num_pending = 0;
    UNSERIALIZE_SCALAR(num_pending);
    pendingTransfers.clear();
    if (num_pending > 0) {
        std::vector<uint32_t> pt_receivers;
        std::vector<uint32_t> pt_offsets;
        UNSERIALIZE_CONTAINER(pt_receivers);
        UNSERIALIZE_CONTAINER(pt_offsets);
        for (uint32_t i = 0; i < num_pending; ++i) {
            pendingTransfers.push_back({pt_receivers[i], pt_offsets[i]});
        }
    }

    if (allRanksInitialized) {
        inform("CXL_HW_BUFFER: Restored from checkpoint with %d ranks "
               "and %zu pending transfers.",
               (int)rankBaseOffsets.size(), pendingTransfers.size());
    }
}

void
CxlHardwareBuffer::startup()
{
    // After restore, immediately complete any pending transfers.
    // The data is already in backing memory from the checkpoint;
    // we just need to set the COMPLETE flags.
    if (!pendingTransfers.empty()) {
        inform("CXL_HW_BUFFER: Completing %zu pending transfers from "
               "checkpoint.", pendingTransfers.size());
        // Copy the vector since completeTransfer modifies it
        auto pending_copy = pendingTransfers;
        for (const auto& pt : pending_copy) {
            completeTransfer(pt.receiver, pt.mpsc_offset);
        }
    }
}

} // namespace gem5
