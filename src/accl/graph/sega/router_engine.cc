/*
 * Copyright (c) 2021 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "accl/graph/sega/router_engine.hh"

namespace gem5
{
RouterEngine::RouterEngine(const Params &params):
  ClockedObject(params),
  gptQSize(params.gpt_queue_size),
  gpnQSize(params.gpn_queue_size),
  nextInteralGPTGPNEvent([this] { processNextInteralGPTGPNEvent(); }, name()),
  nextRemoteGPTGPNEvent([this] { processNextRemoteGPTGPNEvent(); }, name())
{

    for (int i = 0; i < params.port_gpt_req_side_connection_count; ++i) {
        gptReqPorts.emplace_back(
                    name() + ".gpt_req_side" + std::to_string(i), this, i);
    }

    for (int i = 0; i < params.port_gpt_resp_side_connection_count; ++i) {
        gptRespPorts.emplace_back(
                    name() + ".gpt_resp_side" + std::to_string(i), this, i);
    }

    for (int i = 0; i < params.port_gpn_req_side_connection_count; ++i) {
        gpnReqPorts.emplace_back(
                    name() + ".gpn_req_side" + std::to_string(i), this, i);
    }

    for (int i = 0; i < params.port_gpn_resp_side_connection_count; ++i) {
        gpnRespPorts.emplace_back(
                    name() + ".gpn_resp_side" + std::to_string(i), this, i);
    }
}

AddrRangeList
RouterEngine::GPTRespPort::getAddrRanges() const
{
    return owner->getGPNRanges();
}

AddrRangeList
RouterEngine::GPNRespPort::getAddrRanges() const
{
    return owner->getGPTRanges();
}

AddrRangeList
RouterEngine::getGPNRanges()
{
    AddrRangeList ret;
    for (auto &gpnPort : gpnReqPorts) {
        for (auto &addr_range : gpnPort.getAddrRanges()) {
            ret.push_back(addr_range);
        }
    }
    // for(auto i = routerAddrMap.begin(); i != routerAddrMap.end(); ++i) {
    //     ret.push_back(i->second);
    // }
    return ret;
}

AddrRangeList
RouterEngine::getGPTRanges()
{
    AddrRangeList ret;
    for (auto &gptPort : gptReqPorts) {
        for (auto &addr_range : gptPort.getAddrRanges()) {
            ret.push_back(addr_range);
        }
    }
    return ret;
}

void
RouterEngine::init()
{
    for (int i = 0; i < gptReqPorts.size(); i++) {
        gptAddrMap[gptReqPorts[i].id()] = gptReqPorts[i].getAddrRanges();
        for (auto &addrRange: gptReqPorts[i].getAddrRanges()) {
            std::cout<< name()<<", "<<__func__<<addrRange.to_string()<<std::endl;
        }
    }
}

void
RouterEngine::startup()
{
    for (int i = 0; i < gpnReqPorts.size(); i++) {
        routerAddrMap[gpnReqPorts[i].id()] = gpnReqPorts[i].getAddrRanges();
        for (auto &addrRange: gpnReqPorts[i].getAddrRanges()) {
            std::cout<< name()<<", "<<__func__<<addrRange.to_string()<<std::endl;
        }
    }
    std::cout<<"******************"<<std::endl;
}

Port&
RouterEngine::getPort(const std::string& if_name, PortID idx)
{
    if (if_name == "gpt_req_side") {
        return gptReqPorts[idx];
    } else if (if_name == "gpt_resp_side") { 
        return gptRespPorts[idx];
    } else if (if_name == "gpn_req_side") {
        return gpnReqPorts[idx];
    } else if (if_name == "gpn_resp_side") {
        return gpnRespPorts[idx];
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

bool
RouterEngine::GPTReqPort::recvTimingResp(PacketPtr pkt) {
    panic("Not implemented yet!");
    return 0;
}

void
RouterEngine::GPTReqPort::recvReqRetry() {
    panic("Not implemented yet!");
}

bool
RouterEngine::GPNReqPort::recvTimingResp(PacketPtr pkt) {
    panic("Not implemented yet!");
    return 0;
}

void
RouterEngine::GPNReqPort::recvReqRetry() {
    panic("Not implemented yet!");
}

void
RouterEngine::GPNReqPort::sendPacket(PacketPtr pkt) {
    panic_if(blocked(), "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

Tick 
RouterEngine::GPTRespPort::recvAtomic(PacketPtr pkt) {
    panic("Not implemented yet!");
}

bool 
RouterEngine::GPTRespPort::recvTimingReq(PacketPtr pkt) {
    if (!owner->handleRequest(id(), pkt)) {
        return false;
    }
    return true;
}

bool
RouterEngine::handleRequest(PortID portId, PacketPtr pkt)
{
    auto queue = gptReqQueues[portId];
    bool accepted = false;
    if (queue.size() < gptQSize) {
        gptReqQueues[portId].push(pkt);
        accepted = true;
    } else {
        accepted = false;
    }

    if(accepted && (!nextInteralGPTGPNEvent.scheduled())) {
        schedule(nextInteralGPTGPNEvent, nextCycle());
    }

    return accepted;
}

void
RouterEngine::processNextInteralGPTGPNEvent()
{
    bool found = false;
    int queues_none_empty = 0;
    for (auto queue = gptReqQueues.begin(); 
              queue != gptReqQueues.end(); ++queue) {
        if (!queue->second.empty()) {
            PacketPtr pkt = queue->second.front();
            Addr pkt_addr = pkt->getAddr();
            queues_none_empty += 1;
            for (int i = 0; i < gpnReqPorts.size(); i++) {
                AddrRangeList addr_list = routerAddrMap[gpnReqPorts[i].id()];
                if ((contains(addr_list, pkt_addr))) {
                    if (gpnRespQueues[gpnReqPorts[i].id()].size() < gpnQSize) {
                        gpnRespQueues[gpnReqPorts[i].id()].push(pkt);
                        queue->second.pop();
                        found = true;
                        queues_none_empty -= 1;
                        if (!queue->second.empty()) {
                            queues_none_empty += 1;
                        }
                        if ((!nextRemoteGPTGPNEvent.scheduled())) {
                            schedule(nextRemoteGPTGPNEvent, nextCycle());
                        }
                        break;
                    // queue is full
                    } else {
                        found = false;
                        break;
                    }
                }
            }
        }
        if (found) {
            break;
        }
    }

    if (queues_none_empty > 0) {
        schedule(nextInteralGPTGPNEvent, nextCycle());
    }
}

void
RouterEngine::processNextRemoteGPTGPNEvent()
{
    uint32_t none_empty_queue = 0;  
    for (auto queue = gpnRespQueues.begin(); 
              queue != gpnRespQueues.end(); ++queue) {
        if (!queue->second.empty()) {
            if (!gpnReqPorts[queue->first].blocked()) {
                PacketPtr pkt = queue->second.front();
                gpnReqPorts[queue->first].sendPacket(pkt);
                queue->second.pop();
                break;
            }
        }
    }

    for (auto queue = gpnRespQueues.begin(); 
              queue != gpnRespQueues.end(); ++queue) {
        if (!queue->second.empty()) {
            none_empty_queue += 1;
        }
    }

    if (none_empty_queue > 0) {
        schedule(nextRemoteGPTGPNEvent, nextCycle());
    }
}

void 
RouterEngine::GPTRespPort::recvFunctional(PacketPtr pkt) {
    panic("Not implemented yet!");
}

void 
RouterEngine::GPTRespPort::recvRespRetry() {
    panic("Not implemented yet!");
}

Tick 
RouterEngine::GPNRespPort::recvAtomic(PacketPtr pkt) {
    panic("Not implemented yet!");
}

bool 
RouterEngine::GPNRespPort::recvTimingReq(PacketPtr pkt) {
    panic("Not implemented yet!");
}

void 
RouterEngine::GPNRespPort::recvFunctional(PacketPtr pkt) {
    panic("Not implemented yet!");
}

void 
RouterEngine::GPNRespPort::recvRespRetry() {
    panic("Not implemented yet!");
}

// bool
// Router::handleRemoteRequest(PortID portId, PacketPtr pkt)
// {
//     auto queue = find_if(remoteReqQueues.begin(), remoteReqQueues.end(),
//         [portId](RequestQueue &obj) {return obj.cpuPortId == portId;});
//     if (queue->blocked()) {
//         queue->sendReadRetry = true;
//         return false;
//     }
    
//     queue->push(pkt);
//     if (!nextRemoteReqEvent.scheduled()) {
//         schedule(nextReqEvent, nextCycle());
//     }
//     return true;
// }


// void
// Router::processNextRemoteReqEvent() {
//     RequestQueue *queue = NULL;
//     for (auto &it : remoteReqQueues) {
//         if (!it.emptyRead()) {
//             queue = &it;
//             break;
//         }
//     }

//     if (queue == nullptr) {
//         return;
//     }

//     PacketPtr pkt;
//     std::vector<MemSidePort>::iterator memPort;
//     while (true) {
        
//         pkt = queue->readQueue.front();
//         AddrRange addr_range = pkt->getAddrRange();
        
//         PortID localRespPortID = memPortMap.contains(addr_range)->second;
        
//         localRespPort = find_if(localRespPorts.begin(), localRespPorts.end(),
//             [memPortId](LocalRespPort &obj)
//             {return obj.portId() == localRespPortID;});

//         if (!localRespPort->blocked()) {
//             break;
//         }
//         else {
        
//         }
//     }

//     DPRINTF(MemScheduler, "processNextReqEvent: "
//         "Port not blocked! Sending the packet\n");
//     PortID cpuPortId = pick->cpuPortId;

//     memPort->sendPacket(pkt);
//     pick->timesChecked++;

    
        
//     entryTimes.erase(pkt);
//     pick->readQueue.pop();

//     if (!nextReqEvent.scheduled()) {
//         for (auto &queue : requestQueues) {
//             if (!queue.emptyRead() || queue.serviceWrite()) {
//                 DPRINTF(MemScheduler, "processNextReqEvent: "
//                     "Scheduling nextReqEvent in processNextReqEvent\n");
//                 schedule(nextReqEvent, nextCycle());
//                 break;
//             }
//         }
//     }

    
//     if (pick->sendReadRetry && !pick->blocked(pkt->isRead()
//         || pkt->isWrite())) {
//         PortID cpuPortId = pick->cpuPortId;
//         auto cpuPort = find_if(cpuPorts.begin(), cpuPorts.end(),
//             [cpuPortId](CPUSidePort &obj)
//             {return obj.portId() == cpuPortId;});
//         DPRINTF(MemScheduler, "processNextReqEvent: "
//         "Sending read retry to ports previously blocked\n");
//         cpuPort->trySendRetry();
//         pick->sendReadRetry = false;
//     }

//     return;
// }

// void Router::recvRangeChange(PortID portId)
// {
//     for (auto &port : localRespPorts) {
//         if (port.portId() == portId) {
//             AddrRangeList ranges = port.getAddrRanges();
//             for (auto &r : ranges) {
//                 localPortMap.insert(r, portId);
//             }
//         }
//     }
//     sendRangeChange();
// }

}// namespace gem5
