/*
 * Copyright (c) 2025 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __NETWORK_SUPERNETWORK_HH__
#define __NETWORK_SUPERNETWORK_HH__

#include "network/buffered_port.hh"
#include "network/layer.hh"
#include "network/network_scheduler.hh"
#include "params/SuperNetwork.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"
#include "sim/sim_exit.hh"
#include "sim/stats.hh"

namespace gem5
{

class SuperNetwork : public ClockedObject
{
  private:
    std::vector<Layer*> layers;

    Cycles timeSlot;

    std::vector<Layer*> managedLayers; // Store pointers to layers
    const int numLayers; // Total number of layers managed
    int finishedLayers;  // Counter for finished layers

  public:
    // Constructor: Initializes the SuperNetwork with given parameters
    SuperNetwork(const SuperNetworkParams& params);

    // Function for layers to notify when they finish processing
    void notifyLayerFinished(Layer* layer);

    // Function to check if all layers have finished processing
    // and exit the simulation if they have
    void checkCompletionAndExit();
};

} // namespace gem5

#endif // __NETWORK_SUPERNETWORK_HH__
