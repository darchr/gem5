# -*- coding: utf-8 -*-
# Copyright (c) 2016 Jason Lowe-Power
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
# Authors: Brian Coutinho, Keshav Mathur, Jason Lowe-Power

""" Support for parsing a gem5 stat and some basic statistics
    Currently this only supports a single stat that is summed up
    over all gem5 instances, typically floating point operations
    or instructions over all CPUs etc.
"""

import subprocess
import math

def mean(data):
    """Return the sample arithmetic mean of data."""
    n = len(data)
    if n < 1:
        raise ValueError('mean requires at least one data point')
    return sum(data)/float(n)

def _ss(data):
    """Return the std square error of data"""
    c = mean(data)
    ss = sum((x-c)**2 for x in data)
    return ss

def std(data):
    """Calculates the population standard deviation."""
    n = len(data)
    ss = _ss(data)
    pvar = ss/n # the population variance
    return pvar**0.5

def sem(cvar, n):
    """ Calculates the std error of the mean or SEM
        This depends on the coefficient of variation
        and number of samples
       @param cvar coefficient of variation
       @param n  number of observations
    """
    return cvar / (n**0.5)

def getVariance(outdir, n, stat='commit.fp_insts'):
    """Calculates the coeff of variation in a stat
       across multiple simulations. The value of the
       stat is added across multiple CPUs.

       Returns the co-efficient of variation i.e. the
       sample std. dev normalized to the mean.

       @param outdir  root directory from where the
                   simulations are subdirectories
                   assumes they are named 0, 1, 2 etc.
       @param n  number of subdirectories
       @param stat  part of the string for the stat
    """

    statfiles = ["%s/%d/stats.txt" % (outdir,i) for i in xrange(n)]

    data = []
    for sf in statfiles:
      # count the stat
      awkcmd = "grep %s %s  | awk 'BEGIN{sum=0} {sum+=$2} END{print sum}'"\
                    % (stat, sf)
      try:
        output = subprocess.check_output(awkcmd, shell=True)
        dv =  float( output )

        if dv != 0.0:
          data.append(dv)
      except:
        print "DEBUG Stats: failed ", stat, " read for ", sf, " out = ", out

    if len(data) == 0:
      return 0
    return std(data) / mean(data)

if __name__=='__main__':

    import sys
    outdir = sys.argv[1]

    print outdir
    print getVariance(outdir, 10)


