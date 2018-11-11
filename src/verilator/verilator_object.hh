
#ifndef __VERILATOR_VERILATOR_OBJECT_HH__
#define __VERILATOR_VERILATOR_OBJECT_HH__

//#include <fesvr/dtm.h>
#define VM_TRACE 0
#define VL_THREADED 0

#include "VTop.h"
#include "mem/mem_object.hh"
#include "params/VerilatorObject.hh"
#include "sim/sim_object.hh"

class VerilatorObject : public MemObject
{
    private:
        void updateCycle();

        void sendFetch(const RequestPtr &req);
        void sendData(const RequestPtr &req, uint8_t *data, bool read);
        void buildPayloadForWrite(RequestPtr &data_req, uint64_t data,
            uint8_t * &packeddata);

        PacketPtr buildPacket(const RequestPtr &req, bool read);

        //dtm_t * dtm;
        //std::vector<std::string> toDtm;
        VTop dut;
        EventFunctionWrapper event;
        Tick latency;
        int designStages;
        int cyclesPassed;

        class VerilatorCPUMemPort : public MasterPort
        {
            private:
                VerilatorObject *owner;

                PacketPtr blockedPacket;

            public:
                VerilatorCPUMemPort(const std::string& name,
                    VerilatorObject *owner) :
                    MasterPort(name, owner),
                    owner(owner),
                    blockedPacket(nullptr)
                { }

                void sendPacket(PacketPtr pkt);

            protected:
                bool recvTimingResp(PacketPtr pkt) override;

                void recvReqRetry() override;
        };

        VerilatorCPUMemPort instPort;
        VerilatorCPUMemPort dataPort;
        bool dataRequested = false;
        bool instRequested = false;

    public:
        VerilatorObject(VerilatorObjectParams *p);
        ~VerilatorObject();
        void reset(int resetCycles);
        void startup();
        bool handleResponse(PacketPtr pkt);
};
#endif
