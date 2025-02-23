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
    //13 is max num of games - doing this we use some stack memory
    int num_games = 13;
    int num_obj_read;
    int jackpot = (int)10e6;
    int total_turnover;
    
    FILE *file_odds;
    FILE *file_probs;
    FILE *file_num_games;
    FILE *file_total_turnover;
    
    

    probs_t probs[num_games];
    odds_t odds[num_games];

    double picking_probs[3*num_games];
    double implied_probs[3*num_games];
    
    //Read number of games
    file_num_games = fopen("data/num_games.bin", "rb");
    num_obj_read   = fread(&num_games, sizeof(int), 1, file_num_games);
    fclose(file_num_games);
    if(num_obj_read != 1){
        MPI_Finalize();
        printf("Something went wrong reading num games. \n");
        return -1;
    }

    //Read total_turnover
    file_total_turnover = fopen("data/total_turnover.bin", "rb");
    num_obj_read   = fread(&total_turnover, sizeof(int), 1, file_total_turnover);
    fclose(file_total_turnover);
    if(num_obj_read != 1){
        MPI_Finalize();
        printf("Something went wrong reading num games. \n");
        return -1;
    }
    //Should think about some good way to compute this
    int pool_size = (int)(total_turnover) / 50;
    
    
    if (rank == 0) {
        printf("------------------------------------------------\n");
        printf("WELCOME TO THE SIMULATION\n");
        if(num_games == 13){
            if(jackpot == (int)10e6)
                printf("We are pricing Stryktippset with turnover %d kr jackpot %d kr\n", total_turnover, (int)10e6);
            else{
                printf("We are pricing Europatippset with turnover %d kr and jackpot %d kr\n", total_turnover, (int)4e6);
            }
        }
        if(num_games == 8)
            printf("We are pricing Topptippset with turnover %d kr\n", total_turnover);
        printf("------------------------------------------------\n");
        
    }

    
    // Read prob
    file_probs = fopen("data/probs.bin", "rb");
    num_obj_read = fread(probs, sizeof(probs_t), num_games, file_probs);
    fclose(file_probs);
    if (num_obj_read != num_games){
        MPI_Finalize();
        printf("Something went wrong reading probs.\n");
        return -1;
    }

    // Read odds
    file_odds = fopen("data/odds.bin", "rb");
    num_obj_read = fread(odds, sizeof(odds_t), num_games, file_odds);
    fclose(file_odds);
    if (num_obj_read != num_games){
        MPI_Finalize();
        printf("Something went wrong reading odds.\n");
        return -1;
    }
    
    // Implied probs and picking probs
    memcpy(&implied_probs[0], &odds[0], num_games * 3 * sizeof(double));
    memcpy(&picking_probs[0], &probs[0], num_games * 3 * sizeof(double));
    
    const int sample_space_size = pow(3,num_games);
    int *sample_space = (int *)malloc(sample_space_size * num_games * sizeof(int));
    
    // Read the bets into sample space
    const char *filename_bets = "data/PossibleBets.bin";

    FILE *file_bets = fopen(filename_bets, "rb");
    if (file_bets == NULL){
        printf("Error opening file\n");
        return 1;
    }
    
    int bytesRead = fread(&sample_space[0], sizeof(int), sample_space_size * num_games, file_bets);
    if(bytesRead != (sample_space_size*num_games)){
        perror("Error reading\n");
        return 1;
    }
    if (fclose(file_bets) != 0){
        fprintf(stderr, "Error while closing file\n");
        return 1;
    }

    //Read in the PoBin LUT
    FILE * file_pobin = fopen("data/PoBinLUT.bin", "rb");
    double *pobin_lut = (double *)malloc(sizeof(double)*sample_space_size*(num_games+1));
    size_t read_bytes = fread(pobin_lut, sizeof(double), sample_space_size*(num_games+1), file_pobin);
    assert(read_bytes);
    
    fclose(file_pobin);

    int BURNIN_ITERS = 10000;
    int SAMPLE_ITERS = 10000;
    int y[] = {1, 2, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0};

    //Generate propasal y from uniform distribution
        for(int j=0; j<num_games;j++)
            y[j] = rand() % 3;
    
    int best_y[num_games];
    memcpy(best_y, y, num_games*sizeof(int));   
   
    MPI_Barrier(MPI_COMM_WORLD);

    double best_EV = simulated_annealing(&y[0], &best_y[0], &sample_space[0], &pobin_lut[0], &picking_probs[0], 
    &implied_probs[0], pool_size, num_games, BURNIN_ITERS, SAMPLE_ITERS, rank, size, total_turnover, jackpot);
    
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
                printf("EVs are different <-> Chains diverged. \n");
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
        for (int i = 0; i < num_games; i++) {
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
