#!/usr/bin/env python
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
# Authors: Keshav Mathur, Jason Lowe-Power, Brian Coutinho

# Python script to analyse and condense parsed data
from __future__ import division
import os,sys
from termcolor import colored

import numpy as np
import pandas
from tardis_parser import *
from scipy import stats


# User defined dictionary describing the statistics to be collected
# Key (string): name of the statistic in stat.txt file
# Value (3 tuple):  1 - size of statistic, 0 for scalar,
#                       positive integer vector length for vector
#                   2 - data type of statistic
#                   3 - alternate name if desired
# To use wildcards in the stat name use '%d' (for vector stats)
#
# Add to dictionary as desired

stat_dict = { 'sim_ticks' : (0,int,''),
              'system.clk_domain.clock' : (0,int,''),
              'system.timingCpu%d.numCycles' : (8,int,''),
              'system.timingCpu%d.committedInsts' : (8,int,''),
              'system.timingCpu%d.ipc_total' : (8,float,'')
         }

def getDerivedStats(df):
    """ Function to derive statistics from raw values
    Operates on dataframe passed during the call
    """
    # sim_cycles ( or simulation cycles) is added to dataframe
    # by dividing already present ticks with clock
    df['sim_cycles'] = df['sim_ticks']/df['clock']

    # Aggregate IPC is derived by dividing total
    # committed instructions with simulation cycles
    df['agg_ipc'] = df['committedInsts'].apply(sum)/ df['sim_cycles']


def condenseRuns(stat_name,df):
    """ Function to condense stat for each sample point.
    Takes the average over runs and returns list over samples
    This is used to handle space variance allowing time variation
    analysis of the TARDIS methodology.

    Pass a dataframe that has a single config selected
    @param stat_name : statistic to take average of
    @param df: pandas dataframe consisting of parsed values
    """
    # Allow data frame to be indexed by Sample name
    df1 = df.set_index('Sample')

    # Get list of unique Sample points simulated
    smpls = df.Sample.unique()

    # Get a list of desired statistic across all runs at that sample point
    # Data is a list of such lists
    data = [df1[stat_name].loc[point] for point in smpls]

    # For each Sample point, calculate mean over the runs
    mean_stat_list = [np.mean(data[i]) for i in range(len(smpls))]

    # Return a list across Sample points where each element
    # is mean of runs at that point
    return mean_stat_list

def checkConfidence(confInt,confLevel,dataList):
    """ Function to check if desired confidence level is achieved
    on the approximation process.
    @param confInt: The error bound or confidence interval desired
    @param confLevel: Level of confidence desired on the process
                      generating the error bound
    @param dataList: The list of values to be approximated
    """
    # Small table for z-values based on desird confidence level
    # TODO use a scipy function here, below are commonly used
    ztable = {0.90 : 1.645,  0.95 : 1.96, 0.99 : 2.576}

    if confLevel not in ztable.keys():
        return -1

    # Calculate confidence interval using covariance of available population
    nroot = np.sqrt(len(dataList))
    cvar = np.std(dataList)
    interval = cvar * ztable[confLevel] / nroot

    # Compare with provided confidence interval
    if (abs(interval-confInt) > 0.01 ) :
        print "Confidence level on spatial readings out of permissible limits."
        return False
    else:
        return True


def norm_n_estimate(alpha, error, cvar):
    """ Function to estimate the number of samples needed
    to achive desired confidence level over the confidence
    interval of the statistic

    @param alpha: confidence level desired on the process
    @param error: permissible error bound on the statistic
    @param cvar: covariance ( sigma/mu ) of the statistic
    """
    ztable = {0.90 : 1.645,  0.95 : 1.96, 0.99 : 2.576}
    if alpha not in ztable.keys():
        return -1

    # sqrt of the desired number of samples
    nroot = cvar * ztable[alpha] / error

    return nroot * nroot


def populate_stats(data_vect):
    """ Function that carries out statistical analysis and reports the
    estimated samples needed to get the desired confidence level
    """
    astats = dict()
    # Mean of all time varying samples
    astats['mean'] = np.mean(data_vect)

    # Variance across time varying samples
    astats['var'] = np.var(data_vect)

    # Standard deviation of samples
    astats['std'] = np.std(data_vect)

    # Covariance of samples
    astats['cvar'] = stats.variation(data_vect)

    #to add t-conf
    # Calculate estimated N for different combinations
    # of Confidence Interval and level
    astats['n_95_3p'] = norm_n_estimate(0.95, 0.03, astats['cvar'])
    astats['n_95_5p'] = norm_n_estimate(0.95, 0.05, astats['cvar'])
    astats['n_95_10p'] = norm_n_estimate(0.95, 0.10, astats['cvar'])
    astats['n_95_15p'] = norm_n_estimate(0.95, 0.15, astats['cvar'])

    return astats

def SpaceVarianceAnalysis(nativedata,config,stat_name,error,confLevel):
    """ Function that carries out space variance analysis. This checks if
    the average of the runs is within permissible limits of allowed
    error bound or if additional runs shall be required.
    """

    # Select the desired config in the data frame
    df1 = nativedata[( nativedata.config == config)]
    print "Selected config:%s for space variance analysis on Stat:%s"\
            %(config,stat_name)
    # Find sample points for this config
    smpls = nativedata.Sample.unique()

    # Condense runs for each sample
    df2 = df1.set_index('Sample')
    data = [df2[stat_name].loc[point] for point in smpls]

    # Check confidence levels and error bounds
    mean_stat_list = [np.mean(data[i]) for i in range(len(smpls))]
    ReqRuns = []
    ActRuns = []
    print "\nChosen Error Bound:%f\t Chosen confidence level:%f"\
        %(error,confLevel)
    print "Act. Runs\tReq. Runs\n"
    for i in range(len(smpls)):
        cvar = stats.variation(data[i])
        ReqRuns.append(np.ceil(norm_n_estimate(confLevel,error,cvar)))
        ActRuns.append(len(data[i]))
        if ActRuns[i] < ReqRuns[i]:
            print colored("%d\t%d\n",'green') %(ActRuns[i],ReqRuns[i])
        else:
            print colored("%d\t%d\n",'red') %(ActRuns[i],ReqRuns[i])

    return mean_stat_list



def TimeVarianceAnalysis(stat_list,stat_name,error,confLevel):
    """ This function operates on a dataframe consisting of parsed statistics
    of a particular architectural configuration of a given suite/app."""



    astats = populate_stats(stat_list)

    Nest = norm_n_estimate(confLevel,error,astats['cvar'])
    print "Actual Samples:%d\tRequired Samples:%d" % (len(stat_list),Nest)


if  __name__ == '__main__':

    print """Beginning TARDIS Simulation Analysis...\n"""

    usage = " Usage \n" \
            "   tardis_analysis.py  [basepath_of_outputdirs]"

    if len(sys.argv) != 2:
        print "Error: missing one argument\n"
        print len(sys.argv)
        print usage
        sys.exit(1)

    rows = []

    basepath = str(sys.argv[1])
    print "\nWorking from %s" % basepath

    s_path,my_suites = discoverSuites(basepath)
    print "Discovered benchmark suites( Enter one):\n"
    print my_suites
    suite = raw_input()

    a_path,my_apps = discoverApps(basepath,suite)
    print "Discovered benchmarks(Select one):\n"
    print my_apps
    app = raw_input()

    c_path,my_configs =  discoverConfigs(basepath,suite,app)
    print "Discovered Configurations(select one):\n"
    print my_configs
    config = raw_input()

    print "\nParsing %s/%s/%s/%s\n" %(basepath,suite,app,config)

    # Get the dataframe for desired configuration
    mydf = load_stats(stat_dict,basepath,suite,app,config,rows)

    # Populate dataframe with derived statistics
    getDerivedStats(mydf)

    # Carry out Space Variance analysis on aggregate ipc metric using
    #  error = 5% and confidence = 95%
    ipc_time = SpaceVarianceAnalysis(mydf,config,'agg_ipc',0.05,0.95)

    # Carry out Time Variance analysis on aggregate ipc metric using
    #  error = 5% and confidence = 95%
    TimeVarianceAnalysis(ipc_time,'agg_ipc',0.05,0.95)



