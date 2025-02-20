CFLAGS=-Wall -O3
LDFLAGS=-lm -march=native -lfftw3 -pg 

all: mcmcSolver createPoBinLUT 
createPoBinLUT: createPoBinLUT.o pobinfft.o 
	gcc -o createPoBinLUT pobinfft.o  createPoBinLUT.o $(LDFLAGS) 

createPoBinLUT.o: createPoBinLUT.c pobinfft.h
	gcc $(CFLAGS) -c createPoBinLUT.c  

mcmcSolver: mcmcSolver.o pobinfft.o 
	mpicc -o mcmcSolver pobinfft.o  mcmcSolver.o $(LDFLAGS) 

mcmcSolver.o: mcmcSolver.c mcmcSolver.h pobinfft.h
	mpicc $(CFLAGS) -c mcmcSolver.c  

pobinfft.o: pobinfft.c pobinfft.h
	gcc $(CFLAGS) -c pobinfft.c
clean:
	rm -f ./mcmcSolver *.o 