all: vmsim

vmsim: vmsim.o
	g++ -o vmsim vmsim.o

vmsim.cpp: vmsim.cpp
	g++ -c vmsim.cpp