DEPTH = ../../../../hw
#include $(DEPTH)/tools/make/common.make
NVRTL = outdir/nv_full
SRCS = $(wildcard *.cpp)
OBJFILES = $(SRCS:%.cpp=%.o)

VERILATOR_PARAMS ?= --compiler clang --output-split 250000000


all: libVNV_nvdla__ALL.a

VNV_nvdla.mk: verilator.f
	verilator -Wno-UNOPTFLAT --cc -f verilator.f -Mdir . --compiler clang --output-split 250000000 -CFLAGS -g

verilated.o: VNV_nvdla.mk
	make -f VNV_nvdla.mk $@

verilated_dpi.o: VNV_nvdla.mk
	make -f VNV_nvdla.mk $@

VNV_nvdla__ALLcls.o: VNV_nvdla.mk
	make -f VNV_nvdla.mk CC=$(CLANG) CCX=$(CLANG)++ VM_PARRALLEL_BUILDS=1

VNV_nvdla__ALLsup.o: VNV_nvdla.mk
	make -f VNV_nvdla.mk CC=$(CLANG) CCX=$(CLANG)++ VM_PARRALLEL_BUILDS=1

libVNV_nvdla__ALL.a: verilated.o verilated_dpi.o VNV_nvdla__ALLcls.o VNV_nvdla__ALLsup.o
	ar rvs libVNV_nvdla__ALL.a VNV_nvdla__ALLcls.o VNV_nvdla__ALLsup.o verilated.o verilated_dpi.o

.PHONY: all clean

clean:
	rm -rf *.o *.a *.mk *.d