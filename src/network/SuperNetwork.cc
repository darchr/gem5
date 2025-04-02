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

#include "network/SuperNetwork.hh"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>

#include "debug/SuperNetwork.hh"
#include "network/Layer.hh"
#include "network/NetworkScheduler.hh"
#include "sim/eventq.hh"
#include "sim/sim_exit.hh"
#include "sim/stats.hh"
#include "sim/system.hh"

namespace gem5
{
    SuperNetwork::SuperNetwork(const SuperNetworkParams& params)
        : ClockedObject(params),
          crosspointDelay(params.crosspoint_delay),
          mergerDelay(params.merger_delay),
          splitterDelay(params.splitter_delay),
          circuitVariability(params.circuit_variability),
          variabilityCountingNetwork(params.variability_counting_network),
          crosspointSetupTime(params.crosspoint_setup_time)
    {
        assert(params.crosspoint_delay >= 0);
        assert(params.merger_delay >= 0);
        assert(params.splitter_delay >= 0);
        assert(params.circuit_variability >= 0);
        assert(params.variability_counting_network >= 0);
        assert(params.crosspoint_setup_time >= 0);

        // Initialize the network layers
        // for (int i = 0; i < params.layers.size(); i++) {
        //     Layer* layer = new Layer(params.layers[i]);
        //     layers.push_back(layer);
        // }

        for (auto layer : params.layers) {
            layer->setTimeSlot(calculateTimeSlot(layer->getRadix()));
            DPRINTF(SuperNetwork, "Layer %d: time slot = %d\n",
                layer->getRadix(), layer->getTimeSlot()
            );
            layer->setConnectionWindow(Cycles(layer->getDynamicRange() *
                                            timeSlot));
            DPRINTF(SuperNetwork, "Layer %d: connection window = %d\n",
                layer->getRadix(), layer->getConnectionWindow()
            );
            layer->scheduleNextNetworkEvent(curTick());
        }
    }

    Cycles
    SuperNetwork::calculateTimeSlot(uint64_t radix)
    {
        // Adjust setup time considering circuit variability
        double SE_adjusted = std::max(0.0,
            crosspointSetupTime - circuitVariability
        );

        // Calculate time slot considering delays of various network components
        double calculatedTimeSlot = circuitVariability * (
            crosspointDelay + SE_adjusted +
            splitterDelay * (radix - 1) +
            mergerDelay * (radix - 1) +
            variabilityCountingNetwork
        );

        // Round up the calculated time slot
        this->timeSlot = Cycles(std::ceil(calculatedTimeSlot));
        return this->timeSlot;

    }

} // namespace gem5
