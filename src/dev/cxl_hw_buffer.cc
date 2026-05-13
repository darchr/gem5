#include "dev/cxl_hw_buffer.hh"

#include <cstring>

#include "base/logging.hh"
#include "base/trace.hh"
#include "cpu/kvm/vm.hh"
#include "debug/CXLBUF.hh" // Can use DMA trace flag for now
#include "mem/packet_access.hh"
#include "sim/system.hh"

namespace gem5
{

CxlHardwareBuffer::CxlHardwareBuffer(const Params &p)
    : BasicPioDevice(p, p.pio_size),
      transferLatency(p.transfer_latency),
      numEndpoints(p.num_endpoints),
      segmentSize(p.segment_size),
      slotSize(p.slot_size),
      mpscSize(p.mpsc_size),
      allRanksInitialized(false)
{
    // Initialize backing memory
    backingMemory.resize(pioSize, 0);

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

void
CxlHardwareBuffer::startup()
{
    // Register our backing memory with KVM so the guest kernel can
    // access the PMEM region as real RAM during KVM execution.
    // Without this, accesses cause MMIO VM-exits, and ARM KVM cannot
    // emulate atomic/exclusive instructions (LDXR/STXR) as MMIO,
    // resulting in errno 38 (ENOSYS).
    //
    // After switching from KVM to O3/Timing CPUs, the PIO path is
    // used instead and our read()/write() routing logic takes over.
    KvmVM *kvmVM = sys->getKvmVM();
    if (kvmVM) {
        auto slot = kvmVM->allocMemSlot(pioSize);
        kvmVM->setupMemSlot(slot, backingMemory.data(), pioAddr, 0);
        printf("CXL_HW_BUFFER: Registered 0x%lx bytes at PA 0x%lx "
               "as KVM memory slot.\n",
               (unsigned long)pioSize, (unsigned long)pioAddr);
    }
}

AddrRangeList
CxlHardwareBuffer::getAddrRanges() const
{
    AddrRangeList ranges;
    ranges.push_back(RangeSize(pioAddr, pioSize));
    return ranges;
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

Tick
CxlHardwareBuffer::read(PacketPtr pkt)
{
    Addr offset = pkt->getAddr() - pioAddr;

    // Bounds check
    if (offset + pkt->getSize() > pioSize) {
        warn("CXL Hardware Buffer: read out of bounds at offset 0x%lx, "
             "size %d. Ignoring.\n",
             offset, pkt->getSize());
        pkt->makeAtomicResponse();
        return pioDelay;
    }

    // Simple memory read from the backing array
    std::memcpy(pkt->getPtr<uint8_t>(), &backingMemory[offset],
                pkt->getSize());
    pkt->makeAtomicResponse();

    return pioDelay;
}

Tick
CxlHardwareBuffer::write(PacketPtr pkt)
{
    Addr offset = pkt->getAddr() - pioAddr;
    unsigned size = pkt->getSize();

    // Bounds check
    if (offset + size > pioSize) {
        warn("CXL Hardware Buffer: write out of bounds at offset 0x%lx, "
             "size %d. Ignoring.\n",
             offset, size);
        pkt->makeAtomicResponse();
        return pioDelay;
    }

    // Guard: if the packet has no data (e.g. clean eviction, invalidation),
    // just acknowledge it.
    if (!pkt->hasData()) {
        DPRINTF(CXLBUF, "CXL Hardware Buffer: write with no data (cmd: %s) "
                "at offset 0x%lx. Acknowledging.\n",
                pkt->cmdString(), offset);
        pkt->makeAtomicResponse();
        return pioDelay;
    }

    uint8_t *data = pkt->getPtr<uint8_t>();

    // ================================================================
    // PASSTHROUGH MODE: If not all MPI ranks have registered yet,
    // treat the device as simple memory. This allows the kernel to
    // run ndctl, mkfs, mount, etc. without triggering MPI routing.
    // ================================================================
    if (!allRanksInitialized) {
        // If ranks were written during KVM (bypassing our write()),
        // try to discover them from the backing memory.
        discoverRanksFromMemory();

        if (!allRanksInitialized) {
            // Check for an initialization handshake via PIO:
            // A 4-byte write to offset 0 of a segment-aligned boundary.
            if (offset % segmentSize == 0 && size == 4) {
                uint32_t rank = pkt->getLE<uint32_t>();
                // Sanity: only treat as handshake if rank value looks valid
                if (rank < numEndpoints) {
                    rankBaseOffsets[rank] = offset;
                    printf("CXL_HW_BUFFER_INIT: Rank %d initialized at PA "
                           "0x%lx\n", rank, pioAddr + offset);
                    DPRINTF(CXLBUF, "CXL Hardware Buffer: Handshake "
                            "received. Rank %d mapped to offset 0x%lx\n",
                            rank, offset);

                    if (rankBaseOffsets.size() >= numEndpoints) {
                        allRanksInitialized = true;
                        printf("CXL_HW_BUFFER: All %d ranks initialized. "
                               "Switching to routing mode.\n", numEndpoints);
                    }
                }
            }
            // Always write to backing memory in passthrough mode
            std::memcpy(&backingMemory[offset], data, size);
            pkt->makeAtomicResponse();
            return pioDelay;
        }
        // If discoverRanksFromMemory() found all ranks, fall through
        // to routing mode below.
    }

    // ================================================================
    // ROUTING MODE: All ranks are initialized; apply MPI routing logic.
    // ================================================================

    // Still check for handshake writes (re-initialization)
    if (offset % segmentSize == 0 && size == 4) {
        uint32_t rank = pkt->getLE<uint32_t>();
        if (rank < numEndpoints) {
            rankBaseOffsets[rank] = offset;
            DPRINTF(CXLBUF, "CXL Hardware Buffer: Re-handshake. Rank %d "
                    "mapped to offset 0x%lx\n", rank, offset);
            std::memcpy(&backingMemory[offset], data, size);
            pkt->makeAtomicResponse();
            return pioDelay;
        }
    }

    uint32_t receiver = getRankFromOffset(offset);
    uint32_t segment_offset = offset % segmentSize;

    // Is this a write into an SPSC queue?
    // SPSC queues start after the MPSC (which is mpscSize).
    if (segment_offset >= mpscSize) {
        // Calculate sender and slot
        uint32_t spsc_region_offset = segment_offset - mpscSize;
        uint32_t sender = spsc_region_offset / mpscSize;
        // assuming SPSC sizes are equal to mpscSize (1MB) as in hw_emu.c
        uint32_t slot_idx = (spsc_region_offset % mpscSize) / slotSize;
        uint32_t slot_internal_offset = (spsc_region_offset % mpscSize) %
                                         slotSize;

        uint32_t key = (sender << 16) | receiver;

        // Have we allocated an MPSC slot for this?
        if (activeMappings[key].find(slot_idx) == activeMappings[key].end()) {
            // Allocate a new MPSC slot!
            uint32_t new_mpsc_offset = mpsc_tails[receiver];
            activeMappings[key][slot_idx] = new_mpsc_offset;

            DPRINTF(CXLBUF, "CXL Hardware Buffer: Allocated MPSC slot at "
                    "offset 0x%x for Sender %d -> Receiver %d "
                    "(SPSC slot %d)\n",
                    new_mpsc_offset, sender, receiver, slot_idx);

            // Advance the MPSC tail for future allocations
            uint32_t next_tail = new_mpsc_offset + slotSize;
            if (next_tail >= mpscSize) {
                next_tail = slotSize; // wrap around
            }
            mpsc_tails[receiver] = next_tail;
        }

        uint32_t dest_mpsc_offset = activeMappings[key][slot_idx];

        // Validate destination is in bounds
        if (rankBaseOffsets.find(receiver) == rankBaseOffsets.end()) {
            warn("CXL Hardware Buffer: No base offset for receiver %d. "
                 "Writing to backing memory directly.\n", receiver);
            std::memcpy(&backingMemory[offset], data, size);
            pkt->makeAtomicResponse();
            return pioDelay;
        }

        // Translate the write directly to the allocated MPSC slot
        Addr final_write_offset = rankBaseOffsets[receiver] +
                                   dest_mpsc_offset + slot_internal_offset;

        // Final bounds check on the translated address
        if (final_write_offset + size > pioSize) {
            warn("CXL Hardware Buffer: Translated write out of bounds "
                 "(0x%lx + %d > 0x%lx). Writing to original offset.\n",
                 final_write_offset, size, pioSize);
            std::memcpy(&backingMemory[offset], data, size);
            pkt->makeAtomicResponse();
            return pioDelay;
        }

        bool is_complete_flag_write = false;

        // Check if this write encompasses the FLAG_OFFSET
        if (slot_internal_offset <= FLAG_OFFSET &&
            (slot_internal_offset + size) > FLAG_OFFSET) {
            uint8_t flag_val = data[FLAG_OFFSET - slot_internal_offset];
            if (flag_val == FLAG_COMPLETE) {
                is_complete_flag_write = true;
                // DO NOT write the complete flag into backing memory yet!
                // We will schedule an event to write it later.
                for (unsigned i = 0; i < size; ++i) {
                    if ((slot_internal_offset + i) != FLAG_OFFSET) {
                        backingMemory[final_write_offset + i] = data[i];
                    }
                }
            } else {
                std::memcpy(&backingMemory[final_write_offset], data, size);
            }
        } else {
            std::memcpy(&backingMemory[final_write_offset], data, size);
        }

        if (is_complete_flag_write) {
            DPRINTF(CXLBUF, "CXL Hardware Buffer: COMPLETE flag detected! "
                    "Scheduling hardware transfer for Receiver %d at "
                    "MPSC offset 0x%x\n",
                    receiver, dest_mpsc_offset);

            // Remove mapping since it's complete
            activeMappings[key].erase(slot_idx);

            // Schedule the event to finalize the message
            Event* e = new TransferEvent(this, receiver, dest_mpsc_offset);
            schedule(e, curTick() + transferLatency);
        }

    } else {
        // It's a write to the MPSC directly? Or some other region
        // (e.g. control metadata).
        // Just perform a normal write.
        std::memcpy(&backingMemory[offset], data, size);
    }

    pkt->makeAtomicResponse();
    return pioDelay;
}

void
CxlHardwareBuffer::discoverRanksFromMemory()
{
    // After a KVM -> O3 CPU switch, the MPI_Init handshake writes
    // went directly to backingMemory via KVM's memory slot, bypassing
    // our write() method. Scan each segment boundary for a valid
    // rank ID (a little-endian uint32 at offset 0 of each segment).
    //
    // We verify that:
    //   1. The value at the boundary is a valid rank (< numEndpoints)
    //   2. Each rank appears exactly once
    //   3. All numEndpoints ranks are found
    std::map<uint32_t, Addr> discovered;

    for (uint32_t seg = 0; seg < numEndpoints; ++seg) {
        Addr segStart = (Addr)seg * segmentSize;
        if (segStart + 4 > pioSize)
            break;

        uint32_t rank;
        std::memcpy(&rank, &backingMemory[segStart], sizeof(uint32_t));
        // Little-endian assumption matches pkt->getLE<uint32_t>()

        if (rank < numEndpoints &&
            discovered.find(rank) == discovered.end()) {
            discovered[rank] = segStart;
        }
    }

    if (discovered.size() >= numEndpoints) {
        rankBaseOffsets = discovered;
        allRanksInitialized = true;
        printf("CXL_HW_BUFFER: Discovered all %d ranks from "
               "KVM-written backing memory:\n", numEndpoints);
        for (const auto& kv : rankBaseOffsets) {
            printf("  CXL_HW_BUFFER_INIT: Rank %d at PA 0x%lx\n",
                   kv.first, (unsigned long)(pioAddr + kv.second));
        }
        printf("CXL_HW_BUFFER: Switching to routing mode.\n");
    }
}

void
CxlHardwareBuffer::completeTransfer(uint32_t receiver, uint32_t mpsc_offset)
{
    // Write the COMPLETE flag into the receiver's MPSC slot
    Addr final_write_offset = rankBaseOffsets[receiver] + mpsc_offset +
                               FLAG_OFFSET;
    if (final_write_offset < pioSize) {
        backingMemory[final_write_offset] = FLAG_COMPLETE;
        DPRINTF(CXLBUF, "CXL Hardware Buffer: Transfer completed. "
                "Receiver %d MPSC slot at 0x%x is now visible.\n",
                receiver, mpsc_offset);
    } else {
        warn("CXL Hardware Buffer: completeTransfer out of bounds for "
             "receiver %d at offset 0x%x\n", receiver, mpsc_offset);
    }
}

} // namespace gem5
