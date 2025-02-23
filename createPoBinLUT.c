#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<string.h>
#include<math.h>
#include"pobinfft.h"
#include<assert.h>


void compute_likliehood(int *x, const double *picking_probs, int num_games, double result_likliehood[]);


typedef struct {
    double home_prob;
    double draw_prob;
    double away_prob;
} probs_t;


int main(int argc, char const *argv[])
{
    const char *filename_sample_space = "data/PossibleBets.bin";
    const char *filename_pobin_lut = "data/PoBinLUT.bin";
    const char *filename_num_games = "data/num_games.bin";    
    

    FILE *file_num_games;
    

    int num_games, num_obj_read;

    //Read number of games
    file_num_games = fopen(filename_num_games, "rb");
    num_obj_read   = fread(&num_games, sizeof(int), 1, file_num_games);
    fclose(file_num_games);
    if(num_obj_read != 1){
        printf("Something went wrong reading num games. \n");
        return -1;
    }

    const int sample_space_size = pow(3, num_games);


    int *sample_space = (int *)malloc(sizeof(int)*sample_space_size*num_games);
    double *pobin_lut = (double *)malloc(sizeof(double)*sample_space_size*(num_games+1));
    
    
    FILE * file_sample_space = fopen(filename_sample_space, "rb");
    
    if (file_sample_space == NULL){
        printf("Error opening file\n");
        return 1;
    }
    
    //Read in possible bets
    int bytesRead = fread(&sample_space[0], sizeof(int), sample_space_size * num_games, file_sample_space);
    
    if(bytesRead != (sample_space_size*num_games)){
        perror("Error reading\n");
        return 1;
    }
    if (fclose(file_sample_space) != 0){
        fprintf(stderr, "Error while closing file\n");
        return 1;
    }
    
    double picking_probs[3*num_games];
    

    // Read probs
    probs_t probs[num_games];
    FILE * file_probs = fopen("probs.bin", "rb");
    num_obj_read = fread(probs, sizeof(probs_t), num_games, file_probs);
    fclose(file_probs);
    if (num_obj_read != num_games){
        printf("Something went wrong reading probs.\n");
        return -1;
    }
    memcpy(&picking_probs[0], &probs[0], num_games*3*sizeof(double));    
    int *x;
    double result_likliehood[num_games+1];
    double ps_arg[num_games*3];
    memcpy(&ps_arg[0], &picking_probs[0], num_games*3*sizeof(double));
    
    for(int i=0; i<sample_space_size;i++){
        x = &sample_space[i*num_games];
        
        compute_likliehood(&x[0], &ps_arg[0], num_games,result_likliehood);
        
        memcpy(&pobin_lut[i*num_games], result_likliehood, (num_games+1)*sizeof(double));
        if(i==0){
            
            for(int j=0;j<num_games;j++){
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
    size_t items_written = fwrite(pobin_lut, sizeof(double), (num_games + 1) * sample_space_size, f_write);

    // Check if the correct number of items were written
    if (items_written != (num_games + 1) * sample_space_size) {
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

void compute_likliehood(int *x, const double *picking_probs, int num_games, double result_likliehood[]){
    double ps[num_games];
    for(int i=0;i < num_games;i++){
        ps[i] = picking_probs[3*i + x[i]];
    }
    
    double ps_arg[num_games];
    memcpy(ps_arg, ps, num_games*sizeof(double));
    
    int number_splits = 4;
    int p_vec_len = num_games;
    
    //Compute PoBin probability mass
    
    tree_convolution(ps_arg, &p_vec_len, &number_splits, result_likliehood);
    
    double psum = 0.0;
    for(int i = 0; i < num_games+1;i++)
        psum += result_likliehood[i];
    
    double diff = fabs(psum - 1.0);
    assert(diff < 1.0e-9);
    return;
}

