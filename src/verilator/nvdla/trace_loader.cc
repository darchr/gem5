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

#include "base/logging.hh"
#include "debug/Verilator.hh"
#include "trace_loader.hh"

TraceLoader::TraceLoader(CSBMaster *_csb, AXIResponder<uint64_t> *_axi_dbb,
    AXIResponder<uint64_t> *_axi_cvsram)
{
    csb = _csb;
    axi_dbb = _axi_dbb;
    axi_cvsram = _axi_cvsram;
    _test_passed = 1;
}

void
TraceLoader::load(const char *fname)
{
    int fd;
    fd = open(fname, O_RDONLY);
    if (fd < 0)
    {
        fatal("open(trace file)");
    }

    unsigned char cmd;
    //int rv;
    do
    {
        VERILY_READ(&cmd, 1);

        switch (cmd)
        {
        case 1:
            DPRINTF(Verilator, "CMD: wait\n");
            csb->ext_event(TRACE_WFI);
            break;
        case 2:
        {
            uint32_t addr;
            uint32_t data;

            VERILY_READ(&addr, 4);
            VERILY_READ(&data, 4);
            DPRINTF(Verilator,"CMD: write_reg %08x %08x\n", addr, data);
            csb->write(addr, data);
            break;
        }
        case 3:
        {
            uint32_t addr;
            uint32_t mask;
            uint32_t data;

            VERILY_READ(&addr, 4);
            VERILY_READ(&mask, 4);
            VERILY_READ(&data, 4);
            DPRINTF(Verilator,"CMD: read_reg %08x %08x %08x\n",
                addr, mask, data);
            csb->read(addr, mask, data);
            break;
        }
        case 4:
        {
            uint32_t addr;
            uint32_t len;
            uint8_t *buf;
            uint32_t namelen;
            char *fname;
            axi_op op;

            VERILY_READ(&addr, 4);
            VERILY_READ(&len, 4);
            buf = (uint8_t *)malloc(len);
            VERILY_READ(buf, len);

            VERILY_READ(&namelen, 4);
            fname = (char *)malloc(namelen + 1);
            VERILY_READ(fname, namelen);
            fname[namelen] = 0;

            op.opcode = AXI_DUMPMEM;
            op.addr = addr;
            op.len = len;
            op.buf = buf;
            op.fname = fname;
            opq.push(op);
            csb->ext_event(TRACE_AXIEVENT);

            DPRINTF(Verilator,"CMD: dump_mem %08x bytes from %08x -> %s\n",
                len, addr, fname);
            break;
        }
        case 5:
        {
            uint32_t addr;
            uint32_t len;
            uint8_t *buf;
            axi_op op;

            VERILY_READ(&addr, 4);
            VERILY_READ(&len, 4);
            buf = (uint8_t *)malloc(len);
            VERILY_READ(buf, len);

            op.opcode = AXI_LOADMEM;
            op.addr = addr;
            op.len = len;
            op.buf = buf;
            opq.push(op);
            csb->ext_event(TRACE_AXIEVENT);

            DPRINTF(Verilator,"CMD: load_mem %08x bytes to %08x\n", len, addr);
            break;
        }
        case 0xFF:
            DPRINTF(Verilator,"CMD: done\n");
            break;
        default:
            fatal("unknown command %c\n", cmd);
        }
    } while (cmd != 0xFF);

    close(fd);
}

void
TraceLoader::axievent()
{
    if (opq.empty())
    {
        fatal("extevent with nothing in the queue?\n");
    }

    axi_op &op = opq.front();

    AXIResponder<uint64_t> *axi;
    if ((op.addr & 0xF0000000) == 0x50000000)
        axi = axi_cvsram;
    else if ((op.addr & 0xF0000000) == 0x80000000)
        axi = axi_dbb;
    else
    {
        fatal("AXI event to bad offset\n");
    }

    switch (op.opcode)
    {
    case AXI_LOADMEM:
    {
        const uint8_t *buf = op.buf;

        DPRINTF(Verilator, "AXI: loading memory at 0x%08x\n", op.addr);
        while (op.len)
        {
            axi->write(op.addr, *buf);
            buf++;
            op.addr++;
            op.len--;
        }
        break;
    }
    case AXI_DUMPMEM:
    {
        int fd;
        const uint8_t *buf = op.buf;
        int matched = 1;

        DPRINTF(Verilator,"AXI: dumping memory to %s\n", op.fname);
        fd = creat(op.fname, 0666);
        if (!fd)
        {
            DPRINTF(Verilator, "creat(dumpmem)");
            break;
        }
        while (op.len)
        {
            uint8_t da = axi->read(op.addr);
            ssize_t amtwrtn = write(fd, &da, 1);
            if ( amtwrtn < 0)
                fatal("AXI attempted to write to file, but failed\n");
            if (da != *buf && matched)
            {
                DPRINTF(Verilator,"AXI: FAIL: mismatch at memory address %08x"
                    " (exp 0x%02x,got 0x%02x), and maybe others too\n",
                     op.addr, *buf, da);
                matched = 0;
                _test_passed = 0;
            }
            buf++;
            op.addr++;
            op.len--;
        }
        close(fd);

        if (matched)
            printf("AXI: memory dump matched reference\n");
        break;
    }
    default:
        fatal("Unknown trace event");
    }

    opq.pop();
}

int
TraceLoader::test_passed()
{
    return _test_passed;
}
