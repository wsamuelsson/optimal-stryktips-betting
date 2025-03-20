#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <fftw3.h>

double fft_convolution(double kernel[], int kernellen, double signal[], int signallen, double result[]);
double direct_convolution_local(double probs[], int probslen, double result[]);
void direct_convolution(double probs[], int *probslen, double result[]);
void tree_convolution(double p_vec[], int *p_vec_len, int *num_splits, double result_vec[]);
