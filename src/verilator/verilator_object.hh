
#ifndef __VERILATOR_VERILATOR_OBJECT_HHH__
#define __VERILATOR_VERILATOR_OBJECT_HHH__

#include <fesvr/dtm.h>

#include "ITop.hh"
#include "params/VerilatorObject.hh"
#include "sim/sim_object.hh"

class VerilatorObject : public ITop, public SimObject
{
    private:
        void processEvent();

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

    public:
        VerilatorObject(VerilatorObjectParams *p);
        ~VerilatorObject();
        void reset(int);
        void startup();
};
#endif
