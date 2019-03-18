//
// Created by gianluca on 22/02/17.
//

#ifndef DEEPSPACE_TURBO_LIBTURBOCODES_H
#define DEEPSPACE_TURBO_LIBTURBOCODES_H

#include "libconvcodes.h"

typedef struct str_turbocode{
    t_convcode *upper_code;
    t_convcode *lower_code;

    int *interleaver;
    int packet_length;
    int encoded_length;
} t_turbocode;

int *turbo_interleave(int *packet, t_turbocode *code);
int *turbo_deinterleave(int *packet, t_turbocode *code);
void message_interleave(double ***messages, t_turbocode *code);
void message_deinterleave(double ***messages, t_turbocode *code);
t_turbocode *turbo_initialize(t_convcode *upper, t_convcode *lower, int *interleaver, int packet_length);
void *turbocode_clear(t_turbocode *code);

int *turbo_encode(int *packet, t_turbocode *code);
int *turbo_decode(double* received, int iterations, double noise_variance, t_turbocode *code);

#endif //DEEPSPACE_TURBO_LIBTURBOCODES_H
