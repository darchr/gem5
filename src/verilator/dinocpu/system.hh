#ifndef __VERILATOR_DINOCPU_SYSTEM__HH__
#define __VERILATOR_DINOCPU_SYSTEM__HH__


#include "kern/system_events.hh"
#include "params/DinoCPUSystem.hh"
#include "sim/sim_object.hh"
#include "sim/system.hh"

class DinoCPUSystem : public System {
    public:
      typedef DinoCPUSystemParams Params;
      DinoCPUSystem(Params *p): System(p)
      {
      }
    protected:
      const Params *params() const { return (const Params *)_params;}

};

#endif
