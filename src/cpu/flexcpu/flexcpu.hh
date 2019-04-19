/*
 * Copyright (c) 2018 The Regents of The University of California
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
 * Authors: Bradley Wang
 */

#ifndef __CPU_FLEXCPU_FLEXCPU_HH__
#define __CPU_FLEXCPU_FLEXCPU_HH__

#include <functional>
#include <list>
#include <memory>
#include <vector>

#include "cpu/base.hh"
#include "cpu/flexcpu/flexcpu_thread.hh"
#include "cpu/pred/bpred_unit.hh"
#include "mem/packet.hh"
#include "params/FlexCPU.hh"

class FlexCPUThread;
/**
 * The CPU class represents a core, which contains hardware capable of running
 * one or more threads. This class has the responsibility of managing hardware
 * structures and controlling the timing of events.
 */
class FlexCPU : public BaseCPU
{
  public:
    using ExecCallback = std::function<void(Fault)>;
    using FetchCallback = std::function<void(uint8_t*)>;
    using MemCallback = std::function<void(Fault)>;
    // May need to also return an ID of some sort through the MemCallback,
    // since we may need a way for threads to request the squashing of a
    // request that hasn't actually gone to memory yet.
    using TranslationCallback = std::function<void(Fault,const RequestPtr&)>;

    // BEGIN Public constants

    const Addr cacheBlockMask;

    // END Public constants

  protected:
    // BEGIN Internal class definitions

    class DataPort : public MasterPort
    {
      protected:
        FlexCPU* cpu;
      public:
        PacketPtr blockedPkt = nullptr;

        DataPort(const std::string& name_, FlexCPU* cpu_):
            MasterPort(name_, cpu_),
            cpu(cpu_)
        { }

        virtual ~DataPort()
        { }

        bool recvTimingResp(PacketPtr pkt);

        void recvReqRetry();

        void sendPacket(PacketPtr pkt);

        bool blocked() { return blockedPkt != nullptr; }
    };

    class InstPort : public MasterPort
    {
      protected:
        FlexCPU* cpu;
      public:
        PacketPtr blockedPkt = nullptr;

        InstPort(const std::string& name_, FlexCPU* cpu_):
            MasterPort(name_, cpu_),
            cpu(cpu_)
        { }

        virtual ~InstPort()
        { }

        bool recvTimingResp(PacketPtr pkt);

        void recvReqRetry();

        void sendPacket(PacketPtr pkt);

        bool blocked() { return blockedPkt != nullptr; }
    };

    /**
     * Very simple wrapper for the Translation interface, which takes a
     * std::function in any form. This allows the use of lambdas to handle the
     * completion of the translation instead of needing specialized polymorphic
     * classes.
     *
     * Note that instances of this class are assumed to be allocated through
     * Operator::new, because they will self-delete upon calling finish(). Note
     * that this also requires the assumption that all CallbackTransHandlers
     * constructed will have finish() called on them, otherwise this will cause
     * a memory leak.
     */
    class CallbackTransHandler : public BaseTLB::Translation
    {
      protected:
        FlexCPU *cpu;
        TranslationCallback callback;
        Tick sendTime;
      public:
        CallbackTransHandler(FlexCPU *cpu,
                             TranslationCallback callback,
                             Tick send_time):
            cpu(cpu), callback(callback), sendTime(send_time)
        { }

        void markDelayed() override
        { }

        void finish(const Fault& fault, const RequestPtr& req,
                    ThreadContext* tc, BaseTLB::Mode mode) override
        {
            callback(fault, req);

            // It requires assumptions that users of this will be well-behaved
            // to call "delete this;" in a C++ class, but since this is an
            // internal class, we should be able to make this assumption.
            delete this;
        }
    };

    // END Internal class definitions

    // BEGIN Internal container structs

    struct SplitAccCtrlBlk
    {
        // Packet used to communicate to the instruction (completeAcc)
        // The data always is allocated in this packet. The other packets have
        // static pointers to the data here.
        PacketPtr main = nullptr;

        // Used for sending/receiving data from memory system
        PacketPtr low = nullptr;
        bool lowReceived = false;
        PacketPtr high = nullptr;
        bool highReceived = false;
    };

    // END Internal container structs


    // BEGIN Internal parameters

    Cycles executionLatency;

    // END Internal parameters

    /**
     * Models a functional unit. Each unit has a latency, in cycles, and a
     * bandwidth in accesses per cycle. A latency of 0 implies the action is
     * immediately taken. Otherwise, the action of the functional unit is not
     * functionally "executed" until a clock edge after latency cycles.
     *
     * NOTE: We might want to make this a SimObject so it can be configured
     *       from python more easily.
     */
    class Resource
    {
      protected:
        FlexCPU* cpu;

      private:
        const Cycles latency;
        const int bandwidth;
        const std::string _name;

        int usedBandwidth = 0;

        Tick lastActiveTick = MaxTick;

        // All of the functions that we need to call the next time we can.
        // Right now, this is an infinite queue. We should at least check to
        // make sure it doesn't get too large.
        std::list<std::function<bool ()>> requests;

        // This is executed to try to schedule the requests for execution.
        // We should always check to make sure this *isn't* scheduled before
        // scheduling it.
        EventFunctionWrapper attemptAllEvent;

        /**
         * For each request in the request queue (requests) execute it until
         * we run out of available resources
         * */
        void attemptAllRequests();

        /// Stats
        Stats::Scalar activeCycles;
        Stats::Histogram bandwidthPerCycle;

      protected:
        /**
         * Checks to see if we can issue a request to this resource.
         * By default, this only checks the bandwidth for this cycle. However,
         * other implementations might check other things (e.g., if the port is
         * available)
         *
         * @return true if the request can be "executed" on this resource
         */
        virtual bool resourceAvailable();

      public:
        /**
         * @param the cpu that this functional unit is part of. Needed for
         *        the clock.
         * @param the number of cycles to wait before "executing" the request
         * @param the number of requests that can be handled per cycle
         * @param the name of this unit (for debugging and stats)
         * @param whether the event should run last (with lower priority) in
         *        the event queue. Needed for fetches.
         */
        Resource(FlexCPU *cpu,
                 Cycles latency, int bandwidth,
                 std::string _name, bool run_last = false);

        /**
         * Add a function to execute to the queue of requests for this resource
         * to "execute".
         *
         * @param run_function is the function to run when the resource is
         *        available and *after* latency cycles.
         *        The return value of the function is true if the function
         *        actually "executes" and false if the instruction was squashed
         */
        void addRequest(const std::function<bool()>& run_function);

        /**
         * Get the name of this resources. Used for debugging and stats
         */
        const std::string& name() { return _name; }

        /**
         * Make sure the event to fire attemptAllRequests is scheduled.
         * The event will alwyas be scheduled for the current tick or the next
         * clock edge.
         * Note: this may be able to be moved into addRequest except for the
         *       case of memory retries.
         */
        void schedule();

        /**
         * Register the stats. Must be called by the CPU since this isn't a
         * SimObject.
         */
        void regStats();
    };

    class InstFetchResource : public Resource
    {
      public:
        InstFetchResource(FlexCPU *cpu, int bandwidth,
                          std::string _name);
      private:
        bool resourceAvailable() override;
    };

    class MemoryResource : public Resource
    {
      public:
        MemoryResource(FlexCPU *cpu, int bandwidth,
                       std::string _name);
      private:
        bool resourceAvailable() override;
    };

    /// Resources for each unit.
    Resource dataAddrTranslationUnit;
    Resource executionUnit;
    InstFetchResource instFetchUnit;
    Resource instAddrTranslationUnit;
    Resource issueUnit;
    MemoryResource memoryUnit;

    // BEGIN Internal state variables

    DataPort _dataPort;
    InstPort _instPort;

    /**
     * Branch predictor object pointer. One should be selected by the python
     * configuration and assigned via parameter. Currently each CPU will only
     * hold one, which will be shared across all threads requesting access to
     * it.
     */
    BPredUnit* _branchPred;

    // Using unique_ptr for now since holding objects directly results
    // in calls to the object's copy constructor, which SimpleThread does not
    // like.
    std::vector<std::unique_ptr<FlexCPUThread>> threads;

    // Note: I was informed that the pointer you get back is the same pointer
    //       you send through the port, so it's safe to do this mapping. It may
    //       be safer to do this mapping through the SenderState struct instead
    //       based on what has been learned since the writing of this though.
    std::unordered_map<PacketPtr, FetchCallback> outstandingFetches;
    std::unordered_map<PacketPtr, std::function<void()>> outstandingMemReqs;

    // Tick that the CPU was last active for tracking active cycles
    Tick lastActiveTick = MaxTick;

    // BEGIN Statistics

    Stats::Histogram memLatency;

    // For resources
    Stats::Histogram waitingForDataXlation;
    Stats::Histogram waitingForExecution;
    Stats::Histogram waitingForInstXlation;
    Stats::Histogram waitingForInstData;
    Stats::Histogram waitingForMem;

    // END Statistics

    // END Internal state variables


    // BEGIN Internal functions

    void completeMemAccess(PacketPtr orig_pkt, StaticInstPtr inst,
                           std::weak_ptr<ExecContext> context,
                           Trace::InstRecord* trace_data,
                           MemCallback callback,
                           Tick send_time,
                           SplitAccCtrlBlk* split = nullptr);

    /**
     * For the resources to notify CPU that they are active and avoid over
     * counting the CPU's active cycles
     */
    void markActiveCycle();

    // TODO some form of speculative state flush function?

    // END Internal functions


  public:
    /**
     * Constructor.
     */
    FlexCPU(FlexCPUParams* params);

    /**
     * Virtual destructor explicitly declared, so that this class can be
     * subclassed.
     */
    virtual ~FlexCPU() = default;

    // BEGIN Member functions (sorted alphabetically)

    /**
     * Called to set up the state to run a particular thread context. This also
     * kicks off the first simulation
     *
     * @param tid The ID of the thread that was activated
     */
    void activateContext(ThreadID tid) override;

    void suspendContext(ThreadID tid) override;
    void haltContext(ThreadID tid) override;

    /**
     * Non-event-driven way to access the branch predictor. Should only be used
     * to notify the branch predictor of instructions being squashed or
     * committed.
     *
     * @return BPredUnit* The branch predictor owned by the CPU.
     */
    BPredUnit* getBranchPredictor();

    /**
     * Interface calls to retrieve a reference to the port used for data or
     * instructions respectively.
     */
    MasterPort& getDataPort() override;
    MasterPort& getInstPort() override;

    /**
     * Checks if the CPU was configured with a branch predictor.
     *
     * @return true Has a non-NULL branch predictor
     * @return false Has no non-NULL branch predictor
     */
    bool hasBranchPredictor() const;

    /**
     * Called after constructor but before running. (Make sure to call
     * BaseCPU::init() in this function to let it do its job)
     */
    void init() override;

    /**
     * Event-driven way to request access to the branch predictor unit held by
     * the CPU. Callback should be expected to be on the same tick, unless
     * access to the branch predictor is constrained via prediction time or
     * number of ports.
     *
     * @param callback_func the callback through which the predictor will be
     *  provided.
     */
    void requestBranchPredictor(
        std::function<void(BPredUnit* pred)> callback_func);

    /**
     * Event-driven means for classes to request a translation of a particular
     * address via this CPU's DTB. Upon completion of this translation
     * (immediate or not), this CPU implementation should return the values
     * retrieved from the DTB via the callback function, and whether or not
     * there was any fault as a result of the request.
     *
     * @param req A memory request object, constructed using the virtual
     *  address we want translated
     * @param tc A pointer to the ThreadContext for the request, since the dtb
     *  is specific to a thread
     * @param write Whether this is a translation for a write (if not, a read)
     * @param callback_func The function reference for purposes of handling the
     *  result of the translation
     */
    void requestDataAddrTranslation(const RequestPtr& req, ThreadContext* tc,
                                    bool write,
                                    std::weak_ptr<ExecContext> context,
                                    TranslationCallback callback_func);

    /**
     * Event-driven means for other classes to request a timing-based execution
     * of a given instruction within a given context. Upon completion of the
     * execution of the instruction, this CPU implementation should notify the
     * requester by calling the callback function and passing any fault state
     * resulting from execution.
     *
     * Note: we define "completion" of an instruction as the earliest point at
     * which we could commit this instruction in isolation.
     *
     * @param inst Reference to an instruction to execute.
     * @param context Reference to a context interface in which to execute the
     *  instruction
     * @param callback_func The function reference for purposes of handling the
     *  result of the execution
     */
    void requestExecution(StaticInstPtr inst,
                          std::weak_ptr<ExecContext> context,
                          Trace::InstRecord* trace_data,
                          ExecCallback callback_func);

    /**
     * Event-driven means for other classes to request timing-based "issue"
     * The "issue" stage of this processor prepares decoded instructions (or
     * micro-ops) for execution. This is where rename and dependency checking
     * occurs.
     * Also, the next instruction is not fetched until after this stage.
     *
     * @param context for the instruction to know if it has been squashed
     * @param callback_func to call when issuing the instruction
     */
    void requestIssue(std::function<void()> callback_func,
                      std::function<bool()> is_squashed);

    /**
     * Event-driven means for other classes to request a translation of a
     * particular address via this CPU's ITB. Upon completion of this
     * translation (immediate or not), this CPU implementation should return
     * the values retrieved from the ITB via the callback function, and whether
     * or not there was any fault as a result of the request.
     *
     * @param req A pointer to a gem5 Request object containing the virtual
     *  address to be translated
     * @param tc A pointer to the ThreadContext for the request, since the itb
     *  is specific to a thread
     * @param callback_func The function reference for purposes of handling the
     *  result of the translation
     * @param is_squashed is a function that returns true if this fetch has
     *  been squashed by the thread
     */
    void requestInstAddrTranslation(const RequestPtr& req, ThreadContext* tc,
                                    TranslationCallback callback_func,
                                    std::function<bool()> is_squashed);

    /**
     * Event-driven means for other classes to request a single MachInst via
     * this CPU's instruction Port.
     *
     * @param req A pointer to a gem5 Request object which should have a valid
     *  physical memory address to send through the Port
     * @param callback_func The function reference for purposes of handling the
     *  result of the memory request
     * @param is_squashed is a function that returns true if this fetch has
     *  been squashed by the thread
     */
    void requestInstructionData(const RequestPtr& req,
                                FetchCallback callback_func,
                                std::function<bool()> is_squashed);

    /**
     * Event-driven means for classes to request a read access to memory. Upon
     * the response event from the memory port, the result of completeAcc()
     * will be passed to the callback function.
     *
     * @param req A memory request object, which should contain a completed
     *  translation and therefore ready to send to memory.
     * @param tc The ThreadContext making the request. May be necessary in the
     *  case of special types of reads, like LLSC.
     * @param inst The StaticInst reference on which we will call completeAcc()
     *  when the read completes.
     * @param context The ExecContext we will pass to completeAcc().
     * @param callback_func The function reference for purposes of handling the
     *  result of the memory read.
     *
     * @return Whether or not this request has been acknowledged. If this
     *  function returns false, the requester should not expect callback_func
     *  to be called in the future.
     */
    bool requestMemRead(const RequestPtr& req, ThreadContext* tc,
                        StaticInstPtr inst, std::weak_ptr<ExecContext> context,
                        Trace::InstRecord* trace_data,
                        MemCallback callback_func);

    /**
     * Event-driven means for classes to request a write access to memory. Upon
     * the response event from the memory port, the result of completeAcc()
     * will be passed to the callback function.
     *
     * @param req A memory request object, which should contain a completed
     *  translation and therefore ready to send to memory.
     * @param tc The ThreadContext making the request. May be necessary in the
     *  case of special types of reads, like LLSC.
     * @param inst The StaticInst reference on which we will call completeAcc()
     *  when the read completes.
     * @param context The ExecContext we will pass to completeAcc().
     * @param data A pointer to a location in memory holding the data we need
     *  written to memory. The data pointer only needs to be valid for the
     *  duration of this function call.
     * @param callback_func The function reference for purposes of handling the
     *  result of the memory write.
     *
     * @return Whether or not this request has been acknowledged. If this
     *  function returns false, the requester should not expect callback_func
     *  to be called in the future.
     */
    bool requestMemWrite(const RequestPtr& req, ThreadContext* tc,
                         StaticInstPtr inst,
                         std::weak_ptr<ExecContext> context,
                         Trace::InstRecord* trace_data, uint8_t* data,
                         MemCallback callback_func);

    /**
     * Event-driven means for classes to request a split read access to memory.
     * Upon the response event from the memory port, the result of
     * completeAcc() will be passed to the callback function.
     *
     * @param main A memory request object, which will be used to construct the
     *  final packet used to complete the access through the StaticInst
     *  interface.
     * @param low A memory request object which should contain a completed
     *  translation and therefore ready to send to memory.
     * @param high A memory request object which should contain a completed
     *  translation and therefore ready to send to memory.
     * @param tc The ThreadContext making the request. May be necessary in the
     *  case of special types of reads, like LLSC.
     * @param inst The StaticInst reference on which we will call completeAcc()
     *  when the read completes.
     * @param context The ExecContext we will pass to completeAcc().
     * @param callback_func The function reference for purposes of handling the
     *  result of the memory read.
     *
     * @return Whether or not this request has been acknowledged. If this
     *  function returns false, the requester should not expect callback_func
     *  to be called in the future.
     */
    bool requestSplitMemRead(const RequestPtr& main, const RequestPtr& low,
                             const RequestPtr& high, ThreadContext* tc,
                             StaticInstPtr inst,
                             std::weak_ptr<ExecContext> context,
                             Trace::InstRecord* trace_data,
                             MemCallback callback_func);

    /**
     * Event-driven means for classes to request a split write access to
     * memory. Upon the response event from the memory port, the result of
     * completeAcc() will be passed to the callback function.
     *
     * @param main A memory request object, which will be used to construct the
     *  final packet used to complete the access through the StaticInst
     *  interface.
     * @param low A memory request object which should contain a completed
     *  translation and therefore ready to send to memory.
     * @param high A memory request object which should contain a completed
     *  translation and therefore ready to send to memory.
     * @param tc The ThreadContext making the request. May be necessary in the
     *  case of special types of reads, like LLSC.
     * @param inst The StaticInst reference on which we will call completeAcc()
     *  when the read completes.
     * @param context The ExecContext we will pass to completeAcc().
     * @param data A pointer to a location in memory holding the data we need
     *  written to memory. The data pointer only needs to be valid for the
     *  duration of this function call.
     * @param callback_func The function reference for purposes of handling the
     *  result of the memory write.
     *
     * @return Whether or not this request has been acknowledged. If this
     *  function returns false, the requester should not expect callback_func
     *  to be called in the future.
     */
    bool requestSplitMemWrite(const RequestPtr& main, const RequestPtr& low,
                              const RequestPtr& high, ThreadContext* tc,
                              StaticInstPtr inst,
                              std::weak_ptr<ExecContext> context,
                              Trace::InstRecord* trace_data,
                              uint8_t* data, MemCallback callback_func);

    /**
     * Called after init, during the first simulate() event.
     */
    void startup() override;

    /**
     * Called before this CPU instance is swapped with another. Must ensure
     * that by the time this function returns, internal state is flushed.
     */
    void switchOut() override;

    /**
     * Called before a running CPU instance is swapped for this. We are
     * expected to initialize our state using the old CPU's state, and take
     * over any connections (ports) the previous CPU held.
     *
     * @param cpu CPU to load state from.
     */
    void takeOverFrom(BaseCPU* cpu) override;

    /**
     * Global counters returning counts of instructions or ops committed so
     * far.
     */
    Counter totalInsts() const override;
    Counter totalOps() const override;

    /**
     * Called to resume execution of a particular thread on this CPU.
     */
    void wakeup(ThreadID tid) override;

    void regStats() override;

    // END Member functions
};


#endif // __CPU_FLEXCPU_FLEXCPU_HH__
