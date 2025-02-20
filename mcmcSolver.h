#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<string.h>
#include<math.h>
#include"pobinfft.h"
#include<assert.h>

#define SAMPLE_SPACE_SIZE  1594323
#define READ_NUMBER SAMPLE_SPACE_SIZE
#define ALPHA 0.95            // Cooling rate
#define T_INIT 100.0          // Initial temperature
#define T_MIN 1e-3            // Minimum temperature

#ifndef BETS_H
#define BETS_H

typedef struct {
    double home_odds;
    double draw_odds;
    double away_odds;

} odds_t;

typedef struct {
    double home_prob;
    double draw_prob;
    double away_prob;
} probs_t;


void print_odds_and_probs(odds_t *odds, probs_t *probs, int NUM_GAMES);
static inline double get_implied_prob(int *pick, double *implied_prob, int NUM_GAMES);
inline double get_picking_prob(int *pick, double *picking_probs, int NUM_GAMES);
int check_overlap(int *pick_a, int *pick_b, int NUM_GAMES);
double compute_likliehood(int *x, int s, double *picking_probs, double result_lilkliehood[], int NUM_GAMES);
inline void compute_conditional_expectation(double *EV_cond, int *x, int *y, double *result_likliehood, int POOL_SIZE, int NUM_GAMES);
inline double compute_expected_value(int *y, int* sample_space, double *pobin_lut, double *picking_probs, double *implied_probs, int POOL_SIZE, int NUM_GAMES);
void print_loading_bar(int current, int total, const char *label, int finalize);


double metropolis_hasting(int *y, int *best_y, int *sample_space, double *pobin_lut, double *picking_probs, 
                         double *implied_probs, int pool_size, int num_games, int burnin_iters, int sample_iters, int rank, int size){

    double best_EV;
    double EV;
    best_EV = compute_expected_value(y, sample_space, pobin_lut,picking_probs, implied_probs, pool_size, num_games);
    
    if(rank==0)
        printf("Starting MCMC on %d chains. Running %d (%d burn-in phase + %d sample phase) samples\n", size, size*(burnin_iters+sample_iters), size*burnin_iters, size*sample_iters);
    
    
    double alpha; 
    for(int i=0;i<burnin_iters;i++){

        //Generate propasal y from uniform distribution
        for(int j=0; j<num_games;j++)
            y[j] = rand() % 3;
        
        //Compute EV
        EV = compute_expected_value(y, sample_space, pobin_lut, picking_probs, implied_probs, pool_size, num_games);
        alpha = (double)rand() / (RAND_MAX + 1.0);
        if(EV > best_EV){
            best_EV = EV;
            memcpy(best_y, y, num_games*sizeof(int));
        }
        else if(alpha > EV/best_EV){
            best_EV = EV;
            memcpy(best_y, y, num_games*sizeof(int));
        }
        if(rank==0)
            print_loading_bar(i + 1, burnin_iters, "Burn-in phase", i + 1 == burnin_iters);
    }

    for(int i=0;i<sample_iters;i++){

        //Generate propasal y from uniform distribution
        for(int j=0; j<num_games;j++)
            y[j] = rand() % 3;
        
        //Compute EV
        EV = compute_expected_value(y, sample_space, pobin_lut, picking_probs, implied_probs, pool_size, num_games);
        alpha = (double)rand() / (RAND_MAX + 1.0);
        if(EV > best_EV){
            best_EV = EV;
            memcpy(best_y, y, num_games*sizeof(int));
        }
        else if(alpha > EV/best_EV){
            best_EV = EV;
            memcpy(best_y, y, num_games*sizeof(int));
        }
        if(rank==0)
            print_loading_bar(i + 1, sample_iters, "Sample phase", i + 1 == sample_iters);
    }
    
    return best_EV;
}


double simulated_annealing(int *y, int *best_y, int *sample_space, double *pobin_lut, double *picking_probs, 
                         double *implied_probs, int pool_size, int num_games, int burnin_iters, int sample_iters, int rank, int size) {

    double best_EV, new_EV;
    double temperature = T_INIT;
    double alpha;
    
    best_EV = compute_expected_value(y, sample_space, pobin_lut, picking_probs, implied_probs, pool_size, num_games);
    memcpy(best_y, y, num_games * sizeof(int));

    if (rank == 0) {
        printf("Starting Simulated Annealing on %d chains. Running %d iterations (%d burn-in, %d sample phase)\n", 
               size, size * (burnin_iters + sample_iters), size * burnin_iters, size * sample_iters);
    }

    // BURN-IN PHASE
    for (int i = 0; i < burnin_iters; i++) {
        // Generate proposal y from a uniform distribution
        int proposal[num_games];
        for (int j = 0; j < num_games; j++)
            proposal[j] = rand() % 3;

        // Compute EV
        new_EV = compute_expected_value(proposal, sample_space, pobin_lut, picking_probs, implied_probs, pool_size, num_games);
        alpha = (double)rand() / RAND_MAX;

        // Metropolis criterion for maximization
        if (new_EV > best_EV || alpha < exp((new_EV - best_EV) / temperature)) {
            best_EV = new_EV;
            memcpy(y, proposal, num_games * sizeof(int));
            memcpy(best_y, y, num_games * sizeof(int));
        }

        // Cooling schedule update
        temperature *= ALPHA;
        if (temperature < T_MIN) temperature = T_MIN;

        if (rank == 0)
            print_loading_bar(i + 1, burnin_iters, "Burn-in phase", i + 1 == burnin_iters);
    }

    // SAMPLE PHASE
    for (int i = 0; i < sample_iters; i++) {
        // Generate proposal y
        int proposal[num_games];
        for (int j = 0; j < num_games; j++)
            proposal[j] = rand() % 3;

        // Compute EV
        new_EV = compute_expected_value(proposal, sample_space, pobin_lut, picking_probs, implied_probs, pool_size, num_games);
        alpha = (double)rand() / RAND_MAX;

        // Metropolis criterion for maximization
        if (new_EV > best_EV || alpha < exp((new_EV - best_EV) / temperature)) {
            best_EV = new_EV;
            memcpy(y, proposal, num_games * sizeof(int));
            memcpy(best_y, y, num_games * sizeof(int));
        }

        // Cooling schedule update
        temperature *= ALPHA;
        if (temperature < T_MIN) temperature = T_MIN;

        if (rank == 0)
            print_loading_bar(i + 1, sample_iters, "Sample phase", i + 1 == sample_iters);
    }
    return best_EV;
}
 

double compute_expected_value(int *y, int* sample_space, double *pobin_lut, double *picking_probs, double *implied_probs, int POOL_SIZE, int NUM_GAMES){
    double conditional_E = 0.0;
    int *x;
    double implied_prob_x;
    double a = 1.0/SAMPLE_SPACE_SIZE;
    double EV = 0.0;
    
    double *result_likliehood;
    for(int i = 0; i < READ_NUMBER;i++){
        //Read the bet 
        x = &sample_space[i*13];
        
        //Read the likliehoods fom LUT
        result_likliehood = &pobin_lut[i*(NUM_GAMES)];
        //Compute E(Y|X)
        compute_conditional_expectation(&conditional_E, x, y, result_likliehood, POOL_SIZE, NUM_GAMES);
        //Compute A(X)
        implied_prob_x = get_implied_prob(x, implied_probs, NUM_GAMES);
        //E(Y) = sum_x A(X) * E(Y|X)
        EV += (a*conditional_E*implied_prob_x);
        
        
    }
    return EV;
}

double compute_likliehood(int *x, int s, double *picking_probs, double result_likliehood[], const int NUM_GAMES){
    double ps[NUM_GAMES];
    for(int i=0;i < NUM_GAMES;i++){
        ps[i] = picking_probs[3*i + x[i]];
    }

    
    int number_splits = 4;
    int p_vec_len = NUM_GAMES;
    
    //Compute PoBin probability mass
    tree_convolution(ps, &p_vec_len, &number_splits, result_likliehood);
    
    return 0.0;
}

float compute_conditional_payoff(int *other_winners, int score, int total_turnover){
    double Q = 0.0;
    assert(score >= 0);
    switch (score) {
        case 13:
            Q = 0.4;
            break;
        case 12:
            Q = 0.24;
            break;
        case 11:
            Q = 0.20;
            break;
        case 10:
            Q = 0.16;
            break;
        default:
            return 0.0;
    }

    return Q * total_turnover / (other_winners[score] + 1);
}

void compute_conditional_expectation(double * EV_cond, int *x, int *y, double *result_likliehood, int POOL_SIZE, int NUM_GAMES){
    
    int score = check_overlap(x, y, NUM_GAMES);
    //We just need to compute probabiliteis that opponents score: 13, 12, 11, 10 -> p_score*pool_size gives proportion

    double p_13 = result_likliehood[13];
    double p_12 = result_likliehood[12];
    double p_11 = result_likliehood[11];
    double p_10 = result_likliehood[10];

    int other_winners[14] = {0};
    other_winners[13]  = (int) (p_13 * POOL_SIZE);
    other_winners[12]  = (int) (p_12 * POOL_SIZE);
    other_winners[11]  = (int) (p_11 * POOL_SIZE);
    other_winners[10]  = (int) (p_10 * POOL_SIZE);

    *EV_cond = compute_conditional_payoff(&other_winners[0], score, POOL_SIZE);
    return;
}

void print_odds_and_probs(odds_t *odds, probs_t *probs, int NUM_GAMES){
    for(int i = 0; i < NUM_GAMES;i++){
        printf("Game %d\nH:%f\nD:%f\nA:%f\n", i+1, probs[i].home_prob, probs[i].draw_prob, probs[i].away_prob);
    }
    for(int i = 0; i < NUM_GAMES;i++){
        printf("Game %d\nH:%f\nD:%f\nA:%f\n", i+1, odds[i].home_odds, odds[i].draw_odds, odds[i].away_odds);
    }
}


double get_implied_prob(int *pick, double *implied_prob, int NUM_GAMES){
    double implied_prob_sum = 1.0;
    for(int i=0; i<NUM_GAMES; i++){
        implied_prob_sum += implied_prob[pick[i] + 3*i];
        
    }
    return implied_prob_sum;
}

double get_picking_prob(int *pick, double *picking_probs_log, int NUM_GAMES){
    double picking_probs_sum = 0.0;
    for(int i=0;i<NUM_GAMES;i++){
        picking_probs_sum +=  picking_probs_log[pick[i] + 3*i];
        
    }
    
    return picking_probs_sum;
}

int check_overlap(int *pick_a, int *pick_b, int NUM_GAMES){
    int overlap = 0;
    for(int i=0; i<NUM_GAMES;i++) {
        overlap += (int)(pick_a[i] == pick_b[i]); //If equal +1 else +0;
    }
    return overlap;
}

void print_loading_bar(int current, int total, const char *label, int finalize) {
    int bar_width = 50;
    float progress = (float)current / total;
    int pos = bar_width * progress;

    // Print the label with fixed width
    printf("%-15s [", label); // Adjust the width to align the labels
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) {
            printf("=");
        } else if (i == pos) {
            printf(">");
        } else {
            printf(" ");
        }
    }
    printf("] %3d%%\r", (int)(progress * 100));
    fflush(stdout);

    if (finalize) {
        printf("\n");
    }
}
#endif