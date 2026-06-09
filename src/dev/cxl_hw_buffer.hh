#ifndef __DEV_CXL_HW_BUFFER_HH__
#define __DEV_CXL_HW_BUFFER_HH__

#include <map>
#include <vector>

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

    // Offset within a message slot where the flags byte is located
    // 4 bytes rank + 4 bytes padding + 8 bytes pointer + 1 byte tag = 17 bytes
    static const uint32_t FLAG_OFFSET = 25;

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
    };
    std::vector<PendingTransfer> pendingTransfers;

    // Event to handle delayed transfer completion
    // Transfers from the SPSC virtual queue to and actual MPSC queue
    class TransferEvent : public Event
    {
      private:
        CxlHardwareBuffer *device;
        uint32_t receiver;
        uint32_t mpsc_offset;
      public:
        TransferEvent(CxlHardwareBuffer *d, uint32_t rec, uint32_t off)
            : Event(), device(d), receiver(rec), mpsc_offset(off) {}
        void process() override {
            device->completeTransfer(receiver, mpsc_offset);
        }
    };

    // Helper functions
    uint32_t getRankFromOffset(Addr offset);
    void completeTransfer(uint32_t receiver, uint32_t mpsc_offset);
    void discoverRanksFromMemory();

    /**
     * Apply SPSC->MPSC write routing logic.
     * Called from access() for write packets in routing mode.
     * Returns true if the write was handled (routed), false if
     * the caller should fall through to normal memory access.
     */
    bool handleRoutedWrite(PacketPtr pkt, Addr offset, unsigned size);

  public:
    PARAMS(CxlHardwareBuffer);
    CxlHardwareBuffer(const Params &p);

    /**
     * Override AbstractMemory::access() to intercept writes
     * and apply SPSC->MPSC routing before they hit the backing store.
     */
    void access(PacketPtr pkt) override;

    /** Checkpoint serialization support */
    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;

    /** Re-schedule pending transfers after restore */
    void startup() override;
};

} // namespace gem5

#endif // __DEV_CXL_HW_BUFFER_HH__
