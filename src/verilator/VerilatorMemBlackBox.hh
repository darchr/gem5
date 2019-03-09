#ifndef __VERILATOR_VERILATOR_MEM_BLACK_BOX__HH__
#define __VERILATOR_VERILATOR_MEM_BLACK_BOX__HH__

#define VM_TRACE 0
#define VL_THREADED 0

#include "VDualPortedMemoryBlackBox.h"
#include "mem/mem_object.hh"
#include "params/VerilatorMemBlackBox.hh"

class VerilatorMemBlackBox: public MemObject
{
    public:
        DualPortedMemoryBlackBox blkbox;

        void doFetch();
        void doMem();

        class VerilatorMemBlackBoxPort : public MasterPort
        {
            private:
                VerilatorMemBlackBox *owner;

                PacketPtr blockedPacket;

            public:
                VerilatorMemBlackBoxPort(const std::string& name,
                    VerilatorMemBlackBox *owner) :
                    MasterPort(name, owner),
                    owner(owner),
                    blockedPacket(nullptr)
                { }

                void sendPacket(PacketPtr pkt);

            protected:
                bool recvTimingResp(PacketPtr pkt) override;

                void recvReqRetry() override;
        };

        VerilatorMemBlackBoxPort instPort;
        VerilatorMemBlackBoxPort dataPort;

    private:
        VerilatorMemBlackBox( VerilatorMemBlackBoxParams *p );
        ~VerilatorMemBlackBox();
        bool handleResponse( PacketPtr pkt );
        BaseMasterPort& getMasterPort( const std::string& if_name,
                PortID idx );


};
#endif
