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
#include "nvdla_wrapper.hh"

double sc_time_stamp(){
  return 0.0f;
}

NVDLAWrapper::NVDLAWrapper(NVDLAWrapperParams *p):
    DrivenObject(p),
    event([this]{runNVDLA();}, p->name),
    bufferClearCycles(p->buf_clear_cycles),
    system(p->nvdla_sys),
    dla(nullptr),
    csb(nullptr),
    tloader(nullptr),
    axi_dbb(nullptr),
    axi_cvsram(nullptr)
{
    DPRINTF(Verilator, "Setting up NVDLA Wrapper\n");

    if (p->axi_2_gem5 == nullptr)
        fatal("Could not instantiate AXI to gem5 interface");

    resetCycles = p->reset_cycles;
    waiting = 0;
    quiesc_timer = 200;

    dla = driver.getTopLevel();
    //csb reg file (i.e, like instruction decoder)
    csb = new CSBMaster(dla);
    csb->CSBInit();

    //make axi connections
    AXIResponder<uint64_t>::connections dbbconn = {
                .aw_awvalid = &dla->nvdla_core2dbb_aw_awvalid,
                .aw_awready = &dla->nvdla_core2dbb_aw_awready,
                .aw_awid = &dla->nvdla_core2dbb_aw_awid,
                .aw_awlen = &dla->nvdla_core2dbb_aw_awlen,
                .aw_awaddr = &dla->nvdla_core2dbb_aw_awaddr,

                .w_wvalid = &dla->nvdla_core2dbb_w_wvalid,
                .w_wready = &dla->nvdla_core2dbb_w_wready,
                .w_wdata = dla->nvdla_core2dbb_w_wdata,
                .w_wstrb = &dla->nvdla_core2dbb_w_wstrb,
                .w_wlast = &dla->nvdla_core2dbb_w_wlast,

                .b_bvalid = &dla->nvdla_core2dbb_b_bvalid,
                .b_bready = &dla->nvdla_core2dbb_b_bready,
                .b_bid = &dla->nvdla_core2dbb_b_bid,

                .ar_arvalid = &dla->nvdla_core2dbb_ar_arvalid,
                .ar_arready = &dla->nvdla_core2dbb_ar_arready,
                .ar_arid = &dla->nvdla_core2dbb_ar_arid,
                .ar_arlen = &dla->nvdla_core2dbb_ar_arlen,
                .ar_araddr = &dla->nvdla_core2dbb_ar_araddr,

                .r_rvalid = &dla->nvdla_core2dbb_r_rvalid,
                .r_rready = &dla->nvdla_core2dbb_r_rready,
                .r_rid = &dla->nvdla_core2dbb_r_rid,
                .r_rlast = &dla->nvdla_core2dbb_r_rlast,
                .r_rdata = dla->nvdla_core2dbb_r_rdata,
        };
        DPRINTF(Verilator, "Connecting AXI Interface To gem5 Memory (DRAM)\n");

        axi_dbb = new AXIResponder<uint64_t>(dbbconn, "DBB",
             p->axi_2_gem5);

    AXIResponder<uint64_t>::connections cvsramconn = {
                .aw_awvalid = &dla->nvdla_core2cvsram_aw_awvalid,
                .aw_awready = &dla->nvdla_core2cvsram_aw_awready,
                .aw_awid = &dla->nvdla_core2cvsram_aw_awid,
                .aw_awlen = &dla->nvdla_core2cvsram_aw_awlen,
                .aw_awaddr = &dla->nvdla_core2cvsram_aw_awaddr,

                .w_wvalid = &dla->nvdla_core2cvsram_w_wvalid,
                .w_wready = &dla->nvdla_core2cvsram_w_wready,
                .w_wdata = dla->nvdla_core2cvsram_w_wdata,
                .w_wstrb = &dla->nvdla_core2cvsram_w_wstrb,
                .w_wlast = &dla->nvdla_core2cvsram_w_wlast,

                .b_bvalid = &dla->nvdla_core2cvsram_b_bvalid,
                .b_bready = &dla->nvdla_core2cvsram_b_bready,
                .b_bid = &dla->nvdla_core2cvsram_b_bid,

                .ar_arvalid = &dla->nvdla_core2cvsram_ar_arvalid,
                .ar_arready = &dla->nvdla_core2cvsram_ar_arready,
                .ar_arid = &dla->nvdla_core2cvsram_ar_arid,
                .ar_arlen = &dla->nvdla_core2cvsram_ar_arlen,
                .ar_araddr = &dla->nvdla_core2cvsram_ar_araddr,

                .r_rvalid = &dla->nvdla_core2cvsram_r_rvalid,
                .r_rready = &dla->nvdla_core2cvsram_r_rready,
                .r_rid = &dla->nvdla_core2cvsram_r_rid,
                .r_rlast = &dla->nvdla_core2cvsram_r_rlast,
                .r_rdata = dla->nvdla_core2cvsram_r_rdata,
        };
        DPRINTF(Verilator, "Connecting AXI Interface To gem5 Memory(SRAM)\n");

        axi_cvsram = new AXIResponder<uint64_t>(cvsramconn, "CVSRAM",
            p->axi_2_gem5);

        DPRINTF(Verilator, "Setting up Trace Loader\n");

        tloader = new TraceLoader(csb, axi_dbb, axi_cvsram);

}

//creates object for gem5 to use
NVDLAWrapper*
NVDLAWrapperParams::create()
{
  void* ptr = aligned_alloc(128, sizeof(NVDLAWrapper));
  return new(ptr) NVDLAWrapper(this);
}

void NVDLAWrapper::updateCycle(){
    //clock the device
  driver.clockDevice(2,
        &(dla->dla_core_clk),
        &(dla->dla_csb_clk) );
}

void NVDLAWrapper::runNVDLA(){
    DPRINTF(Verilator, "----VERILATOR TICKS PASSED: %d----\n",
        driver.getCyclesPassed());
    //poll csb for new operation requested by external controller
    if (!system->isTracerSystem()){
        //int op = ?
        //csb.ext_event( op )
    }

    DPRINTF(Verilator, "    Executing CSB\n");
    int extevent = csb->eval( waiting );

    //trace specific commands
    DPRINTF(Verilator, "    Executing Trace\n");
    if (system->isTracerSystem()){
        if (extevent == TraceLoader::TRACE_AXIEVENT)
                        tloader->axievent();
                else if (extevent == TraceLoader::TRACE_WFI) {
                        waiting = 1;
                }

                if (waiting && dla->dla_intr) {
                        waiting = 0;
                }
    }


    //axi events for memory
    DPRINTF(Verilator, "    Executing AXI DRAM\n");
    axi_dbb->eval();
    DPRINTF(Verilator, "    Executing AXI SRAM\n");
    axi_cvsram->eval();

    //evaluate nvdla state
    DPRINTF(Verilator, "    Executing Next NVDLA Cycle\n");
    updateCycle();

    //continue execution?
    //us csb->done only for traces? will controller
    //submitting commands ever allow for empty buffer of nvdla commands?
    if ( !csb->done() || quiesc_timer--){
        schedule(event, nextCycle());
    }else if (system->isTracerSystem()){
        inform("****TRACE HAS COMPLETED @ Verilator Tick %d****\n",
            driver.getCyclesPassed() );

        if (!tloader->test_passed()) {
            fatal("*** FAIL: test failed due to output mismatch\n");
        }

        if (!csb->test_passed()) {
            fatal("*** FAIL: test failed due to CSB read mismatch\n");
        }

        if (csb->test_passed() && tloader->test_passed() )
            inform("*** PASS\n");
    }

}

void NVDLAWrapper::initNVDLA(){
    DPRINTF(Verilator, "Initializing nvdla clock and power signals\n");

    dla->global_clk_ovr_on = 0;
    dla->tmc2slcg_disable_clock_gating = 0;
    dla->test_mode = 0;
    dla->nvdla_pwrbus_ram_c_pd = 0;
    dla->nvdla_pwrbus_ram_ma_pd = 0;
    dla->nvdla_pwrbus_ram_mb_pd = 0;
    dla->nvdla_pwrbus_ram_p_pd = 0;
    dla->nvdla_pwrbus_ram_o_pd = 0;
    dla->nvdla_pwrbus_ram_a_pd = 0;
}

void NVDLAWrapper::resetNVDLA(){
    DPRINTF(Verilator, "Resetting nvdla for %d cycles\n", resetCycles*2);

    char fmt[2] = {0,0};
    //reset
    dla->dla_reset_rstn = 1;
    dla->direct_reset_ = 1;
    dla->eval();

    driver.reset(resetCycles, fmt, 2,
        &(dla->dla_core_clk),
        &(dla->dla_csb_clk));

    dla->dla_reset_rstn = 0;
    dla->direct_reset_ = 0;
    dla->eval();

     driver.reset(resetCycles, fmt, 2,
        &(dla->dla_core_clk),
        &(dla->dla_csb_clk));
}

void NVDLAWrapper::initClearDLABuffers(){

    DPRINTF(Verilator, "Clearing hardware buffers for %d cycles\n",
        bufferClearCycles);

    char fmt[2] = {0,0};

    //clear buffers
    dla->dla_reset_rstn = 1;
    dla->direct_reset_ = 1;

     driver.reset(bufferClearCycles, fmt, 2,
        &(dla->dla_core_clk),
        &(dla->dla_csb_clk));
}

void NVDLAWrapper::startup(){


    //init NVDLA
    initNVDLA();

    const char * argv [2] = {"/home/nganjehl/gem5/build/NULL/gem5.debug", ""};
    argv[1] = system->getTracePath();
    Verilated::commandArgs(2, argv);

    if (system->isTracerSystem())
        tloader->load(system->getTracePath());

    //reset
    resetNVDLA();
    //clear hardware buffers
    initClearDLABuffers();

    schedule(event, nextCycle());
}
