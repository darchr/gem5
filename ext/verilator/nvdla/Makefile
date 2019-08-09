DEPTH = ../../../../hw
include $(DEPTH)/tools/make/common.make
NVRTL = outdir/nv_full
OBJFILES =  $(patsubst %.cpp, %.o, ./generated)

default: libVNV_nvdla__ALL.a

VERILATOR_PARAMS ?= --compiler clang --output-split 250000000

VNV_nvdla.mk: verilator.f $(DEPTH)/$(NVRTL)/vmod
	$(VERILATOR) -CFLAGS -g --cc -f verilator.f -Mdir ./generated --compiler clang --output-split 250000000

%.o: VNV_nvdla.mk
	make -C ./generated -f VNV_nvdla.mk CC=$(CLANG) CCX=$(CLANG)++ VM_PARRALLEL_BUILDS=1

libVNV_nvdla__ALL.a: $(OBJFILES)
	ar rvs libVNV_nvdla__ALL.a $(OBJFILES)

.PHONY: clean

clean:
	rm -rf *.o *.a *.mk *.d VNV_nvdla*