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

//gem5 gemeral includes
#include "base/logging.hh"
#include "debug/Verilator.hh"

//gem5 model includes
#include "csb_master.hh"

CSBMaster::CSBMaster(VNV_nvdla *_dla) : dla(_dla)
{
}

void
CSBMaster::CSBInit(){
        dla->csb2nvdla_valid = 0;
        _test_passed = 1;

}

void
CSBMaster::read(uint32_t addr, uint32_t mask, uint32_t data)
{
    csb_op op;

    op.is_ext = 0;
    op.write = 0;
    op.addr = addr;
    op.mask = mask;
    op.data = data;
    op.tries = 10;
    op.reading = 0;

    opq.push(op);
}

void
CSBMaster::write(uint32_t addr, uint32_t data)
{
    csb_op op;

    op.is_ext = 0;
    op.write = 1;
    op.addr = addr;
    op.data = data;

    opq.push(op);
}

void
CSBMaster::ext_event(int ext)
{
    csb_op op;

    op.is_ext = ext;
    opq.push(op);
}

int
CSBMaster::eval(int noop)
{
    if (dla->nvdla2csb_wr_complete)
        printf("write complete from CSB\n");

    dla->csb2nvdla_valid = 0;
    if (opq.empty())
        return 0;

    csb_op &op = opq.front();

    if (op.is_ext && !noop)
    {
        int ext = op.is_ext;
        opq.pop();

        return ext;
    }

    if (!op.write && op.reading && dla->nvdla2csb_valid)
    {
        printf(" read response from nvdla: %08x\n",
           dla->nvdla2csb_data);

        if ((dla->nvdla2csb_data & op.mask) != (op.data & op.mask))
        {
            op.reading = 0;
            op.tries--;
            printf(" invalid response -- trying again\n");
            if (!op.tries)
            {
                printf(" ERROR: timed out reading response\n");
                _test_passed = 0;
                opq.pop();
            }
        }
        else
            opq.pop();
    }

    if (!op.write && op.reading)
        return 0;

    if (noop)
        return 0;

    if (!dla->csb2nvdla_ready)
    {
        printf(" CSB stalled...\n");
        return 0;
    }

    if (op.write)
    {
        dla->csb2nvdla_valid = 1;
        dla->csb2nvdla_addr = op.addr;
        dla->csb2nvdla_wdat = op.data;
        dla->csb2nvdla_write = 1;
        dla->csb2nvdla_nposted = 0;
        printf(" write to nvdla: addr %08x, data %08x\n", op.addr,
            op.data);
        opq.pop();
    }
    else
    {
        dla->csb2nvdla_valid = 1;
        dla->csb2nvdla_addr = op.addr;
        dla->csb2nvdla_write = 0;
        printf("read from nvdla: addr %08x\n", op.addr);

        op.reading = 1;
    }

    return 0;
}

bool
CSBMaster::done()
{
    return opq.empty();
}

int
CSBMaster::test_passed()
{
    return _test_passed;
}
