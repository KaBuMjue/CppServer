CXX=g++
CFLAGS=-g -O2 -Wall -fPIC -Wno-deprecated

INC=-I../../include
LIB=-L../../lib -lrreactor
OBJS = $(addsuffix .o, $(basename $(wildcard *.cc)

all:
	$(CXX) -o royal_reactor $(CFLAGS) royal_reactor.cpp $(INC) $(LIB)

clean:
	-rm -f *.o royal_reactor
