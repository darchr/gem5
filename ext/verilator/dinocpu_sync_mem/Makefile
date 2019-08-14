all: libVTop__ALL.a

VTop.mk: Top.v DualPortedMemoryBlackBox.v
	verilator -Wno-UNOPTFLAT --cc Top.v -Mdir . -CFLAGS "-g"

verilated.o: VTop.mk
	make -f VTop.mk $@

verilated_dpi.o: VTop.mk
	make -f VTop.mk $@

VTop__ALLcls.o: VTop.mk
	make -f VTop.mk $@

VTop__ALLsup.o: VTop.mk
	make -f VTop.mk $@

libVTop__ALL.a: verilated.o verilated_dpi.o VTop__ALLcls.o VTop__ALLsup.o
	ar rvs libVTop__ALL.a VTop__ALLcls.o VTop__ALLsup.o verilated.o verilated_dpi.o

clean:
	rm -f *.o *.a *.mk *.d VTop*
