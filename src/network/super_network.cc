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

#include "network/super_network.hh"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>

#include "debug/SuperNetwork.hh"
#include "network/layer.hh"
#include "network/network_scheduler.hh"
#include "sim/eventq.hh"
#include "sim/sim_exit.hh"
#include "sim/stats.hh"
#include "sim/system.hh"

namespace gem5
{
    SuperNetwork::SuperNetwork(const SuperNetworkParams& params)
        : ClockedObject(params),
          numLayers(params.layers.size()),
          finishedLayers(0)
    {
        int layer_index = 0;
        for (auto layer : params.layers) {
            layer->computeTimingParameters();
            DPRINTF(SuperNetwork, "Layer %d: time slot = %d\n",
                layer_index, layer->getTimeSlot()
            );
            DPRINTF(SuperNetwork, "Layer %d: connection window = %d\n",
                layer_index, layer->getConnectionWindow()
            );
            layer->scheduleNextNetworkEvent(curTick());
            layer->registerSuperNetwork(this);
            managedLayers.push_back(layer);
            layer_index++;
        }
    }

    void
    SuperNetwork::notifyLayerFinished(Layer* layer)
    {

        DPRINTF(SuperNetwork, "Received finish notification from Layer");

        // Increment the counter
        finishedLayers++;

        DPRINTF(SuperNetwork,
            "Finished layers: %d / %d\n",
            finishedLayers, numLayers
        );

        // Check if all layers are now finished
        checkCompletionAndExit();
    }

    void
    SuperNetwork::checkCompletionAndExit()
    {
        if (finishedLayers < 0 || finishedLayers > numLayers) {
            panic("SuperNetwork finishedLayers count (%d) \
                    is out of bounds [0, %d]!",
                    finishedLayers, numLayers
            );
        }

        if (finishedLayers == numLayers) {
            DPRINTF(SuperNetwork,
                "All %d layers have finished processing.\n",
                numLayers
            );
            // Exit the simulation loop
            exitSimLoop("SuperNetwork: \
                All layers finished processing packets."
            );
        }
    }

} // namespace gem5
