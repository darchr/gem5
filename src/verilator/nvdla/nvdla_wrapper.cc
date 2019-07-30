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
    event([this]{runNVDLA();}, params->name)
    testTrace(p->do_trace),
    resetCycles(p->reset_cycles),
    bufferClearCycles(p->buf_clear_cycles),
    tracePath(p->trace_file)
{
    waiting = 0;
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
        &(driver.getTopLevel()->dla_core_clk),
        &(driver.getTopLevel()->dla_csb_clk) );
}

void NVDLAWrapper::runNVDLA(){
    //poll csb for new operation requested by external controller
    if (!testTrace){
        //int op = ?
        //csb.ext_event( op )
    }

    int extevent = csb.eval( waiting );

    //trace specific commands
    if (testTracce){
        if (extevent == TraceLoader::TRACE_AXIEVENT)
                        tloader.axievent();
                else if (extevent == TraceLoader::TRACE_WFI) {
                        waiting = 1;
                }

                if (waiting && driver.getTopLevel()->dla_intr) {
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
    if ( !csb->done() || !driver.isFinished())
        schedule(event, nextCycle());

}

void NVDLAWrapper::initNVDLA(){
    driver.getTopLevel()->global_clk_ovr_on = 0;
        driver.getTopLevel()->tmc2slcg_disable_clock_gating = 0;
        driver.getTopLevel()->test_mode = 0;
        driver.getTopLevel()->nvdla_pwrbus_ram_c_pd = 0;
        driver.getTopLevel()->nvdla_pwrbus_ram_ma_pd = 0;
        driver.getTopLevel()->nvdla_pwrbus_ram_mb_pd = 0;
        driver.getTopLevel()->nvdla_pwrbus_ram_p_pd = 0;
        driver.getTopLevel()->nvdla_pwrbus_ram_o_pd = 0;
        driver.getTopLevel()->nvdla_pwrbus_ram_a_pd = 0;
}

void NVDLAWrapper::resetNVDLA(){
    char fmt[2] = {0,0};
    //reset
    driver.getTopLevel()->dla_reset_rstn = 1;
    driver.getTopLevel()->direct_reset_ = 1;
    driver.getTopLevel()->eval();

    driver.reset(resetCycles, fmt,
        &(driver.getTopLevel()->dla_core_clk),
        &(driver.getTopLevel()->dla_csb_clk));

    driver.getTopLevel()->dla_reset_rstn = 0;
    driver.getTopLevel()->direct_reset_ = 0;
    driver.getTopLevel()->eval();

     driver.reset(resetCycles, fmt,
        &(driver.getTopLevel()->dla_core_clk),
        &(driver.getTopLevel()->dla_csb_clk));
}

void NVDLAWrapper::initClearDLABuffers(){
    char fmt[2] = {0,0};

    //clear buffers
    driver.getTopLevel()->dla_reset_rstn = 1;
    driver.getTopLevel()->direct_reset_ = 1;

     driver.reset(bufferClearCycles, fmt,
        &(driver.getTopLevel()->dla_core_clk),
        &(driver.getTopLevel()->dla_csb_clk));
}


void NVDLAWrapper::startup(){
    if (testTrace)
        tloader.load(tracePath);
    //init NVDLA
    initNVDLA();
    //reset
    resetNVDLA();
    //clear hardware buffers
    clearDLABuffers();

    schedule(event, nextCycle());
}
