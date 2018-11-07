#ifndef __VERILATOR_ITOP__HH__
#define __VERILATOR_ITOP__HH__

#include <cstring>

#include "VTop.h"
#include "VTop__Dpi.h"
#include "sim/sim_object.hh"
#include "verilated.h"
#include "verilator.h"

class ITop
{
        protected:
                virtual void reset(int) = 0;

                VTop dut;

        public:
                virtual void startup() = 0;
};
#endif
