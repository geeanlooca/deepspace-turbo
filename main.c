#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <omp.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "libconvcodes.h"
#include "utilities.h"

#define SIGN(x) (x >= 0) - (x < 0)

// thread routines
int simulate_awgn(int *packet, double *noise_sequence, int packet_length, double sigma);
int encoding_task(int *packet, double *noise_sequence, int packet_length, double sigma, t_convcode code);

// cc functions
int conv_state_update(int state, double input);
void conv_output_fun(int state, double input, int *output);
double termin_fun(int state);

/*
    Main program
*/
int main(int argc, char *argv[])
{

    // define code
    t_convcode code = {/*{{{*/
            .memory = 3,
            .state_update_function = conv_state_update,
            .output_function = conv_output_fun,
            .termination_function = termin_fun,
            .n = 2, //inverse of the rate for rate 1/n,
            .neighbors = {
                    {{0, 0}, {1, 0}},
                    {{2, 0}, {3, 0}},
                    {{4, 0}, {5, 0}},
                    {{6, 0}, {7, 0}},
                    {{0, 1}, {1, 1}},
                    {{3, 1}, {2, 1}},
                    {{4, 1}, {5, 1}},
                    {{6, 1}, {7, 1}}
            }
    };/*}}}*/

    // file handling/*{{{*/
    struct tm *time_;
    time_t now = time(NULL);
    time_ = localtime(&now);
    char buff[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buff, PATH_MAX) ;

    if (len == -1){
        printf("Error while determining the path of the binary file.\n");
        return -1;
    }

    /* printf("Path of the executable: %s\n", buff); */
    /* printf("Lenght of the string: %d\n", len); */

    // remove filename from path
    ssize_t i  = len - 1;
    for (; i >= 0 && buff[i] != '/'; i--);

    char truncated_path[PATH_MAX];
    strncpy(truncated_path, buff, (int)i+1);

    //append "results" directory to current executable file
    strcat(truncated_path, "results/");

    // create directory
    int err = mkdir(truncated_path, S_IRWXU);
    int errsv = errno;
    /* printf("About to create directory %s\n", truncated_path); */
    if(errsv != EEXIST && err == -1){
        perror("Error while creating the output directory");
        return -1;
    }

    // determine filename
    char filename[PATH_MAX];
    strftime(filename, sizeof(filename), "uncoded_%Y-%m-%d_%H-%M-%S.dat", time_);

    // full path
    strcat(truncated_path, filename);

    printf("Trying to create output file...");
    FILE *file = fopen(truncated_path, "w");

    if (!file){
        perror("\nSomething went wrong. Couldn't create output file");
        return -1;
    }

    printf("done.\n");/*}}}*/

    // simulation parameters
    unsigned int packet_length = 1e3;
    unsigned int num_packets = 1e3;

    int num_SNR = 20;
    int min_SNR = 5;
    int max_SNR = 30;
    double *SNR_dB = linspace(min_SNR, max_SNR, num_SNR);
    double *sigma = malloc(num_SNR* sizeof *sigma);

    // get noise std variation from SNR
    for (int i = 0; i < num_SNR; i++)
        sigma[i] = sqrt(1/ pow(10, SNR_dB[i]/10));

    print_array(sigma, num_SNR);

    printf("**************************************************************\n");
    printf("Simulation starting with the following parameters:\n");
    printf("(Uncoded) Packet Length: %d\tPacket Number: %d\n", packet_length, num_packets);
    printf("Minimum SNR: %d dB\tMax SNR: %d dB\tSNR points: %d\n", min_SNR, max_SNR, num_SNR);
    printf("**************************************************************\n\n");
    printf("Press ENTER to start the simulation.\n");
    getchar();

    // simulation loop/*{{{*/

    // initialize seed of RNG
    srand(time(NULL));

    // number of erroneous bits for each tested packet
    long int *errors = calloc(num_SNR, sizeof(long int));
    int pktcount = 0;

    omp_set_num_threads(4);
    #pragma omp parallel
    {
        #pragma omp for nowait
        for (int k = 0; k < num_packets; k++)
        {
            pktcount++;
            // generate packet
            int *packet = randbits(packet_length);

            printf("Processing packet #%d/%d\n", pktcount, num_packets);

            //double *noise_sequence = randn(0, 1, packet_length);
            double *noise_seq_coded = randn(0, 1, (packet_length + code.memory)*code.n);

            for (int s = 0; s < num_SNR; s++){
                errors[s] += encoding_task(packet, noise_seq_coded, packet_length, sigma[s], code);
            }

            free(packet);
            free(noise_seq_coded);
        }
    }

    // compute BER
    double *BER = malloc(num_SNR*sizeof *BER);
    for (int i = 0; i < num_SNR; i++)
        BER[i] = (double) errors[i]/(num_packets*packet_length);

    printf("\nSimulation completed.\n\n");/*}}}*/

    // save results
    char *headers[] = {"SNR", "BER"};
    printf("Saving results...\n");
    save_data(SNR_dB, BER, headers, num_SNR, file);
    fclose(file);

    print_array(BER, num_SNR);

    // release allocated memory
    printf("Releasing memory...");
    free(BER);
    free(errors);
    free(SNR_dB);
    free(sigma);
    printf("Bye!\n");

    return 0;
}

// thread routines
int simulate_awgn(int *packet, double *noise_sequence, int packet_length, double sigma)/*{{{*/
{
    int errors = 0;/*{{{*/
    for (int i = 0; i < packet_length; i++){
        double received = sigma*noise_sequence[i] + (2*packet[i]-1);
        errors += (SIGN(received) != packet[i]);
    }
    return errors;/*}}}*/
}

int encoding_task(int *packet, double *noise_sequence, int packet_length, double sigma, t_convcode code)
{
    int errors = 0;/*{{{*/
    int *encoded = conv_encode(packet, packet_length, code);
    int encoded_length = code.n*(packet_length + code.memory);

    double *received = malloc(encoded_length * sizeof *received);
    for (int i = 0; i < encoded_length; i++)
        received[i] += sigma*noise_sequence[i];

    int *decoded = conv_decode(received, encoded_length, code);
    for (int j = 0; j < packet_length; ++j) {
        errors += (decoded[j] != packet[j]);
    }

    free(encoded);
    free(received);
    return errors;/*}}}*/
}

/*
    state update and output functions for the code assigned for the homework
*/
int conv_state_update(int state, double input)
{
    int s2 = (state >> 1) & 1;/*{{{*/
    int s1 = (state >> 2) & 1;
    int s0 = (int) input;

    return 4*s0 + 2*s1 + s2;/*}}}*/
}

void conv_output_fun(int state, double input, int *output)
{
    int i = (int)input;/*{{{*/
    int s0 = (state >> 2) & 1;
    int s1 = (state >> 1) & 1;
    int s2 = (state >> 0) & 1;

    output[0] = (((i + s1) % 2) + s2) % 2;
    output[1] = (output[0] + s0) % 2;/*}}}*/
}

// poly codes require zero input to terminate trellis
double termin_fun(int state)
{
/*{{{*/
    return 0;
/*}}}*/
}/*}}}*/
