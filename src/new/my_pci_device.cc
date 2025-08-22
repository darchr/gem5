#include "dev/pci/device.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "params/MyPCIDevice.hh"
#include "sim/system.hh"

namespace gem5 {

class MyPCIDevice : public PciDevice {
public:
    MyPCIDevice(const Params *p) : PciDevice(p, 0x100) {
        // Set vendor and device ID
        config.vendor = 0x1234;
        config.device = 0x11e8;
        config.command = 0x0000;
        config.status = 0x0000;
        config.revision = 0x01;
        config.classCode = 0xff0000; // Vendor-specific
    }

    Tick readConfig(PacketPtr pkt) override {
        return PciDevice::readConfig(pkt);
    }

    Tick writeConfig(PacketPtr pkt) override {
        return PciDevice::writeConfig(pkt);
    }

    Tick read(PacketPtr pkt) override {
        pkt->setLE<uint32_t>(0xdeadbeef);
        pkt->makeAtomicResponse();
        return 100;
    }

    Tick write(PacketPtr pkt) override {
        pkt->makeAtomicResponse();
        return 100;
    }
};

MyPCIDevice *createMyPCIDevice(const MyPCIDeviceParams *p) {
    return new MyPCIDevice(p);
}
};
