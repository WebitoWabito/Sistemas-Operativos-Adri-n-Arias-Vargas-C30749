CXX = g++
CXXFLAGS = -Wall -g

all: enviarConClases recibirConClases

enviarConClases: enviarConClases.cc Buzon.o
	$(CXX) $(CXXFLAGS) -o $@ enviarConClases.cc Buzon.o

recibirConClases: recibirConClases.cc Buzon.o
	$(CXX) $(CXXFLAGS) -o $@ recibirConClases.cc Buzon.o

Buzon.o: Buzon.cc Buzon.h
	$(CXX) $(CXXFLAGS) -c Buzon.cc

clean:
	rm -f *.o enviarConClases recibirConClases
