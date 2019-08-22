
/*# Copyright (c) 2019 The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Nima Ganjehloo
*/

//generic includes
//#define NDEBUG
#include <cassert>

//gem5 includes
#include "base/logging.hh"
#include "debug/Verilator.hh"
#include "sim/sim_exit.hh"

//gem5 model includes
#include "driven_object.hh"

//creates object for gem5 to use
DrivenObject*
DrivenObjectParams::create()
{
  //verilator has weird alignment issue for generated code
  void* ptr = aligned_alloc(128, sizeof(DrivenObject));
  return new(ptr) DrivenObject(this);

}

void
DrivenObject::updateCycle()
{
  DPRINTF(Verilator, "\n\nCLOCKING DEVICE\n");
  //clock the device
  driver.clockDevice();

  DPRINTF(Verilator, "\n\nSCHEDULE NEXT CYCLE\n");
  if (!driver.isFinished())
    schedule(event, nextCycle());
}


void
DrivenObject::startup()
{
  DPRINTF(Verilator, "STARTING UP DINOCPU\n");

  DPRINTF(Verilator, "RESETING FOR %d CYCLES\n", resetCycles);
  driver.reset(resetCycles);
  DPRINTF(Verilator, "DONE RESETING\n");

  //lets fetch an instruction before doing anything
  DPRINTF(Verilator, "SCHEDULING FIRST TICK \n");
  schedule(event, nextCycle());
}
