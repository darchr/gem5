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

#include "axi_responder.hh"

#include "base/logging.hh"
#include "debug/Verilator.hh"

template <typename ADDRTYPE>
uint8_t
AXIResponder<ADDRTYPE>::read(uint32_t addr)
{
    //fetch a blocks worth of data from gem5 memory model
    axi2gem->doMem(addr, 0, 0);
    return axi2gem->dmemResp;
}

template <typename ADDRTYPE>
void
AXIResponder<ADDRTYPE>::write(uint32_t addr, uint8_t data)
{
    //write data to gem5 memory model
    axi2gem->doMem(addr, 1, data);
}

template <typename ADDRTYPE>
void
AXIResponder<ADDRTYPE>::eval()
{
    /* write request */
    if (*dla.aw_awvalid && *dla.aw_awready)
    {
        DPRINTF(Verilator,"%s: write request from dla, addr %08lx id %d\n",
               conn_name, *dla.aw_awaddr, *dla.aw_awid);

        axi_aw_txn txn;

        txn.awid = *dla.aw_awid;
        txn.awaddr = *dla.aw_awaddr & ~(ADDRTYPE)(AXI_WIDTH / 8 - 1);
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
        DPRINTF(Verilator,"%s: write data from dla (%08x %08x...)\n",
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
        ADDRTYPE addr = *dla.ar_araddr & ~(ADDRTYPE)(AXI_WIDTH / 8 - 1);
        uint8_t len = *dla.ar_arlen;

        DPRINTF(Verilator,"%s: read request from dla,
            addr %08lx burst %d id %d\n",
            conn_name, *dla.ar_araddr, *dla.ar_arlen, *dla.ar_arid);

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
            DPRINTF(Verilator,"%s: write, last tick\n", conn_name);
            aw_fifo.pop();

            axi_b_txn btxn;
            btxn.bid = awtxn.awid;
            b_fifo.push(btxn);
        }
        else
        {
            DPRINTF(Verilator,"%s: write, ticks remaining\n", conn_name);

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
            DPRINTF(Verilator, "%s: read push: id %d,
                   da %08x %08x %08x %08x\n",
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
