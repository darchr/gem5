
#ifndef __VERILATOR_VERILATOR_OBJECT_HH__
#define __VERILATOR_VERILATOR_OBJECT_HH__

//#include <fesvr/dtm.h>

#include "ITop.hh"
#include "mem/mem_object.hh"
#include "params/VerilatorObject.hh"
#include "sim/sim_object.hh"

class VerilatorObject : public ITop, public MemObject
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
        EventFunctionWrapper event;
        int maxCycles;
        Tick latency;
        int designStages;
        int start;
        const char * loadMem;
        std::string const objName;
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

                void recvRangeChange() override;
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
