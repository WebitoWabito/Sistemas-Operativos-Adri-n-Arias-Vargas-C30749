CXX = g++
CXXFLAGS = -Wall -std=c++11

all: rotonda

rotonda: Rotonda.cc Buzon.cc Buzon.h
	$(CXX) $(CXXFLAGS) -o rotonda Rotonda.cc Buzon.cc

clean:
	rm -f rotonda
	# Se limpian las colas de mensajes huerfanas si hay
	ipcs -q | awk '/0xecci2026/ {print $$2}' | xargs -I{} ipcrm -q {} 2>/dev/null || true
