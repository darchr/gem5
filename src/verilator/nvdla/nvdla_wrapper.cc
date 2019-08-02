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

#include "nvdla_wrapper.hh"

NVDLAWrapper::NVDLAWrapper(NVDLAWrapperParams *p):
    DrivenObject(p),
    event([this]{runNVDLA();}, p->name),
    bufferClearCycles(p->buf_clear_cycles),
    system(p->nvdla_sys),
    dla(nullptr),
    csb(nullptr),
    tloader(&csb, axi_dbb, axi_cvsram),
    axi_dbb(nullptr),
    axi_cvsram(nullptr)
{
    resetCycles = p->reset_cycles;
    tracePath = (p->trace_file).c_str();
    waiting = 0;

    dla = driver.getTopLevel();
    //csb reg file (i.e, like instruction decoder)
    csb = CSBMaster(dla);

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
        axi_dbb = new AXIResponder<uint64_t>(dbbconn, "DBB");

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
        axi_cvsram = new AXIResponder<uint64_t>(cvsramconn, "CVSRAM");


}

//creates object for gem5 to use
NVDLAWrapper*
NVDLAWrapperParams::create()
{
  //verilator has weird alignment issue for generated code
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
    //poll csb for new operation requested by external controller
    if (!system->isTracerSystem()){
        //int op = ?
        //csb.ext_event( op )
    }

    int extevent = csb.eval( waiting );

    //trace specific commands
    if (system->isTracerSystem()){
        if (extevent == TraceLoader::TRACE_AXIEVENT)
                        tloader.axievent();
                else if (extevent == TraceLoader::TRACE_WFI) {
                        waiting = 1;
                }

                if (waiting && dla->dla_intr) {
                        waiting = 0;
                }
    }

    //axi events for memory
    axi_dbb->eval();
    axi_cvsram->eval();

    //evaluate nvdla state
    updateCycle();

    //continue execution?
    //us csb->done only for traces? will controller
    //submitting commands ever allow for empty buffer of nvdla commands?
    if ( !csb.done() || !driver.isFinished())
        schedule(event, nextCycle());

}

void NVDLAWrapper::initNVDLA(){
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
    char fmt[2] = {0,0};
    //reset
    dla->dla_reset_rstn = 1;
    dla->direct_reset_ = 1;
    dla->eval();

    driver.reset(resetCycles, fmt,
        &(dla->dla_core_clk),
        &(dla->dla_csb_clk));

    dla->dla_reset_rstn = 0;
    dla->direct_reset_ = 0;
    dla->eval();

     driver.reset(resetCycles, fmt,
        &(dla->dla_core_clk),
        &(dla->dla_csb_clk));
}

void NVDLAWrapper::initClearDLABuffers(){
    char fmt[2] = {0,0};

    //clear buffers
    dla->dla_reset_rstn = 1;
    dla->direct_reset_ = 1;

     driver.reset(bufferClearCycles, fmt,
        &(dla->dla_core_clk),
        &(dla->dla_csb_clk));
}

void NVDLAWrapper::startup(){
    if (system->isTracerSystem())
        tloader.load(system->getTracePath());
    //init NVDLA
    initNVDLA();
    //reset
    resetNVDLA();
    //clear hardware buffers
    initClearDLABuffers();

    schedule(event, nextCycle());
}
