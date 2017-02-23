//
// Created by gianluca on 22/02/17.
//

#include "libturbocodes.h"
#include <stdlib.h>

void turbo_interleave(int *packet, int length, t_turbocode code)
{
    for (int i = 0; i < code.components; ++i) {
        for (int j = 0; j < code.packet_length; ++j) {
            code.memory_block[i][j] = packet[code.interleaving_vectors[i][j]];
        }
    }
}

int * turbo_encode(int *packet, int packet_length, t_turbocode code)
{
    turbo_interleave(packet, packet_length, code);

    // reference to encoded messages
    int **conv_encoded = malloc(code.components * sizeof(int*));

    for (int i = 0; i < code.components; ++i) {
        // get corresponding (possibly) interleaved input message
        int *interleaved_pkt = code.memory_block[i];
        conv_encoded[i] = convcode_encode(interleaved_pkt, packet_length, code.inner_codes[i]);
    }

    // suppose every convcode has same memory
    int turbo_length = code.k * (packet_length + code.inner_codes[0].memory);
    int conv_length = code.inner_codes[0].components * (packet_length + code.inner_codes[0].memory);
    int *turbo_encoded = malloc(turbo_length * sizeof *turbo_encoded);


    int mask_index = 0;
    int mask_length = code.mask_length;
    int codeword_index = 0;

    for (int j = 0; j < turbo_length;) {
        codeword_index = j / code.k;
        for (int i = 0; i < mask_length && j < turbo_length; ++i) {

            int c = code.code_mask[mask_index];
            int output_index = code.mask[mask_index];
            turbo_encoded[j++] = conv_encoded[c][codeword_index*code.inner_codes[c].components + output_index];
        }
    }

    return turbo_encoded;
}

t_turbocode turbo_initialize(int components, t_convcode *codes, int **interleaver, int packet_length, int *mask,
                             int mask_length, int k)
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
    code.mask = mask;
    code.mask_length = mask_length;
    code.k = k;

    // determine code mask from provided output mask
    int *code_mask = malloc(mask_length * sizeof *code_mask);

    for (int j = 0; j < mask_length; ++j) {
        int c = 0;
        int cumulative = code.inner_codes[c].components;

        while ( mask[j] >= cumulative) {
            cumulative += code.inner_codes[++c].components;
        }

        mask[j] -= (c > 0) ? cumulative : 0;
        code_mask[j] = c;
    }

    code.code_mask = code_mask;

    return code;
}