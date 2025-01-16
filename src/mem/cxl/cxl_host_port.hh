#ifndef __MEM_CXL_CXL_HOST_PORT_HH__
#define __MEM_CXL_CXL_HOST_PORT_HH__

#include "mem/packet.hh"
#include "mem/ruby/network/MessageBuffer.hh"
#include "mem/ruby/slicc_interface/AbstractController.hh"
#include "sim/sim_object.hh"

// Generated from SLICC


class CXLHostPort: public SimObject
{
  public:
    PARAMS(CXLHostPort);
    CXLHostPort(const Params &p);

    void init() override;

    // API to send/receive CXL requests
    // This looks a lot like a generic mem side port
    bool initiateMemoryRequest(PacketPtr pkt);
    void setInvalidationCallback(std::function<void(Addr)> callback);
    bool initiateInvalidationAck(Addr addr);

  private:
    AbstractController *rubyController = nullptr;
    MessageBuffer *mandatoryQueue = nullptr;

    std::function<void(Addr)> invalidationCallback;

};

#endif //__MEM_CXL_CXL_HOST_PORT_HH__
