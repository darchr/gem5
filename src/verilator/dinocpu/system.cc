#include "params/DinoCPUSystem.hh"
#include "system.hh"

DinoCPUSystem *
DinoCPUSystemParams::create()
{
  return new DinoCPUSystem(this);
}
