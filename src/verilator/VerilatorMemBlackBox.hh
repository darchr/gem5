#ifndef __VERILATOR_VERILATOR_MEM_BLACK_BOX__HH__
#define __VERILATOR_VERILATOR_MEM_BLACK_BOX__HH__

#include "mem/mem_object.hh"
#include "params/VerilatorMemBlackBox.hh"

class DualPortedMemoryBlackBox: public MemObject
{
    public:
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
