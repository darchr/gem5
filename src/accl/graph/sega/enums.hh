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

#ifndef __ACCL_GRAPH_SEGA_ENUMS_HH__
#define __ACCL_GRAPH_SEGA_ENUMS_HH__

namespace gem5
{

enum RegisterState
{
    PENDING_READ,
    PENDING_REDUCE,
    PENDING_WRITE,
    NUM_REGISTER_STATE
};
extern const char* registerStateStrings[NUM_REGISTER_STATE];

enum CacheState
{
    INVALID,
    PENDING_DATA,
    BUSY,
    IDLE,
    PENDING_WB,
    NUM_CACHE_STATE
};
extern const char* cacheStateStrings[NUM_CACHE_STATE];

enum ReadReturnStatus
{
    ACCEPT,
    REJECT_ROLL,
    REJECT_NO_ROLL,
    NUM_READ_RETURN_STATUS
};
extern const char* readReturnStatusStrings[NUM_READ_RETURN_STATUS];

enum ReadDestination
{
    READ_FOR_CACHE,
    READ_FOR_PUSH,
    NUM_READ_DESTINATION
};
extern const char* readDestinationStrings[NUM_READ_DESTINATION];

enum ProcessingMode
{
    NOT_SET,
    ASYNCHRONOUS,
    BULK_SYNCHRONOUS,
    POLY_GRAPH,
    NUM_PROCESSING_MODE
};
extern const char* processingModeStrings[NUM_PROCESSING_MODE];

} // namespace gem5

#endif // __ACCL_GRAPH_SEGA_ENUMS_HH__
