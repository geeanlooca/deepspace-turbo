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
    int **memory_block;
} t_turbocode;

void turbo_interleave(int *packet, int *length, t_turbocode code);
t_turbocode turbo_initialize(int components, t_convcode *codes, int **interleaver, int packet_length);

#endif //DEEPSPACE_TURBO_LIBTURBOCODES_H
