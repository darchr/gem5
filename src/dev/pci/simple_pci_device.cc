#include "dev/pci/simple_pci_device.hh"
#include "base/trace.hh"
#include "debug/SimplePciDevice.hh"

namespace gem5 {

SimplePciDevice::SimplePciDevice(const SimplePciDeviceParams &p)
    : PciDevice(p), pioAddr(p.pio_addr), pioSize(p.pio_size)
{
    // Allocate BAR0
    // config.space.config[PCI_BAR0] = pioAddr | PCI_BASE_ADDRESS_SPACE_IO;
}

Tick
SimplePciDevice::read(PacketPtr pkt)
{
    DPRINTF(SimplePciDevice, "Read request at offset %#x\n", pkt->getAddr());
    pkt->makeResponse();
    pkt->setLE<uint32_t>(0xDEADBEEF); // Dummy data
    return pioDelay;
}

Tick
SimplePciDevice::write(PacketPtr pkt)
{
    DPRINTF(SimplePciDevice, "Write request at offset %#x\n", pkt->getAddr());
    pkt->makeResponse();
    return pioDelay;
}

AddrRangeList
SimplePciDevice::getAddrRanges() const
{
    return { RangeSize(pioAddr, pioSize) };
}


// SimplePciDevice *
// SimplePciDeviceParams::create() const
// {
//     return new SimplePciDevice(*this);
// }

}   // namespace gem5
