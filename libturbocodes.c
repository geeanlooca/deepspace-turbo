//
// Created by gianluca on 22/02/17.
//

#include "libturbocodes.h"
#include <stdlib.h>
#include "utilities.h"
#include <stdio.h>

void turbo_interleave(int *packet, int length, t_turbocode code)
{

    printf("Applying interleaving function\n");
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
    int turbo_length = 0;

    for (int i = 0; i < code.components; ++i) {
        // get corresponding (possibly) interleaved input message
        t_convcode cc = code.inner_codes[i];
        int *interleaved_pkt = code.memory_block[i];
        int conv_length = cc.components * (packet_length + cc.memory);
        conv_encoded[i] = convcode_encode(interleaved_pkt, packet_length, cc);

        printf("Output of CC #%d: \n", i);
        /* print_array_int(conv_encoded[i], conv_length); */
        turbo_length += conv_length;
    }

    int *turbo_encoded = malloc(turbo_length * sizeof *turbo_encoded);

    // parallel to serial
    int k = 0;
    int c = 0;
    int cw = 0;

    while (k < turbo_length) {
        t_convcode cc = code.inner_codes[c];
        
        // number of components of cc
        int comps = cc.components;

        // copy bits from cc output to turbo_encoded
        for (int i = 0; i < comps; i++) {
           turbo_encoded[k++] = conv_encoded[c][cw*comps + i];
        }
        
        c = (c + 1) % code.components;
        // when c = 0 the first codeword is complete
        cw = c == 0  ? cw+1 : cw;
    }
    return turbo_encoded;
}

t_turbocode turbo_initialize(int components, t_convcode *codes, int **interleaver, int packet_length)
{
    t_turbocode code;
    code.components = components;
    code.inner_codes = codes;
    code.packet_length = packet_length;
    code.interleaving_vectors = interleaver;

    printf("allocate memory block to store messages to feed to the convolutional encoders \n");
    // allocate memory block to store messages to feed to the convolutional encoders
    int **interleaved_messages = malloc(code.components * sizeof(int*));
    for (int i = 0; i < code.components; ++i) {
        interleaved_messages[i] = malloc(code.packet_length * sizeof(int));
    }

    code.memory_block = interleaved_messages;

    return code;
}
