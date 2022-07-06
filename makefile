IDIR =/home/utilities/json/include
CC=gcc
CINCLFLAGS=-I$(IDIR)
CXXVERFLAGS=-std=c++17
CXXTHRFLAGS=-lpthread

output: Source.o
	g++ Source.o -o output $(CXXTHRFLAGS) $(CXXVERFLAGS) $(CINCLFLAGS)

Source.o: Source.cpp
	g++ -c Source.cpp $(CXXTHRFLAGS) $(CXXVERFLAGS) $(CINCLFLAGS)

clean: 
	rm *.o output 