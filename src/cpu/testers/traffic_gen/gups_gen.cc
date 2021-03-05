#include "cpu/testers/traffic_gen/gups_gen.hh"

#include <cstring>
#include <string>

#include "base/random.hh"
#include "debug/GUPSGen.hh"

#define POLY 0x0000000000000007UL
#define PERIOD 1317624576693539401L


GUPSGen::GUPSGen(const GUPSGenParams &params):
    ClockedObject(params),
    system(params.system),
    requestorId(system->getRequestorId(this)),
    nextSendEvent([this]{ sendNextBatch(); }, name()),
    port(name() + ".port", this),
    startAddr(params.start_addr),
    memSize(params.mem_size),
    blockSize(params.block_size)
{}

Port&
GUPSGen::getPort(const std::string &if_name, PortID idx)
{
    if (if_name != "port"){
        return ClockedObject::getPort(if_name, idx);
    }
    else{
        return port;
    }
}

void
GUPSGen::startup()
{

    uint64_t stride_size = blockSize / sizeof(uint64_t);

    for (uint64_t start_index = 0; start_index < tableSize;
                                start_index += stride_size)
    {
        uint8_t write_data[blockSize];
        for (uint64_t offset = 0; offset < stride_size; offset++)
        {
            uint64_t value = start_index + offset;
            std::memcpy(write_data + offset * sizeof(uint64_t),
                        &value, sizeof(uint64_t));
        }
        Addr addr;
        uint64_t byte_offset;
        std::tie(addr, byte_offset) = indexToAddr(start_index);
        PacketPtr pkt = getWritePacket(addr, write_data);
        port.sendFunctionalPacket(pkt);
    }

    /*
     * initialize memory by functional access
     * call generateNextBatch here.
    */
   generateNextBatch();
}

void
GUPSGen::init()
{

    tableSize = memSize / sizeof(uint64_t);
    numUpdates = 4 * tableSize;

    for (int i = 0; i < 128; i++)
    {
        randomSeeds[i] = randomNumberGenerator((numUpdates / 128) * i);
    }

}

void
GUPSGen::generateNextBatch()
{
    static uint64_t iterations = 0;

    if (iterations >= numUpdates / 128){
        return;
    }

    Addr addr;
    uint64_t byte_offset;

    for (int j = 0; j < 128; j++)
    {
        randomSeeds[j] = (randomSeeds[j] << 1) ^
                        ((int64_t) randomSeeds[j] < 0 ? POLY : 0);
        uint64_t value = randomSeeds[j];
        uint64_t index = randomSeeds[j] & (tableSize - 1);
        std::tie(addr, byte_offset) = indexToAddr(index);
        translationTable[addr].push_back(std::make_pair(byte_offset, value));
    }

    for (auto &entry : translationTable){
        PacketPtr pkt = getReadPacket(entry.first);
        requestPool.push(pkt);
    }

    if (!nextSendEvent.scheduled() && !requestPool.empty()){
        schedule(nextSendEvent, curTick());
    }
}


void
GUPSGen::sendNextBatch()
{
    if (port.blocked()){
        return;
    }

    PacketPtr pkt = requestPool.front();

    exitTimes[pkt] = curTick();
    port.sendTimingPacket(pkt);

    requestPool.pop();

    if (!nextSendEvent.scheduled() && !requestPool.empty())
    {
        schedule(nextSendEvent, curTick() + 250);
    }

    return;
}

PacketPtr
GUPSGen::getReadPacket(Addr addr)
{
    RequestPtr req = std::make_shared<Request>(addr, blockSize,
                                            0, requestorId);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr)requestorId) << 2);

    // Embed it in a packet
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);

    uint8_t* pkt_data = new uint8_t[req->getSize()];
    pkt->dataDynamic(pkt_data);

    return pkt;
}

PacketPtr
GUPSGen::getWritePacket(Addr addr, uint8_t *data)
{
    RequestPtr req = std::make_shared<Request>(addr, blockSize, 0,
                                               requestorId);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr)requestorId) << 2);

    // Embed it in a packet
    PacketPtr pkt = new Packet(req, MemCmd::WriteReq);

    uint8_t* pkt_data = new uint8_t[req->getSize()];
    pkt->dataDynamic(pkt_data);

    // std::fill_n(pkt_data, req->getSize(), (uint8_t)requestorId);
    std::memcpy(pkt_data, data, req->getSize());

    return pkt;
}


uint64_t
GUPSGen::randomNumberGenerator(int64_t n)
{
    int i, j;
    uint64_t m2[64];
    uint64_t temp, ran;

    while (n < 0) n += PERIOD;
    while (n > PERIOD) n -= PERIOD;
    if (n == 0) return 0x1;

    temp = 0x1;
    for (i = 0; i < 64; i++)
    {
        m2[i] = temp;
        temp = (temp << 1) ^ ((int64_t) temp < 0 ? POLY : 0);
        temp = (temp << 1) ^ ((int64_t) temp < 0 ? POLY : 0);
    }

    for (i = 62; i >= 0; i--)
    {
        if ((n >> i) & 1)
            break;
    }

    ran = 0x2;
    while (i > 0)
    {
        temp = 0;
        for (j = 0; j < 64; j++)
            if ((ran >> j) & 1)
            temp ^= m2[j];
        ran = temp;
        i -= 1;
        if ((n >> i) & 1)
            ran = (ran << 1) ^ ((int64_t) ran < 0 ? POLY : 0);
    }

    return ran;
}

std::pair<Addr, uint64_t>
GUPSGen::indexToAddr (uint64_t index)
{
    Addr ret = index * 8 + startAddr;

    uint64_t byte_offset = ret % blockSize;

    ret = ret - byte_offset;

    return std::make_pair(ret, byte_offset);
}

void
GUPSGen::handleResponse(PacketPtr pkt)
{
    responsePool.push(pkt);
}

void
GUPSGen::MemSidePort::sendTimingPacket(PacketPtr pkt)
{
    panic_if(_blocked, "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
        _blocked = true;
    }
}

void
GUPSGen::MemSidePort::recvReqRetry()
{
    // We should have a blocked packet if this function is called.
    assert(_blocked && blockedPacket != nullptr);
    // Try to resend it. It's possible that it fails again.
    _blocked = false;
    sendTimingPacket(blockedPacket);
    blockedPacket = nullptr;

    owner->wakeUp();
}

bool
GUPSGen::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    owner->handleResponse(pkt);
    return true;
}



void
GUPSGen::MemSidePort::sendFunctionalPacket(PacketPtr pkt)
{
    sendFunctional(pkt);
}

void GUPSGen::wakeUp()
{
    if (!nextSendEvent.scheduled() && !requestPool.empty()){
        schedule(nextSendEvent, curTick());
    }
}
