//
// Created by gianluca on 22/02/17.
//

#ifndef DEEPSPACE_TURBO_LIBTURBOCODES_H
#define DEEPSPACE_TURBO_LIBTURBOCODES_H

#include "libconvcodes.h"

typedef struct str_turbocode{
    t_convcode *inner_codes;
    int components;
    int **interleaving_vectors;
    int packet_length;
    int encoded_length;
} t_turbocode;

int **turbo_interleave(int *packet, t_turbocode code);

int *turbo_encode(int *packet, t_turbocode code);

t_turbocode turbo_initialize(t_convcode *codes, int components, int **interleaver, int packet_length);

int *turbo_decode(double* received, t_turbocode code);

#endif //DEEPSPACE_TURBO_LIBTURBOCODES_H
