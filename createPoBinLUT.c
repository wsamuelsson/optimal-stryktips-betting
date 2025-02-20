#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<string.h>
#include<math.h>
#include"pobinfft.h"
#include<assert.h>


#define SAMPLE_SPACE_SIZE  1594323
#define NUM_GAMES 13

void compute_likliehood(int *x, const double *picking_probs, double result_likliehood[]);


typedef struct {
    double home_prob;
    double draw_prob;
    double away_prob;
} probs_t;


int main(int argc, char const *argv[])
{
    const char *filename_sample_space = "PossibleBets.bin";
    const char *filename_pobin_lut = "PoBinLUT.bin";

    int *sample_space = (int *)malloc(sizeof(int)*SAMPLE_SPACE_SIZE*NUM_GAMES);
    double *pobin_lut = (double *)malloc(sizeof(double)*SAMPLE_SPACE_SIZE*(NUM_GAMES+1));
    
    
    FILE * file_sample_space = fopen(filename_sample_space, "rb");
    
    if (file_sample_space == NULL){
        printf("Error opening file\n");
        return 1;
    }
    
    //Read in possible bets
    int bytesRead = fread(&sample_space[0], sizeof(int), SAMPLE_SPACE_SIZE * NUM_GAMES, file_sample_space);
    
    if(bytesRead != (SAMPLE_SPACE_SIZE*NUM_GAMES)){
        perror("Error reading\n");
        return 1;
    }
    if (fclose(file_sample_space) != 0){
        fprintf(stderr, "Error while closing file\n");
        return 1;
    }
    
    double picking_probs[3*NUM_GAMES];
    

    // Read probs
    probs_t probs[NUM_GAMES];
    FILE * file_probs = fopen("probs.bin", "rb");
    int num_obj_read = fread(probs, sizeof(probs_t), NUM_GAMES, file_probs);
    fclose(file_probs);
    if (num_obj_read != NUM_GAMES){
        printf("Something went wrong reading probs.\n");
        return -1;
    }
    memcpy(&picking_probs[0], &probs[0], NUM_GAMES*3*sizeof(double));    
    int *x;
    double result_likliehood[NUM_GAMES+1];
    double ps_arg[NUM_GAMES*3];
    memcpy(&ps_arg[0], &picking_probs[0], NUM_GAMES*3*sizeof(double));
    
    for(int i=0; i<SAMPLE_SPACE_SIZE;i++){
        x = &sample_space[i*NUM_GAMES];
        
        compute_likliehood(&x[0], &ps_arg[0], result_likliehood);
        
        memcpy(&pobin_lut[i*NUM_GAMES], result_likliehood, (NUM_GAMES+1)*sizeof(double));
        if(i==0){
            
            for(int j=0;j<NUM_GAMES;j++){
                printf("Game %d: %lf, index=%d\n", j+1, ps_arg[x[j] + j*3], x[j]+j*3);
            }
        }
    }

   // Write resulting lookup table to file
    FILE *f_write = fopen(filename_pobin_lut, "wb");

    // Check if the file was successfully opened
    if (f_write == NULL) {
        fprintf(stderr, "Error opening file %s for writing\n", filename_pobin_lut);
        return 1; // Return an error code
    }

    // Attempt to write the lookup table to the file
    size_t items_written = fwrite(pobin_lut, sizeof(double), (NUM_GAMES + 1) * SAMPLE_SPACE_SIZE, f_write);

    // Check if the correct number of items were written
    if (items_written != (NUM_GAMES + 1) * SAMPLE_SPACE_SIZE) {
        fprintf(stderr, "Error writing to file %s\n", filename_pobin_lut);
        fclose(f_write); // Close the file before returning
        return 1; // Return an error code
    }

    // Attempt to close the file
    if (fclose(f_write) != 0) {
        fprintf(stderr, "Error closing file %s\n", filename_pobin_lut);
        return 1; // Return an error code
    }
    free(pobin_lut);
    free(sample_space);
    return 0;
}

void compute_likliehood(int *x, const double *picking_probs, double result_likliehood[]){
    double ps[NUM_GAMES];
    for(int i=0;i < NUM_GAMES;i++){
        ps[i] = picking_probs[3*i + x[i]];
    }
    
    double ps_arg[NUM_GAMES];
    memcpy(ps_arg, ps, NUM_GAMES*sizeof(double));
    
    int number_splits = 4;
    int p_vec_len = NUM_GAMES;
    
    //Compute PoBin probability mass
    
    tree_convolution(ps_arg, &p_vec_len, &number_splits, result_likliehood);
    
    double psum = 0.0;
    for(int i = 0; i < NUM_GAMES+1;i++)
        psum += result_likliehood[i];
    
    double diff = fabs(psum - 1.0);
    assert(diff < 1.0e-9);
    return;
}

