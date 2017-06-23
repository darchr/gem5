/*
 * Copyright (c) 2017 Jason Lowe-Power
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
 *
 * Authors: Jason Lowe-Power
 */

#ifndef __CPU_LEARNING_SIMPLE_CPU_HH__
#define __CPU_LEARNING_SIMPLE_CPU_HH__

#include <vector>

#include "config/the_isa.hh"
#include "cpu/base.hh"
#include "cpu/learning_simple_cpu/memory_request.hh"
#include "cpu/simple_thread.hh"
#include "params/LearningSimpleCPU.hh"
#include "sim/insttracer.hh"

class LearningSimpleCPU : public BaseCPU
{
  public:
    /**
    * Port that sends requests and receives responses.
    * Currently only allows a single outstanding request at a time.
    */
    class CPUPort : public MasterPort
    {
      private:
        /// The object that owns this object (SimpleMemobj)
        LearningSimpleCPU *owner;

        /// The request that is outstanding.
        MemoryRequest *outstandingRequest;

        /// If we tried to send a packet and it was blocked, store it here
        PacketPtr blockedPacket;

      public:
        /**
        * Constructor. Just calls the superclass constructor.
        */
        CPUPort(const std::string& name, LearningSimpleCPU *owner) :
          MasterPort(name, owner), owner(owner), outstandingRequest(nullptr),
          blockedPacket(nullptr)
        { }

        /**
        * Send a packet across this port. This is called by the owner and
        * all of the flow control is hanled in this function.
        *
        * @param packet to send.
        */
        void sendPacket(MemoryRequest *request, PacketPtr pkt);

      protected:
        /**
        * Receive a timing response from the slave port.
        */
        bool recvTimingResp(PacketPtr pkt) override;

        /**
        * Called by the slave port if sendTimingReq was called on this
        * master port (causing recvTimingReq to be called on the slave
        * port) and was unsuccesful.
        */
        void recvReqRetry() override;
    };

  private:

    /// Currently we only have one port that is shared between instruciton and
    /// data fetch.
    CPUPort port;

    /// Contains the context of the thread an other information, I guess
    SimpleThread thread;

    /// When executing a macroop, I think it's *required* to keep this pointer
    /// around. I don't think there's any way around it.
    StaticInstPtr currentMacroOp;

    /// For tracing with Exec debug flags
    Trace::InstRecord *traceData;

    /**
     * Called when we receive a response from memory. We previous sent a
     * request.
     *
     * @param the packet that we're receiving from memory.
     * @return false if we can't handle the packet this cycle.
     */
    bool handleResponse(PacketPtr pkt);

    /**
     * The fetchTranslate "stage" of our "single cycle" processor
     * This sends a request for the next PC address to the TLB
     *
     * @param the offset from the instruction address to fetch. This is needed
     *        when the instruction is bigger than we expect.
     *        This defaults to 0 in the common case (initial fetch).
     */
    void fetch(Addr offset = 0);

    /**
     * Execute the instruction
     */
    void executeInstruction(StaticInstPtr inst);

    /**
     * Cleanup any extra things used for execute (e.g., tracing) and move to
     * the next instruction
     */
    void finishExecute(StaticInstPtr inst, const Fault &fault);


  public:
    /**
     * Constructor. Nothing special.
     */
    LearningSimpleCPU(LearningSimpleCPUParams *params);

    /**
     * Initialize some things (called after constructor and before startup)
     * NOTE: Be sure to call BaseCPU::init() in this function!
     */
    void init() override;

    /**
     * Startup some things (called after init, when the first simulate())
     * NOTE: Be sure to call BaseCPU::startup() in this function!
     */
    void startup() override;

    /**
     * Start a particular thread context. This kicks off the first events.
     * This should call the superclass's implementation as well.
     *
     * @param the thread ID to activate
     */
    void activateContext(ThreadID tid) override;

    /**
     * @return a reference to the data port (a CPU port)
     */
    MasterPort &getDataPort() override { return port; };

    /**
     * @return a reference to the instruction port (a CPU port). Currently
     *         there is only one port which is a shared inst/data port.
     */
    MasterPort &getInstPort() override { return port; };

    /**
    * @return the total number of instructions committed so far
    */
    Counter totalInsts() const override { return 0; }

    /**
     * @return the total number of ops committed so far
     */
    Counter totalOps() const override { return 0; }

    /**
     * Wake up the CPU and continue executing the next PC?
     *
     * @param The thread ID to wake up. Currently, this CPU does not support
     *        more than one thread, so this parameter is ignored.
     */
    void wakeup(ThreadID tid) override;

    /**
     * Called from the memory request when it has finished translating.
     * This function should kick off the actual memory request.
     */
    void finishFetchTranslate(MemoryRequest *request);

    /**
     * Called from the memory request when it has finished translating.
     * This function should kick off the actual memory request.
     */
    void finishDataTranslate(MemoryRequest *request);

    /**
     * Decode the instruction fetched and found in pkt. Called from the memory
     * request.
     */
    void decodeInstruction(PacketPtr pkt);

    /**
     * Called after the memory request in memory send finishes (for loads)
     */
    void dataResponse(PacketPtr pkt, StaticInstPtr inst);

};

#endif
