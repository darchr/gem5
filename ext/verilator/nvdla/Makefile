# Makefile
# Build system for Verilator testbench
# NVDLA Open Source Project
#
# Copyright (c) 2017 NVIDIA Corporation.  Licensed under the NVDLA Open
# Hardware License.  For more information, see the "LICENSE" file that came
# with this distribution.

GEM5 = ../../..
DEPTH = ../../../../hw
include $(DEPTH)/tools/make/common.make

default: VNV_nvdla

VERILATOR_PARAMS ?= --compiler clang --output-split 250000000
# Perhaps you need to trace?
#   VERILATOR_PARAMS += --trace --trace-depth 7 --trace-max-array 8

# We end up hardcoding this, because the .f file applies only to nv_full,
# living in outdir, right now.  Otherwise, you're on your own...
VNV_nvdla.mk: verilator.f  $(DEPTH)/outdir/nv_full/vmod # and a lot of RTL...
	$(VERILATOR) -CFLAGS -fPIC --cc --exe -f verilator.f nvdla.cpp --compiler clang --output-split 250000000

VNV_nvdla: VNV_nvdla.mk nvdla.cpp
	make -f VNV_nvdla.mk CC=$(CLANG) CXX=$(CLANG)++ VM_PARALLEL_BUILDS=1