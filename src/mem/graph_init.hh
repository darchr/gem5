#ifndef __MESSAGE_QUEUE_HH__
#define __MESSAGE_QUEUE_HH__

#include "params/GraphInit.hh"
#include "sim/clocked_object.hh"

#include "mem/packet.hh"
#include "mem/port.hh"
#include "mem/request.hh"

#include "sim/system.hh"
#include <cmath>
#include <vector>
#include <string>

#include "base/addr_range.hh"
#include "mem/packet_access.hh"
#include "base/statistics.hh"
#include "debug/GraphInit.hh"


// #include "mem/simple_mem.hh"

namespace gem5
{

class GraphInit : public ClockedObject
{

  // public:
  //   GraphInit(const GraphInitParams& params);
    
  private:
  
    class ReqPort : public RequestPort
    {
      private:
        GraphInit* owner;
        PacketPtr blockedPacket;
        PortID _id;

      public:
        ReqPort(const std::string& name, GraphInit* owner, PortID id):
          RequestPort(name), owner(owner), blockedPacket(nullptr), _id(id)
        {}
        void sendPacket(PacketPtr pkt);
        bool blocked() { return (blockedPacket != nullptr); }

      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvReqRetry();
    };

    ReqPort mapPort;
    Addr maxVertexAddr;
    std::string graphFile;

    // ProcessingMode mode;

    // memory::SimpleMemory* mirrorsMem;

    // std::vector<MPU*> mpuVector;
    // AddrRangeMap<MPU*> mpuAddrMap;

    // int currentSliceId;
    // int numTotalSlices;
    // // int verticesPerSlice;
    // int totalUpdatesLeft;

    // bool chooseBest;
    // int* numPendingUpdates;
    // uint32_t* bestPendingUpdate;
    // int chooseNextSlice();

    // EventFunctionWrapper nextSliceSwitchEvent;
    // void processNextSliceSwitchEvent();

    // struct ControllerStats : public statistics::Group
    // {
    //   ControllerStats(GraphInit& ctrl);

    //   void regStats() override;

    //   GraphInit& ctrl;

    //   statistics::Scalar numSwitches;
    //   statistics::Scalar switchedBytes;
    //   statistics::Scalar switchTicks;
    //   statistics::Formula switchSeconds;
    // };
    // ControllerStats stats;

  protected:
    virtual void recvMemRetry();
    virtual bool handleMemResp(PacketPtr pkt);
    // RequestPtr data_write_req;
    int requestorID = 100;

    RequestPtr req;// = std::make_shared<Request>//(addr, size, flags,
                                               //requestorID);

  public:
    // GraphWorkload* workload;

    // PARAMS(GraphInit);
    uint64_t EL_addr;// = 0x0000000080000000;//1 << 31;// 0x200000000;
    uint64_t VL_addr;// = 6442450944;

    GraphInit(const GraphInitParams& params);
    Port& getPort(const std::string& if_name,
                PortID idx = InvalidPortID) override;

    PacketPtr createELWritePacket(Addr addr, const uint8_t* data);
    PacketPtr createVLWritePacket(Addr addr, const uint8_t* data);
    PacketPtr create64WritePacket(Addr addr, const uint8_t* data);
    virtual void startup() override;

    virtual void recvFunctional(PacketPtr pkt);

    // void setAsyncMode() { mode = ProcessingMode::ASYNCHRONOUS; }
    // void setBSPMode() { mode = ProcessingMode::BULK_SYNCHRONOUS; }
    // void setPGMode() { mode = ProcessingMode::POLY_GRAPH; }

    // void createPopCountDirectory(int atoms_per_block);

    // void createBFSWorkload(Addr init_addr, uint32_t init_value);
    // void createBFSVisitedWorkload(Addr init_addr, uint32_t init_value);
    // void createSSSPWorkload(Addr init_addr, uint32_t init_value);
    // void createCCWorkload();
    // void createAsyncPRWorkload(float alpha, float threshold);
    // void createPRWorkload(int num_nodes, float alpha);
    // void createBCWorkload(Addr init_addr, uint32_t init_value);

    void recvDoneSignal();

    // int workCount();
    // float getPRError();
    // void printAnswerToHostSimout();
};

}

#endif