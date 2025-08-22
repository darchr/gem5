// src/dev/my_uio_device.cc
#include "dev/io_device.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"

#include "params/MyUIODevice.hh"
#include "debug/MyUIODevice.hh"
#include "sim/system.hh"

class MyUIODevice : public BasicPioDevice {
public:
    MyUIODevice(const Params *p) : BasicPioDevice(p, p->pio_size) {}

    Tick read(PacketPtr pkt) override {
        // Simulate reading from device
        uint32_t data = 0x12345678;
        pkt->setUintX(data, pkt->getSize());
        pkt->makeAtomicResponse();
        return pioDelay;
    }

    Tick write(PacketPtr pkt) override {
        // Simulate writing to device
        uint32_t data = pkt->getUintX(pkt->getSize());
        DPRINTF(MyUIODevice, "Write: 0x%x\n", data);
        pkt->makeAtomicResponse();
        return pioDelay;
    }

    AddrRangeList getAddrRanges() const override {
        return {RangeSize(pioAddr, pioSize)};
    }
};
