//
// Created by gianluca on 20/02/17.
//

#include "utilities.h"
#include <stdlib.h>
#include <math.h>


void print_array_int(int *array, int length)
{
    for (int i = 0; i < length; i++)/*{{{*/
    {
        printf("array[%d] = %d\n", i, array[i]);
    }/*}}}*/
}

void print_array(double *array, int length)
{
    for (int i = 0; i < length; i++)/*{{{*/
    {
        printf("array[%d] = %f\n", i, array[i]);
    }/*}}}*/
}

double* randn(double mean, double variance, unsigned int length)
{
    /*{{{*/
    double* random =  malloc(length * sizeof *random);

    for (int i = 0; i < length; i++)
    {
        double U1 = ((double)rand()/(double)RAND_MAX);
        double U2 = ((double)rand()/(double)RAND_MAX);

        double R = sqrt(-2*variance*log(U1));
        double theta = 2*M_PI*U2;

        random[i] = mean + R*cos(theta);;
    }

    return random;/*}}}*/
}

int* randbits(unsigned int length)
{
    int *seq = malloc(length*sizeof *seq);/*{{{*/
    for (int i = 0; i < length; i++)
    {
        seq[i] = rand() % 2;
    }

    return seq;/*}}}*/
}

double* linspace(double start, double end, unsigned int size)
{
    double *array =  malloc(size * sizeof *array);/*{{{*/
    double delta = (double)(end - start)/(size - 1);

    double cumulative = start;
    for (int i = 0; i < size; i++)
    {
        array[i] = cumulative;
        cumulative += delta;
    }

    array[size-1] = end;

    return array;/*}}}*/
}

double* add_arrays(double *one, double *two, unsigned int length)
{
    double *sum = malloc(length*sizeof *sum);/*{{{*/
    for (int i = 0; i < length; i++)
    {
        sum[i] = one[i] + two[i];
    }

    return sum;/*}}}*/
}

double mean(double *input, unsigned int length)
{
    double total = 0;/*{{{*/
    for (int i = 0; i < length; i++)
    {
        total += input[i];
    }

    return total/length;/*}}}*/
}


double *scalar_multiply(double *array, unsigned int length, double scalar)
{
    double *res = malloc(length*sizeof *res);/*{{{*/
    for (int i = 0; i < length; i++)
    {
        res[i] = array[i]*scalar;
    }

    return res;/*}}}*/
}

int save_data(double *x, double *y, double *z, char *header[], unsigned int length, FILE *file)
{
    // write headers/*{{{*/
    fprintf(file, "%s,%s\n", header[0], header[1]);
    for (int i = 0; i < length; i++)
        fprintf(file, "%.12f,%.12f,%.12f\n", x[i], y[i], z[i]);/*}}}*/
}

double max_array(double *array, int size)
{
    double max = array[0];
    for (int i = 0; i < size; ++i)
        max = (array[i] > max) ? array[i] : max;

    return max;
}
