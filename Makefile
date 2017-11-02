CC=/home2/jlp/gcc-linaro-5.4.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-g++

daxpy: daxpy.cc
	$(CC) -Iinclude --std=c++11 -static daxpy.cc util/m5/m5op_arm_A64.S -o daxpy -O1
