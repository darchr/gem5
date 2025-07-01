#ifndef __DEV_PCI_SIMPLE_PCI_DEVICE_HH__
#define __DEV_PCI_SIMPLE_PCI_DEVICE_HH__

#include "dev/pci/device.hh"
#include "params/SimplePciDevice.hh"

namespace gem5 {
class SimplePciDevice : public PciDevice
{
  public:
    SimplePciDevice(const SimplePciDeviceParams &p);

    // SimplePciDevice* create();

    Tick read(PacketPtr pkt) override;
    Tick write(PacketPtr pkt) override;
    AddrRangeList getAddrRanges() const override;

  private:
    Addr pioAddr;
    Addr pioSize;
};
}   // namespace gem5

#endif // __DEV_PCI_SIMPLE_PCI_DEVICE_HH__
