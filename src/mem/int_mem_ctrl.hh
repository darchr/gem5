/*
*Copyright (c) 2021 The Regents of the University of California.
*All Rights Reserved
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

#ifndef __INT_MEM_CTRL_HH__
#define __INT_MEM_CTRL_HH__

#include <deque>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/callback.hh"
#include "base/statistics.hh"
#include "enums/MemSched.hh"
#include "mem/qos/mem_ctrl.hh"
#include "mem/qport.hh"
#include "params/IntMemCtrl.hh"
#include "sim/eventq.hh"

// Probably MemPacket is not really needed or at least
// most of the stuff in it will not be needed

class MemPacket
{
  public:

    /** When did request enter the controller */
    const Tick entryTime;

    /** When will request leave the controller */
    Tick readyTime;

    /** This comes from the outside world */
    const PacketPtr pkt;

    /** RequestorID associated with the packet */
    const RequestorID _requestorId;

    const bool read;

    /** Does this packet access DRAM?*/
    const bool dram;

    /** Will be populated by address decoder */
    const uint8_t rank;
    const uint8_t bank;
    const uint32_t row;

    /**
     * Bank id is calculated considering banks in all the ranks
     * eg: 2 ranks each with 8 banks, then bankId = 0 --> rank0, bank0 and
     * bankId = 8 --> rank1, bank0
     */
    const uint16_t bankId;

    /**
     * The starting address of the packet.
     * This address could be unaligned to burst size boundaries. The
     * reason is to keep the address offset so we can accurately check
     * incoming read packets with packets in the write queue.
     */
    Addr addr;

    /**
     * The size of this dram packet in bytes
     * It is always equal or smaller than the burst size
     */
    unsigned int size;

    /**
     * A pointer to the BurstHelper if this MemPacket is a split packet
     * If not a split packet (common case), this is set to NULL
     */
    BurstHelper* burstHelper;

    /**
     * QoS value of the encapsulated packet read at queuing time
     */
    uint8_t _qosValue;

    /**
     * Set the packet QoS value
     * (interface compatibility with Packet)
     */
    inline void qosValue(const uint8_t qv) { _qosValue = qv; }

    /**
     * Get the packet QoS value
     * (interface compatibility with Packet)
     */
    inline uint8_t qosValue() const { return _qosValue; }

    /**
     * Get the packet RequestorID
     * (interface compatibility with Packet)
     */
    inline RequestorID requestorId() const { return _requestorId; }

    /**
     * Get the packet size
     * (interface compatibility with Packet)
     */
    inline unsigned int getSize() const { return size; }

    /**
     * Get the packet address
     * (interface compatibility with Packet)
     */
    inline Addr getAddr() const { return addr; }

    /**
     * Return true if its a read packet
     * (interface compatibility with Packet)
     */
    inline bool isRead() const { return read; }

    /**
     * Return true if its a write packet
     * (interface compatibility with Packet)
     */
    inline bool isWrite() const { return !read; }

    /**
     * Return true if its a DRAM access
     */
    inline bool isDram() const { return dram; }

    MemPacket(PacketPtr _pkt, bool is_read, bool is_dram, uint8_t _rank,
               uint8_t _bank, uint32_t _row, uint16_t bank_id, Addr _addr,
               unsigned int _size)
        : entryTime(curTick()), readyTime(curTick()), pkt(_pkt),
          _requestorId(pkt->requestorId()),
          read(is_read), dram(is_dram), rank(_rank), bank(_bank), row(_row),
          bankId(bank_id), addr(_addr), size(_size), burstHelper(NULL),
          _qosValue(_pkt->qosValue())
    { }

};



class IntMemCtrl : public QoS::MemCtrl
{


private:

    /**
     * The controller's read and write pending queues
     */
    std::vector<MemPacket*> wpq;
    std::vector<MemPacket*> rpq;


    void flush_wpq();
    void flush_rpq();

  private:

    // For now, make use of a queued response port to avoid dealing with
    // flow control for the responses being sent back
    class MemoryPort : public QueuedResponsePort
    {

        RespPacketQueue queue;
        MemCtrl& ctrl;

      public:

        MemoryPort(const std::string& name, MemCtrl& _ctrl);

      protected:

        Tick recvAtomic(PacketPtr pkt) override;
        Tick recvAtomicBackdoor(
                PacketPtr pkt, MemBackdoorPtr &backdoor) override;

        void recvFunctional(PacketPtr pkt) override;

        bool recvTimingReq(PacketPtr) override;

        AddrRangeList getAddrRanges() const override;

    };

    /**
     * Our incoming port, for a multi-ported controller add a crossbar
     * in front of it
     */
    MemoryPort port;

    /**
     * The controller's main read and write queues,
     * with support for QoS reordering
     */
    std::vector<MemPacketQueue> readQueue;
    std::vector<MemPacketQueue> writeQueue;

    // pointers to actual memory device interfaces
    DRAMInterface* const dram;
    NVMInterface* const nvm;


    /**
     * Memory controller configuration initialized based on parameter
     * values.
     */
    Enums::MemSched memSchedPolicy;

    // to mimic the controller latency
    const Tick SomeLatency;


  public:

    IntMemCtrl(const IntMemCtrlParams &p);

  protected:

    Tick recvAtomic(PacketPtr pkt);
    Tick recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &backdoor);
    void recvFunctional(PacketPtr pkt);
    bool recvTimingReq(PacketPtr pkt);

};

#endif //__INT_MEM_CTRL_HH__
