//
// Created by gianluca on 20/02/17.
//

#include "libconvcodes.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "libconvcodes.h"

int* conv_encode(double* input, unsigned int length, t_convcode code)
{
    // length of the encoded packet
    int out_length = (code.memory + length)*code.n;

    // array containing the encoded packet
    int *encoded = malloc(out_length * sizeof *encoded);

    // terminated trellis: initial and final state must be 0
    int state = 0;
    int *output = (int*) calloc(code.n, sizeof *output); // contains the n-long output for given (state,input symbol) couple

    int (*state_update_function)(int , double) = code.state_update_function;
    void (*output_function)(int, double, int*) = code.output_function;
    double (*termination_function)(int) = code.termination_function;

    for (int i = 0; i < length + code.memory; i++)
    {
        double u = (i < length) ? input[i] : (*termination_function)(state); // current input
        (*output_function)(state, u, output);
        state = (*state_update_function)(state, u);

        for (int j = 0; j < code.n; j++)
            encoded[i*code.n + j] = output[j];
    }

    //assume poly code
    free(output);

    return encoded;
}

static int index3d(int i, int j, int k, int sizex, int sizey, int sizez)
{
    return sizez*sizey*i + sizez*j + k;
}

int* conv_decode(double* input, unsigned int length, t_convcode code)
{
    int packet_length = length / code.n;
    int *decoded_packet = malloc(packet_length * sizeof *decoded_packet);

    int Ns = (int) pow(2, code.memory); // number of states

    int *data_matrix = malloc(Ns*packet_length*2 * sizeof *data_matrix);

    //initialize metric array
    double *metric = malloc(Ns * sizeof *metric);
    for (int i = 0; i < Ns; i++)
        metric[i] = 1e9;

    int *output0 = malloc(code.n* sizeof *output0);
    int *output1 = malloc(code.n* sizeof *output1);
    void (*output_function)(int, double, int*) = code.output_function;

    for (int k = 0; k < packet_length; k++)
    {
        double *tmp_metric = calloc(Ns, sizeof *tmp_metric);
        double min_metric = 0;
        for(int s = 0; s < Ns; s++)
        {

            double mincost = 0;
            int minindex = 0;

            int n0 = code.neighbors[s][0][0];
            int u0 = code.neighbors[s][0][1];

            int n1 = code.neighbors[s][1][0];
            int u1 = code.neighbors[s][1][1];

            (*output_function)(n0, u0, output0);
            (*output_function)(n1, u1, output1);

            double pathcost0 = 0;
            double pathcost1 = 0;

            for (int p = 0; p < code.n; p++)
            {
                pathcost0 += pow(input[code.n*k + p] - 2*output0[p] + 1,2);
                pathcost1 += pow(input[code.n*k + p] - 2*output1[p] + 1,2);
            }

            double cost0 = metric[n0] + pathcost0;
            double cost1 = metric[n1] + pathcost1;

            if (cost0 < cost1)
            {
                mincost = cost0;
                data_matrix[index3d(s, k, 0, Ns, packet_length, 2)] = n0;
                data_matrix[index3d(s, k, 1, Ns, packet_length, 2)] = u0;
            }
            else
            {
                mincost = cost1;
                data_matrix[index3d(s, k, 0, Ns, packet_length, 2)] = n1;
                data_matrix[index3d(s, k, 1, Ns, packet_length, 2)] = u1;
            }
            min_metric = (mincost < min_metric) ? mincost : min_metric;
            tmp_metric[s] = mincost;
        }

        memcpy(metric, tmp_metric, Ns*sizeof(double));
        free(tmp_metric);
    }

    // backtracking
    int state = 0; //final state is always zero due to trellis termination
    for (int k = packet_length-1; k >= 0; k--)
    {
        decoded_packet[k] = data_matrix[index3d(state, k, 1, Ns, packet_length, 2)];
        state = data_matrix[index3d(state, k, 0, Ns, packet_length, 2)];
    }

    free(data_matrix);
    free(metric);

    return decoded_packet;
}
