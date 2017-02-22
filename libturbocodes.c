//
// Created by gianluca on 22/02/17.
//

#include "libturbocodes.h"
#include<stdlib.h>

void turbo_interleave(int *packet, int *length, t_turbocode code)
{
    int components = code.components;
    for (int i = 0; i < code.components; ++i) {
        for (int j = 0; j < code.packet_length; ++j) {
            code.memory_block[i][j] = packet[code.interleaving_vectors[i][j]];
        }
    }
}


t_turbocode turbo_initialize(int components, t_convcode *codes, int **interleaver, int packet_length)
{
    t_turbocode code;
    code.components = components;
    code.inner_codes = codes;
    code.packet_length = packet_length;
    code.interleaving_vectors = interleaver;

    // allocate memory block to store messages to feed to the convolutional encoders
    int **interleaved_messages = malloc(code.components * sizeof(int*));
    for (int i = 0; i < code.components; ++i) {
        interleaved_messages[i] = malloc(code.packet_length * sizeof(int));
    }

    code.memory_block = interleaved_messages;

    return code;
}