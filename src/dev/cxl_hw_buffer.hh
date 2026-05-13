#ifndef __DEV_CXL_HW_BUFFER_HH__
#define __DEV_CXL_HW_BUFFER_HH__

#include <map>
#include <vector>

#include "dev/io_device.hh"
#include "params/CxlHardwareBuffer.hh"
#include "sim/eventq.hh"

namespace gem5
{

class KvmVM;

class CxlHardwareBuffer : public BasicPioDevice
{
  private:
    const Tick transferLatency;
    const uint32_t numEndpoints;
    const size_t segmentSize;
    const size_t slotSize;
    const size_t mpscSize;

    // The status flag indicating a message is fully written
    static const uint8_t FLAG_COMPLETE = 2;
    // Offset within a message slot where the flags byte is located
    // 4 bytes rank + 4 bytes padding + 8 bytes pointer + 1 byte tag = 17 bytes
    static const uint32_t FLAG_OFFSET = 17;

    // Backing simple memory
    std::vector<uint8_t> backingMemory;

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

    // Event to handle delayed transfer completion
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

  public:
    typedef CxlHardwareBufferParams Params;
    CxlHardwareBuffer(const Params &p);

    /**
     * Register the backing memory with KVM so the guest can access it
     * as real RAM during KVM execution (avoiding MMIO VM-exit overhead
     * and supporting atomic/exclusive instructions).
     */
    void startup() override;

    Tick read(PacketPtr pkt) override;
    Tick write(PacketPtr pkt) override;

    AddrRangeList getAddrRanges() const override;
};

} // namespace gem5

#endif // __DEV_CXL_HW_BUFFER_HH__
