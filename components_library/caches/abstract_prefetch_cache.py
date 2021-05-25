# Copyright (c) 2021 The Regents of the University of California
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

from m5.objects import Cache, StridePrefetcher

class AbstractPrefetchCache(Cache):
    """
    Classes which inherit from the AbstractPrefetchCache are prefetch caches.
    This class provides common properties and methods for all prefetch caches.
    """

    def __init__(
        self,
        size: str,
        assoc: int,
        tag_latency: int,
        data_latency: int,
        response_latency: int,
        mshrs: int,
        tgts_per_mshr: int,
        writeback_clean: bool,
    ):
        """
        :param size: The size of the cache (e.g, "32kB").

        :type size: str

        :param assoc: The associativity of the cache.

        :type assoc: int

        :param tag_latency: The tag latency of the cache.

        :type tag_latency: int

        :param data_latency: The data letency of the cache.

        :type data_latency: int

        :param responce_latency: The responce latency of the cache.

        :type responce_latency: int

        :param mshrs: Number of MSHRs (??? #TODO: I have no idea)

        :type mshrs: int

        :param tgts_per_mshr: The Ticket-Granting-Tickets (TGT) per HSHR.

        :type tgts_per_mshr: int

        :param writeback_clean: Whether the writeback is clean.

        :type writeback_clean: bool
        """
        super(AbstractPrefetchCache, self).__init__()
        self.size = size
        self.assoc = assoc
        self.tag_latency = tag_latency
        self.data_latency = data_latency
        self.response_latency = response_latency
        self.mshrs = mshrs
        self.tgts_per_mshr = tgts_per_mshr
        self.writeback_clean = writeback_clean
        self.prefetcher = StridePrefetcher()


#  @abstractmethod
#  def get_bus_side(self):
#      raise NotImplementedError

#   @abstractmethod
#   def connect_cpu_side(self):
#       raise NotImplementedError
