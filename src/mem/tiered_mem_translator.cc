
#include "mem/tiered_mem_translator.hh"

#include <vector>

#incldue "base/addr_range.hh"

namespace gem5
{

TieredMemTranslator::TieredMemTranslator(const Params& params):
    SimObject(params),
    cpuSide = CPUSidePort("cpu_side", this),
    offset(params.addr_offset)
{
    for (int i = 0; i < params.port_mem_side_ports_connection_count; i++) {
        memSide.emplace_back("mem_side" + std::to_string(i), this);
    }
}

Port&
TieredMemTranslator::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "cpu_side") {
        return cpuSide;
    } else if (if_name == "mem_side") {
        return memSide[idx];
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

// virtual AddrRangeList getAddrRanges() const;

//       protected:
//         virtual bool recvTimingReq(PacketPtr pkt);
//         virtual Tick recvAtomic(PacketPtr pkt);
//         virtual void recvFunctional(PacketPtr pkt);
//         virtual void recvRespRetry();

void
TieredMemTranslator::init()
{
    std::vector<AddrRange> ranges;

    for (auto& port: memSide) {
        AddrRangeList range_list = port.getAddrRanges();
        for (auto range: range_list) {
            ranges.push_back(range);
        }
    }

    AddrRange range = AddrRange(ranges);
    AddrRange offset_range = AddrRange(range.start() + offset, range.end() + offset);
    rangeList.insert(offset_range);
}

AddrRangeList
TieredMemTranslator::CPUSidePort::getAddrRanges()
{

}

} // namespace gem5