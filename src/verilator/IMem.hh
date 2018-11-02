#ifndef __VERILATOR_IMEM__HH__
#define __VERILATOR_IMEM__HH__

#include "mem/mem_object.hh"

//verilator file should be here

class IMem
{
        protected:
                virtual void* fetch(void* address) = 0;
                virtual void* store(void* address, void* value) = 0;


};
#endif

