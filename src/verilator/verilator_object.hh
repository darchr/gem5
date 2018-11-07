
#ifndef __VERILATOR_VERILATOR_OBJECT_HHH__
#define __VERILATOR_VERILATOR_OBJECT_HHH__

#include <fesvr/dtm.h>

#include "ITop.hh"
#include "params/VerilatorObject.hh"
#include "sim/sim_object.hh"

class VerilatorCPUMemPort : public MasterPort
{
    private:
        VerilatorObject *owner;

        PacketPtr blockedPacket;

    public:
        MemSidePort(const std::string& name, SimpleMemobj *owner) :
            MasterPort(name, owner), owner(owner), blockedPacket(nullptr)
        { }

        void sendPacket(PacketPtr pkt);

    protected:
        bool recvTimingResp(PacketPtr pkt) override;

        void recvReqRetry() override;

        void recvRangeChange() override;
};


class VerilatorObject : public ITop, public SimObject
{
    private:
        void processEvent();
        void sendFetch(const RequestPtr &req ;
        void sendData(const RequestPtr &req, uint8_t *data, bool read);
        handleResponse(PacketPtr pkt);


        dtm_t * dtm;
        std::vector<std::string> toDtm;
        EventFunctionWrapper event;
        int maxCycles;
        Tick latency;
        int designStages;
        int start;
        const char * loadMem;
        std::string const objName;
        int cyclesPassed;

        VerilatorCPUMemPort instPort;
        VerilatorCPUMemPort dataPort;

        PacketPtr inst_pkt;
        PacketPtr data_pkt;

    public:
        VerilatorObject(VerilatorObjectParams *p);
        ~VerilatorObject();
        void reset(int);
        void startup();
};
#endif
