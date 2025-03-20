CFLAGS = -Wall -O3
LDFLAGS = -lm -march=native -lfftw3 -pg 

all: run_simulation createPoBinLUT 

# --- CREATE POBIN LUT ----
createPoBinLUT: createPoBinLUT.o pobinfft.o 
	gcc -o createPoBinLUT createPoBinLUT.o pobinfft.o $(LDFLAGS) 

createPoBinLUT.o: createPoBinLUT.c pobinfft.h
	gcc $(CFLAGS) -c createPoBinLUT.c  

# --- RUN SIMULATION ----
run_simulation: run_simulation.o simulatedAnnealing.o pobinfft.o 
	mpicc -o run_simulation run_simulation.o simulatedAnnealing.o pobinfft.o $(LDFLAGS) 

run_simulation.o: run_simulation.c simulatedAnnealing.h pobinfft.h
	mpicc $(CFLAGS) -c run_simulation.c  

simulatedAnnealing.o: simulatedAnnealing.c simulatedAnnealing.h pobinfft.h
	mpicc $(CFLAGS) -c simulatedAnnealing.c  

pobinfft.o: pobinfft.c pobinfft.h
	gcc $(CFLAGS) -c pobinfft.c

# --- CLEAN ----
clean:
	rm -f *.o run_simulation createPoBinLUT
