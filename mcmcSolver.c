#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<string.h>
#include<math.h>
#include"pobinfft.h"
#include"mcmcSolver.h"
#include<mpi.h>





int main(int argc, char **argv){

    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        printf("------------------------------------------------\n");
        printf("WELCOME TO THE SIMULATION\n");
        printf("------------------------------------------------\n");
        
    }


    srand(rank);
    const int NUM_GAMES = 13;
    int num_obj_read;
    const int POOL_SIZE = (int)2e6;
    FILE *file_odds;
    FILE *file_probs;

    probs_t probs[NUM_GAMES];
    odds_t odds[NUM_GAMES];

    double picking_probs[3*NUM_GAMES];
    double implied_probs[3*NUM_GAMES];
    

    // Read probs
    file_probs = fopen("probs.bin", "rb");
    num_obj_read = fread(probs, sizeof(probs_t), NUM_GAMES, file_probs);
    fclose(file_probs);
    if (num_obj_read != NUM_GAMES){
        printf("Something went wrong reading probs.\n");
        return -1;
    }

    // Read odds
    file_odds = fopen("odds.bin", "rb");
    num_obj_read = fread(odds, sizeof(odds_t), NUM_GAMES, file_odds);
    fclose(file_odds);
    if (num_obj_read != NUM_GAMES){
        printf("Something went wrong reading odds.\n");
        return -1;
    }
    
    // Implied probs and picking probs
    memcpy(&implied_probs[0], &odds[0], NUM_GAMES * 3 * sizeof(double));
    memcpy(&picking_probs[0], &probs[0], NUM_GAMES * 3 * sizeof(double));
    
    

    int *sample_space = (int *)malloc(READ_NUMBER * NUM_GAMES * sizeof(int));
    
    // Read the bets into sample space
    const char *filename_bets = "PossibleBets.bin";

    FILE *file_bets = fopen(filename_bets, "rb");
    if (file_bets == NULL){
        printf("Error opening file\n");
        return 1;
    }
    
    int bytesRead = fread(&sample_space[0], sizeof(int), READ_NUMBER * NUM_GAMES, file_bets);
    if(bytesRead != (READ_NUMBER*NUM_GAMES)){
        perror("Error reading\n");
        return 1;
    }
    if (fclose(file_bets) != 0){
        fprintf(stderr, "Error while closing file\n");
        return 1;
    }

    //Read in the PoBin LUT
    FILE * file_pobin = fopen("PoBinLUT.bin", "rb");
    double *pobin_lut = (double *)malloc(sizeof(double)*SAMPLE_SPACE_SIZE*(NUM_GAMES+1));
    size_t read_bytes = fread(pobin_lut, sizeof(double), SAMPLE_SPACE_SIZE*(NUM_GAMES+1), file_pobin);
    assert(read_bytes);
    
    fclose(file_pobin);

    int BURNIN_ITERS = 1000;
    int SAMPLE_ITERS = 1000;
    int y[] = {1, 2, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0};

    //Generate propasal y from uniform distribution
        for(int j=0; j<NUM_GAMES;j++)
            y[j] = rand() % 3;
    
    int best_y[NUM_GAMES];
    memcpy(best_y, y, NUM_GAMES*sizeof(int));
    
    MPI_Barrier(MPI_COMM_WORLD);

    double best_EV = simulated_annealing(&y[0], &best_y[0], &sample_space[0], &pobin_lut[0], &picking_probs[0], 
    &implied_probs[0], POOL_SIZE, NUM_GAMES, BURNIN_ITERS, SAMPLE_ITERS, rank, size);
    
    MPI_Barrier(MPI_COMM_WORLD);
    double *all_EV = NULL;

    if (rank == 0) {
        all_EV = (double *)malloc(size * sizeof(double)); // Buffer to gather all EV values
    }

    // Gather all best_EV values from each rank into all_EV on rank 0
    MPI_Gather(&best_EV, 1, MPI_DOUBLE, all_EV, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    
    int best_rank = 0;
    double global_best_EV = 0.0;

    if (rank == 0) {
        printf("------------------------------------------------\n");
        global_best_EV = all_EV[0];
        printf("EV from core 0: %lf\n", global_best_EV);
        for (int i = 1; i < size; i++) {
            printf("EV from core %i: %lf\n", i, all_EV[i]);
            if (all_EV[i] > global_best_EV) {
                global_best_EV = all_EV[i];
                best_rank = i;
            }
        }
        printf("------------------------------------------------\n");
        free(all_EV);
    }

    // Broadcast the best rank to all processes
    MPI_Bcast(&best_rank, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
   

    //Print best pick
    if(rank == best_rank){
        char signs[] = {'1', 'X', '2'};
        // Print header with proper spacing
        printf("Game   Pick  picking p (sweds)    Implied p (odds)\n");
        printf("------------------------------------------------\n");

        // Print values with aligned columns
        for (int i = 0; i < NUM_GAMES; i++) {
            printf("Game %2d: %c   %5.6lf    %17.6lf\n",
                i + 1,
                signs[best_y[i]],
                picking_probs[3 * i + best_y[i]],
                1.0 / implied_probs[3 * i + best_y[i]]);
        }
    printf("------------------------------------------------\n");
    printf("EV=%lf \n", best_EV);
    }
   
    MPI_Finalize();
    free(sample_space);
    free(pobin_lut);
    return 0;
}
