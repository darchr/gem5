

#ifndef __ACCEL_DAXPY_HH__
#define __ACCEL_DAXPY_HH__

#include <map>

#include "arch/tlb.hh"
#include "cpu/thread_context.hh"
#include "cpu/translation.hh"
#include "dev/io_device.hh"
#include "mem/mport.hh"
#include "params/Daxpy.hh"
#include "params/DaxpyDriver.hh"
#include "sim/emul_driver.hh"
#include "sim/process.hh"
#include "sim/system.hh"

class Daxpy : public BasicPioDevice
{
  private:

    class MemoryPort : public QueuedMasterPort
    {
        ReqPacketQueue queue;
        SnoopRespPacketQueue snoopQueue;
      public:
        MemoryPort(const std::string &name, MemObject *owner) :
            QueuedMasterPort(name, owner, queue, snoopQueue),
            queue(*owner, *this), snoopQueue(*owner, *this)
        {}
      protected:
        bool recvTimingResp(PacketPtr pkt) override;
    };

    typedef struct {
        Addr X;
        Addr Y;
        double alpha;
        int N;
    } FuncParams;

    FuncParams daxpyParams;

    int completedIterations;

    class LoopIteration
    {
      private:
        int i;
        int step;
        double x;
        double y;
        int stage;
        FuncParams params;
        Daxpy *accel;
      public:
        // Note: each stage should happen 1 cycle after the response
        void stage1();
        void stage2();
        void stage3();
        void stage4();
        void recvResponse(PacketPtr pkt);
        std::string name() { return std::string("LoopIteration"); }
      private:
        EventWrapper<LoopIteration, &LoopIteration::stage2> runStage2;
        EventWrapper<LoopIteration, &LoopIteration::stage3> runStage3;
        EventWrapper<LoopIteration, &LoopIteration::stage4> runStage4;
        LoopIteration(int i, int step, FuncParams params, Daxpy* accel) :
            i(i), step(step), x(0), y(0), stage(0), params(params),
            accel(accel), runStage2(this), runStage3(this), runStage4(this)
            {stage1();}
      public:
        LoopIteration(int step, FuncParams params, Daxpy* accel);
    };

    MemoryPort memoryPort;

    Addr monitorAddr;
    Addr paramsAddr;

    ThreadContext *context;

    TheISA::TLB *tlb;

    int maxUnroll;

    typedef enum {
        Uninitialized,     // Nothing is ready
        Initialized,       // monitorAddr and paramsAddr are valid
        GettingParams,     // Have issued requests for params
        ExecutingLoop,     // Got all params, actually executing
        Returning          // Returning the result to the CPU thread
    } Status;

    Status status;

    int paramsLoaded;

    std::map<Addr, LoopIteration*> addressCallbacks;

  public:
    typedef DaxpyParams Params;
    Daxpy(const Params *p);

    /**
     *
     * @param if_name the port name
     * @param idx ignored index
     *
     * @return a reference to the port with the given name
     */
    BaseMasterPort &getMasterPort(const std::string &if_name,
                                  PortID idx = InvalidPortID);

    /**
    * Determine the address ranges that this device responds to.
    *
    * @return a list of non-overlapping address ranges
    */
    AddrRangeList getAddrRanges() const override;

    /** Called when a read command is recieved by the port.
     * @param pkt Packet describing this request
     * @return number of ticks it took to complete
     */
    virtual Tick read(PacketPtr pkt);

    /** Called when a write command is recieved by the port.
     * @param pkt Packet describing this request
     * @return number of ticks it took to complete
     */
    virtual Tick write(PacketPtr pkt);

    void runDaxpy();

    void open(ThreadContext *tc);

    void finishTranslation(WholeTranslationState *state);
    bool isSquashed() { return false; }

    void sendData(RequestPtr req, uint8_t *data, bool read);

    void accessMemoryCallback(Addr addr, int size, BaseTLB::Mode mode,
                              uint8_t *data, LoopIteration *iter);

  private:
    EventWrapper<Daxpy, &Daxpy::runDaxpy> runEvent;

    void accessMemory(Addr addr, int size, BaseTLB::Mode mode, uint8_t *data);

    void setAddressCallback(Addr addr, LoopIteration* iter);

    void loadParams();
    void recvParam(PacketPtr pkt);
    void executeLoop();
    void recvLoop(PacketPtr pkt);
    void loopIteration1(LoopIteration *iter);
    void loopIteration2(LoopIteration *iter);
    void loopIteration3(LoopIteration *iter);
    void loopIteration4(LoopIteration *iter);
    void sendFinish();
};


class DaxpyDriver : public EmulatedDriver
{
  private:
    Daxpy *hardware;

  public:
    typedef DaxpyDriverParams Params;
    DaxpyDriver(Params *p) : EmulatedDriver(p), hardware(p->hardware) {}

    int open(Process *p, ThreadContext *tc, int mode, int flags) {
        hardware->open(tc);
        return 1;
    }

    int ioctl(Process *p, ThreadContext *tc, unsigned req) {
        panic("Daxpy driver doesn't implement any ioctls");
        M5_DUMMY_RETURN
    }
};

#endif //__ACCEL_DAXPY_HH__
