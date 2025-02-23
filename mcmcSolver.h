#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<string.h>
#include<math.h>
#include"pobinfft.h"
#include<assert.h>


#define ALPHA 0.95            // Cooling rate
#define T_INIT 100.0          // Initial temperature
#define T_MIN 1e-3            // Minimum temperature

#ifndef _MCMC_SOLVER
#define _MCMC_SOLVER

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

double simulated_annealing(int *y, int *best_y, int *sample_space, double *pobin_lut, double *picking_probs, 
                         double *implied_probs, int pool_size, int num_games, int burnin_iters, int sample_iters, int rank, int size, int total_turnover, int jackpot);

double metropolis_hasting(int *y, int *best_y, int *sample_space, double *pobin_lut, double *picking_probs, 
                         double *implied_probs, int pool_size, int num_games, int burnin_iters, int sample_iters, int rank, int size, int total_turnover, int jackpot);
void print_odds_and_probs(odds_t *odds, probs_t *probs, int num_games);
static inline double get_implied_prob(int *pick, double *implied_prob, int num_games);
int check_overlap(int *pick_a, int *pick_b, int num_games);
inline double compute_conditional_expectation(int *x, int *y, double *result_likliehood, int pool_size, int num_games, int total_turnover, int jackpot);
inline double compute_expected_value(int *y, int* sample_space, double *pobin_lut, double *picking_probs, double *implied_probs, int pool_size, int num_games, int total_turnover, int jackpot);
void print_loading_bar(int current, int total, const char *label, int finalize);


double metropolis_hasting(int *y, int *best_y, int *sample_space, double *pobin_lut, double *picking_probs, 
                         double *implied_probs, int pool_size, int num_games, int burnin_iters, int sample_iters, int rank, int size, int total_turnover, int jackpot){

    double best_EV;
    double EV;
    best_EV = compute_expected_value(y, sample_space, pobin_lut,picking_probs, implied_probs, pool_size, num_games, total_turnover, jackpot);
    
    if(rank==0)
        printf("Starting MCMC on %d chains. Running %d (%d burn-in phase + %d sample phase) samples\n", size, size*(burnin_iters+sample_iters), size*burnin_iters, size*sample_iters);
    
    
    double alpha; 
    for(int i=0;i<burnin_iters;i++){

        //Generate propasal y from uniform distribution
        for(int j=0; j<num_games;j++)
            y[j] = rand() % 3;
        
        //Compute EV
        EV = compute_expected_value(y, sample_space, pobin_lut, picking_probs, implied_probs, pool_size, num_games, total_turnover, jackpot);
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
        EV = compute_expected_value(y, sample_space, pobin_lut, picking_probs, implied_probs, pool_size, num_games, total_turnover, jackpot);
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
                         double *implied_probs, int pool_size, int num_games, int burnin_iters, int sample_iters, int rank, int size, int total_turnover, int jackpot) {

    double best_EV, new_EV;
    double temperature = T_INIT;
    double alpha;
    
    best_EV = compute_expected_value(y, sample_space, pobin_lut, picking_probs, implied_probs, pool_size, num_games, total_turnover, jackpot);
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
        new_EV = compute_expected_value(proposal, sample_space, pobin_lut, picking_probs, implied_probs, pool_size, num_games, total_turnover, jackpot);
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
        new_EV = compute_expected_value(proposal, sample_space, pobin_lut, picking_probs, implied_probs, pool_size, num_games, total_turnover, jackpot);
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
 

double compute_expected_value(int *y, int* sample_space, double *pobin_lut, double *picking_probs, double *implied_probs, int pool_size, int num_games, int total_turnover, int jackpot){
    double EV_cond = 0.0;
    int *x;
    double implied_prob_x;
    double EV = 0.0;
    const int sample_space_size = pow(3,num_games);
    double *result_likliehood;

    for(int i = 0; i < sample_space_size;i++){
        //Read the bet 
        x = &sample_space[i*num_games];
        
        //Read the likliehoods fom LUT
        result_likliehood = &pobin_lut[i*(num_games)];
        //Compute E(Y|X)
        EV_cond = compute_conditional_expectation(x, y, result_likliehood, pool_size, num_games, total_turnover, jackpot);
        //Compute A(X)
        implied_prob_x = get_implied_prob(x, implied_probs, num_games);
        //E(Y) = sum_x A(X) * E(Y|X)
        EV += (EV_cond*implied_prob_x);
      
    }
    return EV;
}



double compute_conditional_payoff(int *other_winners, int score, int num_games, int total_turnover, int jackpot){
    double Q = 0.0;
    assert(score >= 0);

    if(num_games == 13){
        //Sole winner - Jackpott !!!
        if((score == num_games) && (other_winners[num_games] == 0)){
            return jackpot;
        }
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
    }

    if(num_games == 8){
        if (score == 8)
        {
            Q = 0.7;
        }
        
    }


    return Q * total_turnover / (other_winners[score] + 1);
}

double compute_conditional_expectation(int *x, int *y, double *result_likliehood, int pool_size, int num_games, int total_turnover, int jackpot){
    
    int score = check_overlap(x, y, num_games);
    //We just need to compute probabiliteis that opponents score: 13, 12, 11, 10 -> p_score*pool_size gives proportion

    double p_13 = result_likliehood[13];
    double p_12 = result_likliehood[12];
    double p_11 = result_likliehood[11];
    double p_10 = result_likliehood[10];
    double p_8  = result_likliehood[8];
    
    
    int other_winners[14] = {0};
    other_winners[13]  = (int) (p_13 * pool_size);
    other_winners[12]  = (int) (p_12 * pool_size);
    other_winners[11]  = (int) (p_11 * pool_size);
    other_winners[10]  = (int) (p_10 * pool_size);

    if(num_games == 8)
        other_winners[8] = (int) (p_8 * pool_size);
    
    double EV_cond = compute_conditional_payoff(&other_winners[0], score,  num_games, total_turnover, jackpot);
    
    return EV_cond;
}

void print_odds_and_probs(odds_t *odds, probs_t *probs, int num_games){
    for(int i = 0; i < num_games;i++){
        printf("Game %d\nH:%f\nD:%f\nA:%f\n", i+1, probs[i].home_prob, probs[i].draw_prob, probs[i].away_prob);
    }
    for(int i = 0; i < num_games;i++){
        printf("Game %d\nH:%f\nD:%f\nA:%f\n", i+1, odds[i].home_odds, odds[i].draw_odds, odds[i].away_odds);
    }
}


double get_implied_prob(int *pick, double *implied_prob, int num_games){
    double p = 1.0;
    for(int i=0; i<num_games; i++){
        p *= 1.0/implied_prob[pick[i] + 3*i];
        
    }
    return p;
}


int check_overlap(int *pick_a, int *pick_b, int num_games){
    int overlap = 0;
    for(int i=0; i<num_games;i++) {
        overlap += (int)(pick_a[i] == pick_b[i]); //If equal +1 else +0;
    }
    return overlap;
}

void print_loading_bar(int current, int total, const char *label, int finalize) {
    int bar_width = 50;
    double progress = (double)current / total;
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