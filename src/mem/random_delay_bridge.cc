
#include "base/random.hh"
#include "mem/random_delay_bridge.hh"

RandomDelayBridge*
RandomDelayBridgeParams::create()
{
    return new RandomDelayBridge(this);
}


bool
RandomDelayBridge::recvTimingReq(PacketPtr pkt)
{
    if (maxDelay == 0) {
        return masterPort.sendTimingReq(pkt);
    }

    // Align the delay to the clock edge
    Cycles max_cycles = ticksToCycles(maxDelay);
    // Note: "Cycles" isn't actually an integral type...
    // max_cycles will be small (< 10 likely)
    Cycles delay_cycles = Cycles(random_mt.random<int>(0, int(max_cycles)));
    Tick delay = cyclesToTicks(delay_cycles);

    // Add the delay to the packet
    pkt->payloadDelay += delay;

    // Send the request
    bool sent = masterPort.sendTimingReq(pkt);

    // If it fails to send, remove the extra delay we added. The packet
    // will be re-sent by the slave.
    if (!sent) {
        pkt->payloadDelay -= delay;
    }

    return sent;
}
