//
// Created by gianluca on 20/02/17.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "libconvcodes.h"

static int get_bit(int num, int position)
{
    return (num >> position) & 1;
}

static char* state2str(int state, int memory)
{
    char *str_state = malloc(memory + 1);
    str_state[memory] = '\0';

    for (int i = 0; i < memory; i++) {
       str_state[i] = '0' + get_bit(state, memory - 1 - i);
    }

    return str_state;
}

static int convcode_stateupdate(int state, int input, t_convcode code)
{
    int memory = code.memory;

    int first_reg = 0;
    for (int i = 0; i < memory; i++) {
        first_reg = (first_reg + code.backward_connections[i]*get_bit(state, memory - 1 - i)) % 2;
    }

    // shift the content of the registers
    int new_state = state >> 1;

    // compute the new content of the first register (MSB)
    first_reg = (first_reg + input) % 2;

    // switch last bit
    new_state ^= (-first_reg ^ new_state) & (1 << (memory - 1));

    return new_state;
}

static int *convcode_output(int state, int input, t_convcode code)
{
    int *output = calloc(code.components, sizeof(int));
    int new_state = convcode_stateupdate(state, input, code);

    // get content of first register of the new state
    // we have to add it to the feedforward part
    int first_reg = get_bit(new_state, code.memory - 1);

    for (int c = 0; c < code.components; c++) {
        output[c] = code.forward_connections[c][0]*first_reg;

        for (int i = 0; i < code.memory; i++) {
            output[c] = (output[c] + code.forward_connections[c][i+1]*get_bit(state, code.memory - 1 - i)) % 2;
        }
    }

    return output;
}

t_convcode convcode_initialize(char *forward[], char *backward, int N_components)
{

    // code initialized
    t_convcode code;

    code.components = N_components;

    // number of shift registers
    int code_memory = strlen(backward);
    code.memory = code_memory;

    // initialize connection arrays
    int **fwd_con = malloc(N_components * sizeof(int*));
    int *bwd_con = malloc(code_memory*sizeof(int));

    // convert input strings to arrays
    for (int i = 0; i < N_components; i++) {
        fwd_con[i] = malloc((code_memory+1) * sizeof(int));

        int j = 0;
        for (; j < code_memory; j++) {
            fwd_con[i][j] = forward[i][j] - '0';
            bwd_con[j] = backward[j] - '0';
        }
        fwd_con[i][j] = forward[i][j] - '0';
    }

    code.forward_connections = fwd_con;
    code.backward_connections = bwd_con;

    int N_states = 2 << (code_memory - 1);
    int **neighbors = malloc(N_states * sizeof(int*));

    // populate lookup table for state-update function
    // and create neighbors array
    int **next_state = malloc(N_states * sizeof(int*));
    for (int i = 0; i < N_states; i++) {
        // initialize to 0
        neighbors[i] = calloc(2, sizeof(int));
    }

    for (int i = 0; i < N_states; i++) {
       next_state[i] = malloc(2 * sizeof(int));

       int updated0 = convcode_stateupdate(i, 0, code);
       next_state[i][0] = updated0;


       // save to neighbords array, use minus sign is input is 0
       // plus sign if input is 1. check whether it's possible to
       // write by checking if it's content is zero.
       // Exploit the fact that in binary codes a state only
       // has two neighbors
       if (!neighbors[updated0][0])
            neighbors[updated0][0] = -(i + 1);
       else
            neighbors[updated0][1] = -(i + 1);

       int updated1 = convcode_stateupdate(i, 1, code);
       next_state[i][1] = updated1;

       if (!neighbors[updated1][0])
            neighbors[updated1][0] = i + 1;
       else
            neighbors[updated1][1] = i + 1;
    }

    code.next_state = next_state;
    code.neighbors = neighbors;


    // populate output function lookup table
    int ***output;
    output = malloc(N_states * sizeof(int**));
    for (int i = 0; i < N_states; i++) {
        output[i] = malloc(2*sizeof(int*));
        for (int j = 0; j < 2; j++) {
            output[i][j] = convcode_output(i, j, code);
        }
    }
    code.output = output;

    return code;
}


void convcode_clear(t_convcode code)
{
    for (int i = 0; i < code.components; i++) {
        /* printf("Component %d \t Address %p\n", i, code.forward_connections[i]); */
        free(code.forward_connections[i]);
        free(code.next_state[i]);
        free(code.neighbors[i]);
    }

    free(code.forward_connections);
    free(code.backward_connections);
    free(code.next_state);
    free(code.neighbors);
}

int* convcode_encode(int *packet, int packet_length, t_convcode code)
{
    // add support for puncturing patterns?
    int encoded_length = (packet_length + code.memory) * code.components;
    int *encoded_packet = malloc(encoded_length * sizeof *encoded_packet);

    int state = 0;

    for (int i = 0; i < packet_length; i++)
    {
        int current_bit = packet[i];
        int *output = code.output[state][current_bit];
        state = code.next_state[state][current_bit];

        for (int c = 0; c < code.components; c++)
        {
            int out = output[c];
            encoded_packet[code.components * i + c] = output[c];
        }
    }

    // add trellis termination
    for (int i = packet_length; i < packet_length + code.memory; i++)
    {
        int input = 0;

        // input is equal to the feedback part in order to inject zeros into the registers
        for (int j = 0; j < code.memory; j++)
            input = (input + code.backward_connections[j]*get_bit(state, code.memory - 1 - j)) % 2;

        int *output = code.output[state][input];
        state = code.next_state[state][input];

        for (int c = 0; c < code.components; c++)
        {
            int out = output[c];
            encoded_packet[code.components * i + c] = out;
        }
    }

    return encoded_packet;
}

int* convcode_decode(double *received, int length, t_convcode code)
{
    int N_states = 2 << (code.memory - 1);
    int packet_length = length / code.components - code.memory;
    int *decoded_packet = malloc(packet_length * sizeof *decoded_packet);


    // allocate matrix containing survivor sequences and metric vector
    double *metric = malloc(N_states * sizeof *metric);
    int **data_matrix;
    data_matrix = malloc(N_states * sizeof(int*));

    for (int i = 0; i < N_states; i++ )
    {
        data_matrix[i] = malloc((packet_length + code.memory)* sizeof(int*));
        metric[i] = 1e6; // should be Infinity
    }

    // trellis starts at state 0
    metric[0] = 0;

    for (int k = 0; k < packet_length + code.memory; k++) {

        /* printf("Iteration %d\t Received symbols:", k); */
        // get received symbol
        double *rho = malloc(code.components * sizeof *rho);
        for (int r = 0; r < code.components; r++) {
           rho[r] = received[k*code.components + r];
           /* printf("\t%f ", rho[r]); */
        }
        /* printf("\n"); */

        double *tmp_metric = malloc(N_states * sizeof *tmp_metric);
        for (int s = 0; s < N_states; s++) {

            /* printf("State: %d\tNeighbors:", s); */
            // get neighbors
            int nA = abs(code.neighbors[s][0]) - 1;
            int uA = (code.neighbors[s][0] > 0);
            int nB = abs(code.neighbors[s][1]) - 1;
            int uB = (code.neighbors[s][1] > 0);

            int *outA = code.output[nA][uA];
            int *outB = code.output[nB][uB];
            /* printf("\t(%s, %d) -> [%d%d]", state2str(nA, code.memory), uA, outA[0], outA[1]); */
            /* printf("\t(%s, %d) -> [%d%d]\n", state2str(nB, code.memory), uB, outB[0], outB[1]); */


            double costA = 0;
            double costB = 0;
            for (int i = 0; i < code.components; i++) {
               costA +=  pow(rho[i] - 2*outA[i] + 1, 2);
               costB +=  pow(rho[i] - 2*outB[i] + 1, 2);
            }

            costA += metric[nA];
            costB += metric[nB];

            double minimum_cost = (costA > costB) ? costB : costA;
            int idx = minimum_cost == costB;
            tmp_metric[s] = minimum_cost;

            data_matrix[s][k] = code.neighbors[s][idx];
            /* printf("State: %d, idx: %d, Neighbor: %d\n", s, idx, code.neighbors[s][idx]); */
        }

        double min_metric = tmp_metric[0];
        for (int s = 0; s < N_states; s++) {
           min_metric = (min_metric < tmp_metric[s]) ? min_metric : tmp_metric[s];
        }

        for (int s = 0; s < N_states; s++){
            metric[s] = tmp_metric[s] - min_metric;
        }

        free(rho);
        free(tmp_metric);
    }

    /* printf("Start backtracking\n"); */
    // backtrack
    int state = 0; // trellis is terminated
    for (int k = packet_length + code.memory - 1; k >= 0; k--)
    {
       int input = (data_matrix[state][k] > 0);
       state = abs(data_matrix[state][k]) - 1;

       if (k < packet_length)
           decoded_packet[k] = input;
    }

    // de-allocate metric
    free(metric);

    // de-allocate matrix
    for (int i = 0; i < N_states; i++ )
        free(data_matrix[i]);
    free(data_matrix);

    return decoded_packet;
}

void print_neighbors(t_convcode code)
{
    int N_states = 2 << (code.memory - 1);/*{{{*/

    for (int i = 0; i < 34; i++){
        if (i % 11)
            printf("-");
        else
            printf("+");
    }
    printf("\n");
    printf("|%-10s|%-10s|%-10s|\n", "STATE", "NEIGHBOR", "INPUT");
    for (int i = 0; i < 34; i++){
        if (i % 11)
            printf("-");
        else
            printf("+");
    }
    printf("\n");

    for (int i = 0; i < N_states; i++) {
        int s0 = abs(code.neighbors[i][0])-1;
        int s1 = abs(code.neighbors[i][1])-1;

        int u0 = (code.neighbors[i][0] > 0) ? 1 : 0;
        int u1 = (code.neighbors[i][1] > 0) ? 1 : 0;

        printf("|%-10s|%-10s|%-10d|\n", state2str(i, code.memory), state2str(s0, code.memory), u0);
        printf("|%-10s|%-10s|%-10d|\n", state2str(i, code.memory), state2str(s1, code.memory), u1);
    }
    for (int i = 0; i < 34; i++){
        if (i % 11)
            printf("-");
        else
            printf("+");
    }
    printf("\n");/*}}}*/
}


