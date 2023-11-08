CXX=g++
CXXFLAGS=-Wall -Wextra -pedantic -std=c++11

counter: counter.cpp
	$(CXX) $(CXXFLAGS) -o counter counter.cpp

consumer: consumer.cpp
	$(CXX) $(CXXFLAGS) -o consumer consumer.cpp

generator: generator.cpp
	$(CXX) $(CXXFLAGS) -o generator generator.cpp

# make all 
all: counter consumer generator

clean:
	rm -f counter consumer generator
	