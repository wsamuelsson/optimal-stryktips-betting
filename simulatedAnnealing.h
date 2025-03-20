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

#ifndef _SA_SOLVER
#define _SA_SOLVER

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

void print_odds_and_probs(odds_t *odds, probs_t *probs, int num_games);
double get_implied_prob(int *pick, double *implied_prob, int num_games);
int check_overlap(int *pick_a, int *pick_b, int num_games);
double compute_conditional_expectation(int *x, int *y, double *result_likliehood, int pool_size, int num_games, int total_turnover, int jackpot);
double compute_expected_value(int *y, int* sample_space, double *pobin_lut, double *picking_probs, double *implied_probs, int pool_size, int num_games, int total_turnover, int jackpot);
void print_loading_bar(int current, int total, const char *label, int finalize);
int estimate_pareto_pool_size(int total_turnover);


#endif