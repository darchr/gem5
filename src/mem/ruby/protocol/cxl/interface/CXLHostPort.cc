#include "mem/ruby/protocol/cxl/interface/CXLHostPort.hh"

#include "base/trace.hh"
#include "debug/CXLHostPort.hh"

namespace gem5
{

namespace ruby
{

namespace CXL
{

CXLHostPort::CXLHostPort(const Params &params)
    : ClockedObject(params),
      rubySystemPtr(params.ruby_system),
      hostSidePort(this, name() + ".cpu_side"),
      requests(params.request_latency),
      responses(params.response_latency),
      requestEvent([this](){ processRequestEvent(); }, name() + "requestEvent"),
      responseEvent([this](){ processResponseEvent(); }, name() + "responseEvent")
{
    /*
     * We can use -1 as a proxy for infinity, in case we don't want to model any
     * bandwidth or buffer size.
     */
    for (auto &range : params.mem_ranges) {
        memRanges.push_back(range);
    }

    if (params.request_queue_size == -1) {
        reqQueueSize = std::numeric_limits<int>::max();
    } else {
        reqQueueSize = params.request_queue_size;
    }

    if (params.request_issue_width == -1) {
        requestIssueWidth = std::numeric_limits<int>::max();
    } else {
        requestIssueWidth = params.request_issue_width;
    }
}

void
CXLHostPort::init()
{
    assert(rubyController != nullptr);
}

void
CXLHostPort::initiateMemoryRequest(PacketPtr pkt)
{
    // Get the device that we should set as the destination

    // Create a CXL request packet
    std::shared_ptr<CXLMemRequestMsg> cxl_pkt = std::make_shared<CXLMemRequestMsg>(curTick(), pkt->getSize(), rubySystemPtr);
    cxl_pkt->m_addr = pkt->getAddr();
    outstandingRequests[cxl_pkt->m_addr] = pkt;

    if (pkt->isRead()) {
        cxl_pkt->m_Type = CXL_M2S_Req_Type_MemRd;
    }
    
    rubyController->getMandatoryQueue()->enqueue(cxl_pkt, clockEdge(), 0, false, false);

    // need to return whether the request was sent or not
}

bool
CXLHostPort::recvTimingReq(PacketPtr pkt) 
{
    if (requests.size() < reqQueueSize) {
        requests.push(pkt, curTick());
        // REMEMBER: Should we also check if an event has already been scheduled here?
        // We don't want to schedule the same event multiple times.
        if (!requestEvent.scheduled()) {
            scheduleNextProcessRequestEvent(nextCycle());
        }
        return true;
    }
    return false;
}

void
CXLHostPort::scheduleNextProcessRequestEvent(Tick when)
{
    panic_if(requestEvent.scheduled(), "Trying to schedule the requestEvent that has already been scheduled.\n");

    // Check if the time specified by scheduler is earlier than the first ready
    // time of the packet.
    Tick first_ready_time = requests.firstReadyTime();
    Tick schedule_tick = std::max(when, first_ready_time);

    // If the queue is empty, firstReadyTime returns MaxTick.
    if (schedule_tick != MaxTick) {
        schedule(requestEvent, schedule_tick);
    }
}

void
CXLHostPort::processRequestEvent()
{
    // Issue a certain number of requests (in accordance to the requestIssueWidth.

    for (int i = 0; i < requestIssueWidth; i++) {
        // Check if we have requests that can be issued
        if (!requests.hasReady(curTick())) {
            break;
        }
        DPRINTF(CXLHostPort, "Initiating Request %s", requests.front());
        initiateMemoryRequest(requests.front());
        // NOTE: We can pop this because initiateMemoryRequest tracks this 
        // value in a separate map.
        requests.pop();
    }
    // We don't want to schedule the same event multiple times.
    if (!requestEvent.scheduled()) {
        scheduleNextProcessRequestEvent(nextCycle());
    }
}

bool
CXLHostPort::HostSidePort::recvTimingReq(PacketPtr pkt)
{
    DPRINTF(CXLHostPort, "Got request %s\n", pkt);
    return owner->recvTimingReq(pkt);
}

Tick
CXLHostPort::HostSidePort::recvAtomic(PacketPtr pkt)
{
    panic("CXLHostPort doesn't expect atomic requests\n");
    return MaxTick;
}

void
CXLHostPort::HostSidePort::recvFunctional(PacketPtr pkt)
{
    panic("CXLHostPort doesn't expect functional requests\n");
}

AddrRangeList
CXLHostPort::HostSidePort::getAddrRanges() const
{
    return owner->memRanges;
}

void
CXLHostPort::HostSidePort::recvRespRetry()
{
     panic("CXLHostPort doesn't expect response retries\n");
}

Port&
CXLHostPort::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "host_side_port") {
        return hostSidePort;
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

void
CXLHostPort::responseCallback(Addr addr, DataBlock data)
{
    panic_if(outstandingRequests.find(addr) == outstandingRequests.end(), "Could not find addr %#x in outstanding requests.\n", addr);
    PacketPtr pkt = outstandingRequests[addr];
    pkt->makeResponse();
    responses.push(pkt, curTick());
    // Don't need to schedule a response event if one is already scheduled
    if (!responseEvent.scheduled()) {
        scheduleNextProcessResponseEvent(curTick());
    }
    
}

void
CXLHostPort::scheduleNextProcessResponseEvent(Tick when)
{
    panic_if(responseEvent.scheduled(), "Trying to schedule the responseEvent that has already been scheduled.\n");
    Tick first_ready_time = responses.firstReadyTime();
    Tick schedule_tick = std::max(when, first_ready_time);
    if (schedule_tick != MaxTick) {
        schedule(responseEvent, schedule_tick);
    }
}

void
CXLHostPort::processResponseEvent()
{
    if (responses.hasReady(curTick())) {
        if (!hostSidePort.sendTimingResp(responses.front())) {
            panic("%s: Failed to send pkt.\n", __func__);
        }
        responses.pop();
    }
    // Don't schedule a response event if one is already scheduled
    if (!responseEvent.scheduled()) {
        scheduleNextProcessResponseEvent(nextCycle());
    }
}

} // namespace CXL

} // namespace ruby

} // namespace gem5
