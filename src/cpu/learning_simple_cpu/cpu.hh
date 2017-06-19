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

#include <vector>

#include "config/the_isa.hh"
#include "cpu/base.hh"
#include "cpu/simple_thread.hh"
#include "params/LearningSimpleCPU.hh"

class LearningSimpleCPU : public BaseCPU
{
  private:
    /**
    * Port that sends requests and receives responses.
    * Currently only allows a single outstanding request at a time.
    */
    class CPUPort : public MasterPort
    {
      private:
        /// The object that owns this object (SimpleMemobj)
        LearningSimpleCPU *owner;

        /// If we tried to send a packet and it was blocked, store it here
        PacketPtr blockedPacket;

      public:
        /**
        * Constructor. Just calls the superclass constructor.
        */
        CPUPort(const std::string& name, LearningSimpleCPU *owner) :
          MasterPort(name, owner), owner(owner), blockedPacket(nullptr)
        { }

        /**
        * Send a packet across this port. This is called by the owner and
        * all of the flow control is hanled in this function.
        *
        * @param packet to send.
        */
        void sendPacket(PacketPtr pkt);

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
    CPUPort port;

    /// True when there is a memory request outstanding. Right now we are only
    /// supporting a single memory request at a time.
    bool memOutstanding;

    /// Contains the context of the thread an other information, I guess
    SimpleThread *thread;

    /**
     * Called when we receive a response from memory. We previous sent a
     * request.
     *
     * @param the packet that we're receiving from memory.
     * @return false if we can't handle the packet this cycle.
     */
    bool handleResponse(PacketPtr pkt);


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


};
