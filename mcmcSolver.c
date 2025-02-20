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



    srand(rank);
    const int NUM_GAMES = 13;
    int num_obj_read;
    const int POOL_SIZE = (int)1e5;
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
    fread(pobin_lut, sizeof(double), SAMPLE_SPACE_SIZE*(NUM_GAMES+1), file_pobin);
    fclose(file_pobin);

    int BURNIN_ITERS = 1000;
    int SAMPLE_ITERS = 1000;
    int y[] = {1, 2, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0};

    //Generate propasal y from uniform distribution
        for(int j=0; j<NUM_GAMES;j++)
            y[j] = rand() % 3;
    
    int best_y[NUM_GAMES];
    memcpy(best_y, y, NUM_GAMES*sizeof(int));

    double best_EV;
    double EV;
    compute_expected_value(&best_EV, y, sample_space, pobin_lut,picking_probs, implied_probs, POOL_SIZE, NUM_GAMES);
    if(rank==0)
        printf("Starting MCMC on %d chains. Running %d (%d burn-in phase + %d sample phase) samples\n", size, size*(BURNIN_ITERS+SAMPLE_ITERS), size*BURNIN_ITERS, size*SAMPLE_ITERS);
    double alpha; 
    for(int i=0;i<BURNIN_ITERS;i++){

        //Generate propasal y from uniform distribution
        for(int j=0; j<NUM_GAMES;j++)
            y[j] = rand() % 3;
        
        //Compute EV
        compute_expected_value(&EV, y, sample_space, pobin_lut, picking_probs, implied_probs, POOL_SIZE, NUM_GAMES);
        alpha = (double)rand() / (RAND_MAX + 1.0);
        if(EV > best_EV){
            best_EV = EV;
            memcpy(best_y, y, NUM_GAMES*sizeof(int));
        }
        else if(alpha < EV/best_EV){
            best_EV = EV;
            memcpy(best_y, y, NUM_GAMES*sizeof(int));
        }
        if(rank==0)
            print_loading_bar(i + 1, BURNIN_ITERS, "Burn-in phase", i + 1 == BURNIN_ITERS);
    }

    for(int i=0;i<SAMPLE_ITERS;i++){

        //Generate propasal y from uniform distribution
        for(int j=0; j<NUM_GAMES;j++)
            y[j] = rand() % 3;
        
        //Compute EV
        compute_expected_value(&EV, y, sample_space, pobin_lut, picking_probs, implied_probs, POOL_SIZE, NUM_GAMES);
        alpha = (double)rand() / (RAND_MAX + 1.0);
        if(EV > best_EV){
            best_EV = EV;
            memcpy(best_y, y, NUM_GAMES*sizeof(int));
        }
        else if(alpha < EV/best_EV){
            best_EV = EV;
            memcpy(best_y, y, NUM_GAMES*sizeof(int));
        }
        if(rank==0)
            print_loading_bar(i + 1, SAMPLE_ITERS, "Sample phase", i + 1 == SAMPLE_ITERS);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    printf("Best Prediction from core %d with EV=%lf\n", rank, best_EV);
    //Print best pick
    for(int i=0;i<NUM_GAMES;i++)
        printf("Game %d: %d\n", i+1, best_y[i]);
    MPI_Finalize();
    free(sample_space);
    free(pobin_lut);
    return 0;
}
