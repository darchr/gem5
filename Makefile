
all: daxpy daxpy-gem5 daxpy-accel

daxpy: daxpy.cpp
	g++ daxpy.cpp -o daxpy -std=c++11 -O2 -static -g

daxpy-accel: daxpy.cpp
	g++ daxpy.cpp -o daxpy-accel -std=c++11 -O2 -static -g -DACCEL -DM5OP  util/m5/m5op_x86.S

daxpy-gem5: daxpy.cpp
	g++ daxpy.cpp -o daxpy-gem5 -std=c++11 -O2 -DM5OP util/m5/m5op_x86.S -static -g

clean:
	rm -f daxpy daxpy-gem5 daxpy-accel
