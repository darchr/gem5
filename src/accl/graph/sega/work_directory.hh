/*
 * Copyright (c) 2020 The Regents of the University of California.
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

#ifndef __ACCL_GRAPH_SEGA_WORK_DIRECTORY_HH__
#define __ACCL_GRAPH_SEGA_WORK_DIRECTORY_HH__

#include <iostream>

#include "accl/graph/base/data_structs.hh"
#include "base/addr_range.hh"
#include "base/types.hh"

namespace gem5
{

class WorkDirectory
{
  public:
    virtual int activate(Addr atom_addr) = 0;
    virtual int deactivate(Addr atom_addr) = 0;
    virtual Addr getNextWork() = 0;

    virtual int workCount() = 0;
    bool empty() { return workCount() == 0; }

    virtual void setLastAtomAddr(Addr atom_addr) = 0;
};

class PopCountDirectory: public WorkDirectory
{
  private:
    AddrRange memoryRange;

    int numAtomsPerBlock;
    int memoryAtomSize;
    int blockSize;

    uint32_t _workCount;

    int numCounters;
    int lastCounterIndex;
    uint32_t* popCount;

    uint32_t prevIndex;
    uint32_t currentCounter;

    UniqueFIFO<int> activeBlockIndices;

    int getIndexFromAtomAddr(Addr atom_addr)
    {
        assert((atom_addr % memoryAtomSize) == 0);
        Addr trimmed_addr = memoryRange.removeIntlvBits(atom_addr);
        int index = (int) (trimmed_addr / blockSize);
        return index;
    }

    Addr getAtomAddrFromIndex(int block_index, int atom_index)
    {
        Addr block_addr = block_index * blockSize;
        Addr trimmed_addr = block_addr + atom_index * memoryAtomSize;
        return memoryRange.addIntlvBits(trimmed_addr);
    }

  public:
    PopCountDirectory(AddrRange mem_range, int atoms_per_block, int atom_size):
        WorkDirectory(),
        memoryRange(mem_range), numAtomsPerBlock(atoms_per_block),
        memoryAtomSize(atom_size), _workCount(0),
        prevIndex(-1), currentCounter(0)
    {
        blockSize = numAtomsPerBlock * memoryAtomSize;
        int numCounters = (int) (memoryRange.size() / blockSize);
        lastCounterIndex = numCounters - 1;
        popCount = new uint32_t [numCounters];
        for (int index = 0; index < numCounters; index++) {
            popCount[index] = 0;
        }
        activeBlockIndices = UniqueFIFO<int>(numCounters);
    }

    // CAUTION: This should only be called when the work
    // directory **is not** tracking the the atom with atom_addr
    virtual int activate(Addr atom_addr)
    {
        int index = getIndexFromAtomAddr(atom_addr);
        uint32_t prev_count = popCount[index];
        popCount[index]++;
        _workCount++;
        activeBlockIndices.push_back(index);
        assert(popCount[index] > prev_count);
        assert(popCount[index] <= numAtomsPerBlock);
        return popCount[index];
    }

    // CAUTION: This should only be called when the work
    // directory **is** tracking the the atom with atom_addr
    virtual int deactivate(Addr atom_addr)
    {
        int index = getIndexFromAtomAddr(atom_addr);
        uint32_t prev_count = popCount[index];
        popCount[index]--;
        _workCount--;
        if (popCount[index] == 0) {
            activeBlockIndices.erase(index);
        }
        assert(popCount[index] < prev_count);
        assert(popCount[index] <= numAtomsPerBlock);
        return popCount[index];
    }

    virtual int workCount() { return _workCount; }

    void setLastAtomAddr(Addr atom_addr)
    {
        lastCounterIndex = getIndexFromAtomAddr(atom_addr);
    }

    // CAUTION: This directory only tracks active vertices in the memory
    // and it does not have any information on the state of the cache and/or
    // the active buffer or the write buffer. Therefore, it might generate a
    // read request to an address that might be in any of those. In that case,
    // the generated address should be ignored.
    virtual Addr getNextWork()
    {
        // Why ask directory if it's empty?
        assert(!activeBlockIndices.empty());
        int front_index = activeBlockIndices.front();
        assert(popCount[front_index] > 0);
        if ((prevIndex != -1) && (prevIndex != front_index)) {
            currentCounter = 0;
        }
        if (currentCounter == numAtomsPerBlock) {
            currentCounter = 0;
            activeBlockIndices.pop_front();
            activeBlockIndices.push_back(front_index);
        }
        int current_index = activeBlockIndices.front();
        Addr ret_addr = getAtomAddrFromIndex(current_index, currentCounter);
        prevIndex = current_index;
        currentCounter++;
        return ret_addr;
    }
};

} // namespace gem5

#endif // __ACCL_GRAPH_SEGA_WORK_DIRECTORY_HH__
