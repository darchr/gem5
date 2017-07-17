/* Copyright (c) 2016 Mark D. Hill and David A. Wood
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
 *
 * Authors: Jason Lowe-Power
 */

#ifndef __MEM_DRAM_CACHE_DRAM_CACHE_CTRL_FOM_HH__
#define __MEM_DRAM_CACHE_DRAM_CACHE_CTRL_FOM_HH__

#include "mem/dram_cache/dram_cache_ctrl.hh"
#include "params/DRAMCacheCtrlFOM.hh"

class DRAMCacheCtrlFOM : public DRAMCacheCtrl
{
  public:
    typedef DRAMCacheCtrlFOMParams Params;

    /**
     * Constructor based on the Python params
     *
     * @param params Python parameters
     */
    DRAMCacheCtrlFOM(Params* params);

    void recvStorageResponse(PacketPtr pkt, bool hit) override;

  private:
    bool recvTimingReq(PacketPtr pkt, bool from_cache=true) override;
    bool recvTimingResp(PacketPtr pkt) override;
    void handleRead(PacketPtr pkt) override;
    bool canRecvStorageResp(PacketPtr pkt, bool hit, bool dirty);


};

#endif
