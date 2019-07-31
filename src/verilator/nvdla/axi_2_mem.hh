/* nvdla.cpp
 * Driver for Verilator testbench
 * NVDLA Open Source Project
 *
 * Copyright (c) 2017 NVIDIA Corporation.  Licensed under the NVDLA Open
 * Hardware License.  For more information, see the "LICENSE" file that came
 * with this distribution.
 */

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

#ifndef __VERILATOR_AXI_2_MEM__HH__
#define __VERILATOR_AXI_2_MEM__HH__

#include <fcntl.h>
#include <stdint.h>

#include <cstdio>
#include <cstdlib>
#include <queue>

template <typename ADDRTYPE>
class AXIResponder {
public:
        struct connections {
                uint8_t *aw_awvalid;
                uint8_t *aw_awready;
                uint8_t *aw_awid;
                uint8_t *aw_awlen;
                ADDRTYPE *aw_awaddr;

                uint8_t *w_wvalid;
                uint8_t *w_wready;
                uint32_t *w_wdata;
                uint64_t *w_wstrb;
                uint8_t *w_wlast;

                uint8_t *b_bvalid;
                uint8_t *b_bready;
                uint8_t *b_bid;

                uint8_t *ar_arvalid;
                uint8_t *ar_arready;
                uint8_t *ar_arid;
                uint8_t *ar_arlen;
                ADDRTYPE *ar_araddr;

                uint8_t *r_rvalid;
                uint8_t *r_rready;
                uint8_t *r_rid;
                uint8_t *r_rlast;
                uint32_t *r_rdata;
        };

private:

#define AXI_BLOCK_SIZE 4096
#define AXI_WIDTH 512

        //read request
        struct axi_r_txn {
                int rvalid;
                int rlast;
                uint32_t rdata[AXI_WIDTH / 32];
                uint8_t rid;
        };
        //read request buffer
        std::queue<axi_r_txn> r_fifo;
        //read response buffer
        std::queue<axi_r_txn> r0_fifo;

        //write request
        struct axi_aw_txn {
                uint8_t awid;
                uint32_t awaddr;
                uint8_t awlen;
        };
        //write request buffer
        std::queue<axi_aw_txn> aw_fifo;

        //write data
        struct axi_w_txn {
                uint32_t wdata[AXI_WIDTH / 32];
                uint64_t wstrb;
                uint8_t wlast;
        };
        //write data buffer
        std::queue<axi_w_txn> w_fifo;

        //write response
        struct axi_b_txn {
                uint8_t bid;
        };
        //write response buffer
        std::queue<axi_b_txn> b_fifo;

        struct connections dla;
        const char *name;

public:
        AXIResponder(struct connections _dla, const char *_name);

        uint8_t read(uint32_t addr);

        void write(uint32_t addr, uint8_t data);

        void eval();
};
#endif
