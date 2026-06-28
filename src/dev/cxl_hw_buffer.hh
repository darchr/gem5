#ifndef __DEV_CXL_HW_BUFFER_HH__
#define __DEV_CXL_HW_BUFFER_HH__

#include <map>
#include <queue>
#include <vector>

#include "base/statistics.hh"
#include "mem/simple_mem.hh"
#include "params/CxlHardwareBuffer.hh"
#include "sim/eventq.hh"

namespace gem5
{

/**
 * CXL Hardware Buffer: A memory device that intercepts writes to
 * translate SPSC (sender) writes into MPSC (receiver) slots,
 * modeling a hardware message routing buffer.
 *
 * Inherits from SimpleMemory so that:
 *  - It registers with PhysicalMemory (isMemAddr returns true)
 *  - KVM memory mapping is automatic (kvmMap)
 *  - It provides a standard ResponsePort for the memory hierarchy
 *  - The backing store (pmemAddr) is managed by PhysicalMemory
 *
 * The routing logic is applied in the overridden access() method,
 * which is called for every cache-line access that reaches the
 * memory controller.
 */
class CxlHardwareBuffer : public memory::SimpleMemory
{
  private:
    const Tick transferLatency;
    const uint32_t numEndpoints;
    const size_t segmentSize;
    const size_t slotSize;
    const size_t mpscSize;

    // The status flag indicating a message is fully written
    // This is used to communicate with the software
    // Receiving process will poll a flag until it sees this value
    // This will need to be back invalidated when we push to cache
    static const uint8_t FLAG_COMPLETE = 2;

    // offsetof(mca_btl_cxlbuf_hdr_t, flags) in the cxlbuf BTL.
    //
    // Current btl_cxlbuf_frag.h compiles to flags @ 17
    // (sender_rank[0-3], pad[4-7], frag[8-15], tag[16], flags[17],
    // len[20-23]).
    // This MUST equal offsetof(flags) in the libmca_btl_cxlbuf.so deployed in
    // the disk image, or the device never sees FLAG_COMPLETE and every
    // receiver polls its MPSC queue forever (deadlock). Verify after any BTL
    // rebuild: the device's "FLAG WRITE TRACE" shows the offset of the size-1
    // value-2 completion store; it must match this constant.
    //
    // (Earlier a stale .so placed flags @ 25; the fix was to rebuild the BTL
    // into the image so the deployed binary matches this source-derived 17.)
    static const uint32_t FLAG_OFFSET = 17;

    // The BTL writes its handshake magic (0xC0010000 | rank) at
    // (my_segment + 8). my_segment is NOT page-aligned (opal_shmem maps it at
    // page+8), so the device must anchor each rank's segment base at
    // (magic_address - HANDSHAKE_MAGIC_OFFSET) instead of rounding the magic
    // address down to a page boundary -- otherwise the base is recorded 8
    // bytes too low and every slot field is seen +8 (flag@17 read as @25).
    static const uint32_t HANDSHAKE_MAGIC_OFFSET = 8;

    // State tracking
    std::vector<uint32_t> mpsc_tails;

    // Physical base offset mapping for each endpoint (Rank -> Base Offset)
    std::map<uint32_t, Addr> rankBaseOffsets;

    // Track whether all ranks have completed handshake.
    // When false, the device acts as simple passthrough memory.
    // When true, SPSC->MPSC routing logic is active.
    bool allRanksInitialized;

    // Mappings: SPSC slot index -> allocated MPSC slot offset
    // Key: (sender_id << 16) | receiver_id
    // Value: map<spsc_slot_idx, mpsc_offset>
    std::map<uint32_t, std::map<uint32_t, uint32_t>> activeMappings;

    // Track in-flight transfers so they survive checkpoint/restore.
    // Each entry is a (receiver, mpsc_offset) pair for a transfer that
    // has been scheduled but not yet completed.
    struct PendingTransfer
    {
        uint32_t receiver;
        uint32_t mpsc_offset;
        uint32_t spsc_offset;
    };
    std::vector<PendingTransfer> pendingTransfers;

    // Hardware Backing Store for MPSC Overflow
    uint64_t backingSize;
    uint32_t backingChunkSize;
    Tick backingLatency;
    std::vector<uint8_t> overflowBackingStore;
    std::queue<uint32_t> freeChunks;

    struct ReceiverOverflowState
    {
        // The 32MB chunks currently owned by this receiver
        std::queue<uint32_t> chunks;
        // Read pointer inside the FIRST chunk (chunks.front())
        uint32_t headOffset;
        // Write pointer inside the LAST chunk (chunks.back())
        uint32_t tailOffset;
    };
    std::vector<ReceiverOverflowState> overflowStates;

    // Event to handle delayed transfer completion
    // Transfers from the SPSC virtual queue to and actual MPSC queue
    class TransferEvent : public Event
    {
      private:
        CxlHardwareBuffer *device;
        uint32_t receiver;
        uint32_t mpsc_offset;
        uint32_t spsc_offset;
      public:
        TransferEvent(CxlHardwareBuffer *d, uint32_t rec,
                      uint32_t off, uint32_t spsc_off)
            : Event(), device(d), receiver(rec),
              mpsc_offset(off), spsc_offset(spsc_off) {}
        void process() override {
            device->completeTransfer(receiver, mpsc_offset, spsc_offset);
        }
    };

    // Helper functions
    uint32_t getRankFromOffset(Addr offset);
    void completeTransfer(uint32_t receiver, uint32_t mpsc_offset,
                          uint32_t spsc_offset);
    void discoverRanksFromMemory();
    void scanForPendingMessages(uint32_t receiver, Addr base);

    /**
     * Apply SPSC->MPSC write routing logic.
     * Called from access() for write packets in routing mode.
     * Returns true if the write was handled (routed), false if
     * the caller should fall through to normal memory access.
     */
    bool handleRoutedWrite(PacketPtr pkt, Addr offset, unsigned size);

    struct CxlHardwareBufferStats : public statistics::Group
    {
        CxlHardwareBufferStats(statistics::Group *parent);
        statistics::Histogram mpscOccupancy;
    } stats;

  public:
    PARAMS(CxlHardwareBuffer);
    CxlHardwareBuffer(const Params &p);

    /**
     * Override AbstractMemory::access() to intercept writes
     * and apply SPSC->MPSC routing before they hit the backing store.
     */
    void access(PacketPtr pkt) override;

    // ===== EXPERIMENT START: CXLBUFEARLY tracing =====
    /**
     * Override functionalAccess() too: gem5 can read/write memory
     * functionally (debugger, some DMA, KVM, fast-forward) WITHOUT going
     * through access(), so any such traffic would bypass the routing logic
     * and leave no CXLBUF trace. Instrumented under CXLBUFEARLY to catch it.
     */
    void functionalAccess(PacketPtr pkt) override;
    // ===== EXPERIMENT END: CXLBUFEARLY tracing =====

    /** Checkpoint serialization support */
    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;

    /** Re-schedule pending transfers after restore */
    void startup() override;
};

} // namespace gem5

#endif // __DEV_CXL_HW_BUFFER_HH__
