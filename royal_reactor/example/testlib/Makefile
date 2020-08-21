CXX=g++
CFLAGS=-g -O2 -Wall -fPIC -Wno-deprecated

INC=-I../../include
LIB=-L../../lib -lrreactor
OBJS = $(addsuffix .o, $(basename $(wildcard *.cc)))

all:
	$(CXX) -o hello_royal $(CFLAGS) hello_royal.cpp $(INC) $(LIB)

clean:
	-rm -f *.o hello_royal
