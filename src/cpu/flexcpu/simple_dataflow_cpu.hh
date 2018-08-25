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

#ifndef __CPU_FLEXCPU_SIMPLE_DATAFLOW_CPU_HH__
#define __CPU_FLEXCPU_SIMPLE_DATAFLOW_CPU_HH__

#include <functional>
#include <list>
#include <memory>
#include <vector>

#include "cpu/base.hh"
#include "cpu/flexcpu/simple_dataflow_thread.hh"
#include "mem/packet.hh"
#include "params/SimpleDataflowCPU.hh"

class SDCPUThread;
/**
 * The CPU class represents a core, which contains hardware capable of running
 * one or more threads. This class has the responsibility of managing hardware
 * structures and controlling the timing of events.
 */
class SimpleDataflowCPU : public BaseCPU
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
        SimpleDataflowCPU* cpu;
      public:
        bool receivedPushback = false;

        DataPort(const std::string& name_, SimpleDataflowCPU* cpu_):
            MasterPort(name_, cpu_),
            cpu(cpu_)
        { }

        virtual ~DataPort()
        { }

        bool recvTimingResp(PacketPtr pkt);

        void recvReqRetry();
    };

    class InstPort : public MasterPort
    {
      protected:
        SimpleDataflowCPU* cpu;
      public:
        bool receivedPushback = false;

        InstPort(const std::string& name_, SimpleDataflowCPU* cpu_):
            MasterPort(name_, cpu_),
            cpu(cpu_)
        { }

        virtual ~InstPort()
        { }

        bool recvTimingResp(PacketPtr pkt);

        void recvReqRetry();
    };

    class CallbackTransHandler : public BaseTLB::Translation
    {
      protected:
        TranslationCallback callback;
        bool selfDestruct;
      public:
        CallbackTransHandler(TranslationCallback callback,
                         bool self_destruct = false):
            callback(callback),
            selfDestruct(self_destruct)
        { }

        void
        markDelayed() override
        { }

        void finish(const Fault& fault, const RequestPtr& req,
                    ThreadContext* tc, BaseTLB::Mode mode) override
        {
            callback(fault, req);

            // It requires assumptions that users of this will be well-behaved
            // to call "delete this;" in a C++ class, but since this is an
            // internal class, we should be able to contain access to this line
            if (selfDestruct) delete this;
        }
    };

    // END Internal class definitions

    // BEGIN Internal container structs

    struct TranslationReq
    {
        RequestPtr request;
        TranslationCallback callback;
    };

    struct FetchReq
    {
        PacketPtr packet;
        FetchCallback callback;
    };

    struct ExecutionReq
    {
        StaticInstPtr staticInst;
        std::weak_ptr<ExecContext> execContext;
        Trace::InstRecord* traceData;
        ExecCallback callback;
    };

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

    struct MemAccessReq
    {
        PacketPtr packet;
        StaticInstPtr staticInst;
        std::weak_ptr<ExecContext> execContext;
        Trace::InstRecord* traceData;
        ThreadContext* tc;
        MemCallback callback;

        SplitAccCtrlBlk* split;
    };

    // END Internal container structs


    // BEGIN Internal constants

    // END Internal constants


    // BEGIN Internal parameters

    // TODO temporary mark all executions as taking one cycle's length at least
    Tick executionTime = clockPeriod();

    // END Internal parameters


    // BEGIN Internal state variables

    DataPort _dataPort;
    InstPort _instPort;

    // Using unique_ptr for now since holding objects directly results
    // in calls to the object's copy constructor, which SimpleThread does not
    // like.
    std::vector<std::unique_ptr<SDCPUThread>> threads;

    // For use in enqueuing requests if we want to limit number of requests
    // servicable at once in the future. TODO for now.
    std::list<TranslationReq> dataTranslationReqs;
    std::list<TranslationReq> instTranslationReqs;
    std::list<FetchReq> fetchReqs;
    std::list<ExecutionReq> executionReqs;
    std::list<MemAccessReq> memReqs;

    // Note: I was informed that the pointer you get back is the same pointer
    //       you send through the port, so it's safe to do this mapping. It may
    //       be safer to do this mapping through the SenderState struct instead
    //       based on what has been learned since the writing of this though.
    std::unordered_map<PacketPtr, FetchCallback> outstandingFetches;
    std::unordered_map<PacketPtr, MemAccessReq> outstandingMemReqs;

    // END Internal state variables


    // BEGIN Internal functions

    void attemptAllFetchReqs();

    void attemptAllMemReqs();

    void beginExecution(const ExecutionReq& req);

    void completeExecution(const ExecutionReq& req);

    void completeMemAccess(const MemAccessReq& req);

    // TODO some form of speculative state flush function?

    // END Internal functions


  public:
    /**
     * Constructor.
     */
    SimpleDataflowCPU(SimpleDataflowCPUParams* params);

    /**
     * Virtual destructor explicitly declared, so that this class can be
     * subclassed.
     */
    virtual ~SimpleDataflowCPU();

    // BEGIN Member functions (sorted alphabetically)

    /**
     * Called to set up the state to run a particular thread context. This also
     * kicks off the first simulation
     *
     * @param tid The ID of the thread that was activated
     */
    void activateContext(ThreadID tid) override;

    /**
     * Interface calls to retrieve a reference to the port used for data or
     * instructions respectively.
     */
    MasterPort& getDataPort() override;
    MasterPort& getInstPort() override;

    /**
     * Called after constructor but before running. (Make sure to call
     * BaseCPU::init() in this function to let it do its job)
     */
    void init() override;

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
     *
     * @return Whether or not this request has been acknowledged. If this
     *  function returns false, the requester should not expect callback_func
     *  to be called in the future.
     */
    bool requestDataAddrTranslation(const RequestPtr& req, ThreadContext* tc,
                                    bool write,
                                    TranslationCallback callback_func);

    // TODO maybe we just need a generic requestInstSquash function to squash
    //      anything we don't want to execute anymore. Might be useful to free
    //      up execution units, even if we already can just choose not to
    //      commit an instruction.
    bool requestMemSquash(); // TODO

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
     *
     * @return Whether or not this request has been acknowledged. If this
     *  function returns false, the requester should not expect callback_func
     *  to be called in the future.
     */
    bool requestExecution(StaticInstPtr inst,
                          std::weak_ptr<ExecContext> context,
                          Trace::InstRecord* trace_data,
                          ExecCallback callback_func);

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
     *
     * @return Whether or not this request has been acknowledged. If this
     *  function returns false, the requester should not expect callback_func
     *  to be called in the future.
     */
    bool requestInstAddrTranslation(const RequestPtr& req, ThreadContext* tc,
                                    TranslationCallback callback_func);

    /**
     * Event-driven means for other classes to request a single MachInst via
     * this CPU's instruction Port.
     *
     * @param req A pointer to a gem5 Request object which should have a valid
     *  physical memory address to send through the Port
     * @param callback_func The function reference for purposes of handling the
     *  result of the memory request
     *
     * @return Whether or not this request has been acknowledged. If this
     *  function returns false, the requester should not expect callback_func
     *  to be called in the future.
     */
    bool requestInstruction(const RequestPtr& req,
                            FetchCallback callback_func);

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

    // END Member functions
};


#endif // __CPU_FLEXCPU_SIMPLE_DATAFLOW_CPU_HH__
