//
// Created by gianluca on 20/02/17.
//

#ifndef DEEPSPACE_TURBO_UTILITIES_H
#define DEEPSPACE_TURBO_UTILITIES_H

#include<stdio.h>
void print_array_int(int *array, int length);

void print_array(double *array, int length);

double* randn(double mean, double variance, unsigned int length);

int* randbits(unsigned int length);

double* linspace(double start, double end, unsigned int size);

double* add_arrays(double *one, double *two, unsigned int length);

double mean(double *input, unsigned int length);

double *scalar_multiply(double *array, unsigned int length, double scalar);

int save_data(double *x, double *y, char *header[], unsigned int length, FILE *file);

double max_array(double *array, int size);

#endif //DEEPSPACE_TURBO_UTILITIES_H
