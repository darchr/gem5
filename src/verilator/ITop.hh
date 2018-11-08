#ifndef __VERILATOR_ITOP__HH__
#define __VERILATOR_ITOP__HH__

#include <cstring>

#include "include/VTop.h"
#include "include/VTop__Dpi.h"
#include "include/verilated.h"
#include "include/verilator.h"
#include "sim/sim_object.hh"

class ITop
{
        protected:
                virtual void reset(int) = 0;

                VTop dut;

        public:
                virtual void startup() = 0;
};
#endif
