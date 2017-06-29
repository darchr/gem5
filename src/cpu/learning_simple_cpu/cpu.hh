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
    * In theory, this could be the LSQ if you wanted to allow multiple requests
    */
    class CPUPort : public MasterPort
    {
      private:
        /// The request that is outstanding.
        MemoryRequest *outstandingRequest;

        /// If we tried to send a packet and it was blocked, store it here
        PacketPtr blockedPacket;

      public:
        /**
        * Constructor. Just calls the superclass constructor.
        */
        CPUPort(const std::string& name, LearningSimpleCPU *owner) :
          MasterPort(name, owner), outstandingRequest(nullptr),
          blockedPacket(nullptr)
        { }

        /**
        * Send a packet across this port. This is called by the owner and
        * all of the flow control is hanled in this function.
        *
        * @param the main request associated with this packet. It's possible
        *        that the request has multiple packets (e.g., split req)
        * @param packet to send.
        */
        void sendPacket(MemoryRequest *request, PacketPtr pkt);

        /**
         * Always check this before trying to send a request across this port.
         * For now, we only allow a single outstanding request, so if there is
         * one we should block the port
         *
         * @return true if the port is blocked
         */
        bool isBlocked() { return outstandingRequest == nullptr; }

      protected:
        /**
        * Receive a timing response from the slave port. This will call the
        * recvResponse function of the corresponding MemoryRequest object.
        *
        * @param the packet recvResponse
        * @return false if the CPU can't process this response now
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

    /// Contains the context of the thread and other information. This is the
    /// "hardware context" of the processor.
    SimpleThread thread;

    /// When executing a macroop, I think it's *required* to keep this pointer
    /// around. I don't think there's any way around it.
    StaticInstPtr currentMacroOp;

    /// For tracing with Exec debug flags
    Trace::InstRecord *traceData;

    /**
     * The fetch "stage" of our "single cycle" processor.
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
     * This function should kick off the actual memory request for the inst.
     * This needs to be in the CPU so the CPU can deal with the faults.
     *
     * @param the request object associated with this fetch
     */
    void finishFetchTranslate(MemoryRequest *request);

    /**
     * Called from the memory request when it has finished translating.
     * This function should kick off the actual memory request.
     * This needs to be in the CPU so it can deal with faults and special
     * memory accesses (e.g., mmaped IPR)
     *
     * @param the request object associated with this memory access
     */
    void finishDataTranslate(MemoryRequest *request);

    /**
     * Decode the instruction fetched and found in pkt. Called from the memory
     * request.
     *
     * @param the (whole) packet that contains the fetched data.
     */
    void decodeInstruction(PacketPtr pkt);

    /**
     * Called after the memory request in memory send finishes (for loads)
     *
     * @param the whole packet that contains the read data, or the write
     * @param the (micro) instruction that initiated this request
     */
    void dataResponse(PacketPtr pkt, StaticInstPtr inst);

};

#endif
