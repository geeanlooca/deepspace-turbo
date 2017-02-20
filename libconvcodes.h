//
// Created by gianluca on 20/02/17.
//

#ifndef DEEPSPACE_TURBO_LIBCONVCODES_H
#define DEEPSPACE_TURBO_LIBCONVCODES_H
#define MAX_STATES 100

typedef struct str_neighbor{
    int state;
    int input;
} t_neigh;

typedef struct str_convcode{
    int memory;
    int (*state_update_function)(int , double);
    void (*output_function)(int, double, int*);
    double (*termination_function)(int);
    int n; //inverse of the rate for rate 1/n
    int neighbors[MAX_STATES][MAX_STATES][2];

} t_convcode;

static int index3d(int i, int j, int k, int sizex, int sizey, int sizez);

int* conv_encode(double* input, unsigned int length, t_convcode code);

int* conv_decode(double* input, unsigned int length, t_convcode code);

#endif //DEEPSPACE_TURBO_LIBCONVCODES_H
