//
// Created by gianluca on 22/02/17.
//

#include "libturbocodes.h"
#include <stdlib.h>
#include "utilities.h"
#include <stdio.h>
#include <math.h>


t_turbocode turbo_initialize(t_convcode upper, t_convcode lower, int components, int *interleaver, int packet_length)
{
    t_turbocode code;/*{{{*/
    code.upper_code = upper;
    code.lower_code = lower;

    code.packet_length = packet_length;
    code.interleaver = interleaver;

    // compute encoded length
    int turbo_length = 0;
    turbo_length += upper.components * (code.packet_length + upper.memory);
    turbo_length += lower.components * (code.packet_length + lower.memory);

    code.encoded_length = turbo_length;

    return code;/*}}}*/
}

static int *turbo_interleave(int *packet, t_turbocode code)
{
    int *interleaved_message = malloc(code.packet_length * sizeof(int));
    for (int j = 0; j < code.packet_length; ++j) {
        interleaved_message[j] = packet[code.interleaver[j]];
    }

    return interleaved_message;/*}}}*/
}

int *turbo_encode(int *packet, t_turbocode code)
{
    int *interleaved_packet = turbo_interleave(packet, code);/*{{{*/

    // reference to encoded messages
    int **conv_encoded = malloc(2 * sizeof(int*));
    int turbo_length = code.encoded_length;
    conv_encoded[0] = convcode_encode(packet, code.packet_length, code.upper_code);
    conv_encoded[1] = convcode_encode(interleaved_packet, code.packet_length, code.lower_code);

    int *turbo_encoded = malloc(turbo_length * sizeof *turbo_encoded);

    t_convcode codes[2] = {code.upper_code, code.lower_code};
    // parallel to serial
    int k = 0, c = 0, cw = 0;/*{{{*/
    while (k < turbo_length) {
        t_convcode cc = codes[c];

        // number of components of cc
        int comps = cc.components;

        // copy bits from cc output to turbo_encoded
        for (int i = 0; i < comps; i++) {
            int bit = conv_encoded[c][cw*comps + i];
            turbo_encoded[k++] = bit;
        }

        c = (c + 1) % 2;
        // when c = 0 the first codeword is complete
        cw = !c  ? cw+1 : cw;
    }/*}}}*/

    free(conv_encoded[0]);
    free(conv_encoded[1]);
    free(conv_encoded);

    free(interleaved_packet);

    return turbo_encoded;/*}}}*/
}

int *turbo_decode(double *received, int iterations, double noise_variance, t_turbocode code)
{
    // serial to parallel/*{{{*/
    int *lengths = malloc(2 * sizeof  *lengths);/*{{{*/
    double **streams = malloc(2 * sizeof(double*));
    t_convcode codes[2] = {code.upper_code, code.lower_code};
    for (int i = 0; i < 2; i++) {
        t_convcode cc = codes[i];
        lengths[i] = cc.components * (code.packet_length + cc.memory);
        streams[i] = malloc(lengths[i] * sizeof(double));
    }
    
    int k = 0, c = 0, cw = 0;
    while (k < code.encoded_length) {
        t_convcode cc = codes[c];

        for (int i = 0; i < cc.components; i++)
           streams[c][cw*cc.components + i] = received[k++];

        c = (c + 1) % 2;
        cw = !c ? cw + 1 : cw;
    }/*}}}*/

    int *turbo_decoded = malloc(code.packet_length * sizeof *turbo_decoded);

    // initial messages
    double **messages = malloc(2 * sizeof(double *));
    for (int i = 0; i < 2; i++) {
       messages[i] = malloc(code.packet_length * sizeof(double));
       for (int j = 0; j < code.packet_length; j++) {
           messages[i][j] = log(0.5);
       }
    }

    for (int i = 0; i < iterations; i++) {
       double **extrinsic_upper = convcode_extrinsic(streams[0], lengths[0], messages, code.upper_code, noise_variance);

       // apply interleaver
       
       double **extrinsic_lower = convcode_extrinsic(streams[1], lengths[1], extrinsic_upper, code.lower_code, noise_variance);

       // deinterleave

       free(messages[0]);
       free(messages[1]);
       free(extrinsic_upper[0]);
       free(extrinsic_upper[1]);

       messages = extrinsic_lower;
    }

    //decision


    for (int i = 0; i < 2; i++)
        free(streams[i]);
    free(streams);

    //length of the 
    return turbo_decoded; /*}}}*/
}
