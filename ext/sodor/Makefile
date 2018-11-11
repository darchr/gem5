all: libVTop__ALL.a

VTop.mk: Top.v AsyncReadMem.sv SimDTM.v
	verilator --cc Top.v -Mdir .

verilated.o: VTop.mk
	make -f VTop.mk $@

verilated_dpi.o: VTop.mk
	make -f VTop.mk $@

VTop__ALLcls.o: VTop.mk
	make -f VTop.mk $@

VTop__ALLsup.o: VTop.mk
	make -f VTop.mk $@

sc_time_stamp.o:
	g++ -c sc_time_stamp.cpp

libVTop__ALL.a: verilated.o verilated_dpi.o VTop__ALLcls.o VTop__ALLsup.o sc_time_stamp.o
	ar rvs libVTop__ALL.a VTop__ALLcls.o VTop__ALLsup.o verilated.o verilated_dpi.o sc_time_stamp.o

clean:
	rm -f *.o *.a *.mk *.d VTop*
