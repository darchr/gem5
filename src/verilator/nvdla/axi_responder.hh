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

#ifndef __VERILATOR_AXI_RESPONDER__HH__
#define __VERILATOR_AXI_RESPONDER__HH__

#include <fcntl.h>
#include <stdint.h>

#include <cstdio>
#include <cstdlib>
#include <queue>

#include "axi_to_mem.hh"

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

        const static int AXI_R_LATENCY = 32;
        const static int AXI_R_DELAY = 0;

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
        const char *conn_name;
        AXIToMem * axi2gem;

public:
        AXIResponder(struct connections _dla,
        const char *_name,
                AXIToMem * mem)
        {
                axi2gem = mem;
                dla = _dla;

                *dla.aw_awready = 1;
                *dla.w_wready = 1;
                *dla.b_bvalid = 0;
                *dla.ar_arready = 1;
                *dla.r_rvalid = 0;

                conn_name = _name;

                //WE SHOULDNT NEED THIS  r0 is initial steady state fifo
                /* add some latency... */
                for (int i = 0; i < AXI_R_LATENCY; i++)
                {
                        axi_r_txn txn;

                        txn.rvalid = 0;
                        txn.rvalid = 0;
                        txn.rid = 0;
                        txn.rlast = 0;
                        for (int i = 0; i < AXI_WIDTH / 32; i++)
                        {
                                txn.rdata[i] = 0xAAAAAAAA;
                        }

                        r0_fifo.push(txn);
                }
        }

        uint8_t read(uint32_t addr){
                //fetch a blocks worth of data from gem5 memory model
                axi2gem->doMem(addr, 0, 0);
                return axi2gem->dmemResp;
        }

        void write(uint32_t addr, uint8_t data){
                //write data to gem5 memory model
                axi2gem->doMem(addr, 1, data);
        }

        void eval(){
                /* write request */
                if (*dla.aw_awvalid && *dla.aw_awready)
                {
                printf("%s: write request from dla, \
                addr %08lx id %d\n", conn_name, *dla.aw_awaddr,
                *dla.aw_awid);

                        axi_aw_txn txn;

                        txn.awid = *dla.aw_awid;
                        txn.awaddr = *dla.aw_awaddr &
                                ~(ADDRTYPE)(AXI_WIDTH / 8 - 1);
                        txn.awlen = *dla.aw_awlen;
                        aw_fifo.push(txn);

                        *dla.aw_awready = 0;
                }
                else
                {
                        *dla.aw_awready = 1;
                }

                /* write data */
                if (*dla.w_wvalid)
                {
                printf("%s: write data from dla (%08x %08x...)\n",
                conn_name, dla.w_wdata[0], dla.w_wdata[1]);

                        axi_w_txn txn;

                        for (int i = 0; i < AXI_WIDTH / 32; i++)
                        txn.wdata[i] = dla.w_wdata[i];
                        txn.wstrb = *dla.w_wstrb;
                        txn.wlast = *dla.w_wlast;
                        w_fifo.push(txn);
                }

                /* read request */
                if (*dla.ar_arvalid && *dla.ar_arready)
                {
                        ADDRTYPE addr = *dla.ar_araddr
                                & ~(ADDRTYPE)(AXI_WIDTH / 8 - 1);
                        uint8_t len = *dla.ar_arlen;

                printf("%s:read request from dla, addr %08lx burst %d id %d\n",
                        conn_name, *dla.ar_araddr,
                        *dla.ar_arlen, *dla.ar_arid);

                        do
                        {
                        axi_r_txn txn;

                        txn.rvalid = 1;
                        txn.rlast = len == 0;
                        txn.rid = *dla.ar_arid;

                        for (int i = 0; i < AXI_WIDTH / 32; i++)
                        {
                                uint32_t da = read(addr + i * 4) +
                                        (read(addr + i * 4 + 1) << 8) +
                                        (read(addr + i * 4 + 2) << 16) +
                                        (read(addr + i * 4 + 3) << 24);
                                txn.rdata[i] = da;
                        }

                        r_fifo.push(txn);

                        addr += AXI_WIDTH / 8;
                        } while (len--);

                        // We shouldn't need this delay code
                        axi_r_txn txn;

                        txn.rvalid = 0;
                        txn.rid = 0;
                        txn.rlast = 0;
                        for (int i = 0; i < AXI_WIDTH / 32; i++)
                        {
                        txn.rdata[i] = 0x55555555;
                        }

                        for (int i = 0; i < AXI_R_DELAY; i++)
                                r_fifo.push(txn);

                        *dla.ar_arready = 0;
                }
                else
                        *dla.ar_arready = 1;

                /* now handle the write FIFOs ... */
                if (!aw_fifo.empty() && !w_fifo.empty())
                {
                        axi_aw_txn &awtxn = aw_fifo.front();
                        axi_w_txn &wtxn = w_fifo.front();

                        if (wtxn.wlast != (awtxn.awlen == 0))
                        {
                        fatal("%s: wlast / awlen mismatch\n", conn_name);
                        }

                        for (int i = 0; i < AXI_WIDTH / 8; i++)
                        {
                        if (!((wtxn.wstrb >> i) & 1))
                                continue;

                        write(awtxn.awaddr + i,
                                (wtxn.wdata[i / 4] >> ((i % 4) * 8)) & 0xFF);
                        }

                        if (wtxn.wlast)
                        {
                        printf("%s: write, last tick\n", conn_name);
                        aw_fifo.pop();

                        axi_b_txn btxn;
                        btxn.bid = awtxn.awid;
                        b_fifo.push(btxn);
                        }
                        else
                        {
                        printf("%s: write, ticks remaining\n", conn_name);

                        awtxn.awlen--;
                        awtxn.awaddr += AXI_WIDTH / 8;
                        }

                        w_fifo.pop();
                }

                /* read response */
                if (!r_fifo.empty())
                {
                        axi_r_txn &txn = r_fifo.front();

                        r0_fifo.push(txn);
                        r_fifo.pop();
                }
                else
                {
                        axi_r_txn txn;

                        txn.rvalid = 0;
                        txn.rid = 0;
                        txn.rlast = 0;
                        for (int i = 0; i < AXI_WIDTH / 32; i++)
                        {
                        txn.rdata[i] = 0xAAAAAAAA;
                        }

                        r0_fifo.push(txn);
                }

                *dla.r_rvalid = 0;
                if (*dla.r_rready && !r0_fifo.empty())
                {
                        axi_r_txn &txn = r0_fifo.front();

                        *dla.r_rvalid = txn.rvalid;
                        *dla.r_rid = txn.rid;
                        *dla.r_rlast = txn.rlast;
                        for (int i = 0; i < AXI_WIDTH / 32; i++)
                        {
                        dla.r_rdata[i] = txn.rdata[i];
                        }

                        if (txn.rvalid)
                        printf("%s: read push: id %d,da %08x %08x %08x %08x\n",
                                conn_name, txn.rid, txn.rdata[0], txn.rdata[1],
                                txn.rdata[2], txn.rdata[3]);

                        r0_fifo.pop();
                }

                /* write response */
                *dla.b_bvalid = 0;
                if (*dla.b_bready && !b_fifo.empty())
                {
                        *dla.b_bvalid = 1;

                        axi_b_txn &txn = b_fifo.front();
                        *dla.b_bid = txn.bid;
                        b_fifo.pop();
                }
        }
};
#endif
