#!/bin/bash

ln -s ../../disaggregated_memory_setup_arm
ln -s ../../ext/sst/libgem5.so
ln -s ../../ext/sst/sst

# Use sst.self to use SelfPartitioner, which uses the rank set by setRank() in the SST config file
/scr/hn/opt/mpi/bin/mpiexec sst --add-lib-path=./ --partitioner=sst.self sst/memory-pool-example-atomic-multinodes.py
