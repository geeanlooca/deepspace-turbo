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
#include "libturbocodes.h"
#include "utilities.h"

// thread routines
int simulate_awgn(int *packet, double *noise_sequence, int packet_length, double sigma);
int simulate_conv(int *packet, double *noise_sequence, int packet_length, double sigma, t_convcode code);

int main(int argc, char *argv[])
{
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

    // define code
    int N_components = 2;
    char *forward[N_components];
    forward[0] = "11010";
    forward[1] = "11101";
    forward[2] = "10111";

    char *backward;
    backward = "0000";

    double rate = 1.0f/N_components;

    // initialize code: mandatory call
    t_convcode code = convcode_initialize(forward, backward, N_components);

    // simulation parameters
    int packet_length = (int) 1e3;
    int num_packets = (int) 1e4;

    int num_SNR = 20;
    int min_SNR = -2;
    int max_SNR = 10;
    double *SNR_dB = linspace(min_SNR, max_SNR, num_SNR);
    double *sigma = malloc(num_SNR* sizeof *sigma);
    double *EbN0 = malloc(num_SNR * sizeof *EbN0);

    // get noise std variation from SNR
    for (int i = 0; i < num_SNR; i++)
    {
        sigma[i] = sqrt(1/ pow(10, SNR_dB[i]/10));
        EbN0[i] = 1 / (2*rate*pow(sigma[i], 2));
    }

    // print info
    printf("**************************************************************\n");
    printf("Simulation starting with the following parameters:\n");
    printf("(Uncoded) Packet Length: %d\tPacket Number: %d\n", packet_length, num_packets);
    printf("Minimum SNR: %d dB\tMax SNR: %d dB\tSNR points: %d\n", min_SNR, max_SNR, num_SNR);
    printf("**************************************************************\n\n");
    printf("Press ENTER to start the simulation.\n");

    // build interleaver
    int octets = 1;
    int base = 223;
    int info_length = base * 8 * octets;
    int p[8] = {31, 37, 43, 47, 53, 59, 61, 67};
    int k1 = 8;
    int k2 = base * octets;

    int *pi = malloc(info_length * sizeof *pi);
    for (int s = 0; s < info_length; ++s) {
        int m = (s-1)%2;
        int i = (int) floor((s-1) / (2 * k2));
        int j = (int) floor((s-1) / 2 ) - i*k2;
        int t = (19*i + 1)%(k2/2);
        int q = t % 8 + 1;
        int c = (p[q-1]*j + 21*m) % k2;
        pi[s] = 2*(t + c*(k1/2) + 1) - m;
    }




    return 0;

    // simulation loop
    // initialize seed of RNG
    srand(time(NULL));

    // number of erroneous bits for each tested packet
    long int *errors = calloc(num_SNR, sizeof(long int));
    int packet_count = 0;

    omp_set_num_threads(4);
    #pragma omp parallel
    {
        #pragma omp for nowait
        for (int k = 0; k < num_packets; k++)
        {
            packet_count++;
            // generate packet
            int *packet = randbits(packet_length);

            printf("Processing packet #%d/%d\n", packet_count, num_packets);

            //double *noise_sequence = randn(0, 1, packet_length);
            double *noise_seq_coded = randn(0, 1, (packet_length + code.memory)*code.components);

            for (int s = 0; s < num_SNR; s++){
                errors[s] += simulate_conv(packet, noise_seq_coded, packet_length, sigma[s], code);
                //errors[s] += simulate_awgn(packet, noise_sequence, packet_length, sigma[s]);
            }

            free(packet);
            //free(noise_sequence);
            free(noise_seq_coded);
        }
    }

    // compute BER
    double *BER = malloc(num_SNR*sizeof *BER);
    for (int i = 0; i < num_SNR; i++)
        BER[i] = (double) errors[i]/(num_packets*packet_length);

    printf("\nSimulation completed.\n\n");

    // save results
    char *headers[] = {"SNR", "BER"};
    printf("Saving results...\n");
    save_data(SNR_dB, BER, headers, num_SNR, file);
    fclose(file);

    for (int j = 0; j < num_SNR; ++j) {
       printf("%20f dB%20.4e\n", SNR_dB[j], BER[j]);
    }

    // release allocated memory
    free(BER);
    free(errors);
    free(SNR_dB);
    free(sigma);

    return 0;
}

// thread routines
int simulate_awgn(int *packet, double *noise_sequence, int packet_length, double sigma)/*{{{*/
{
    int errors = 0;/*{{{*/
    for (int i = 0; i < packet_length; i++){
        double received = sigma*noise_sequence[i] + (2*packet[i]-1);
        errors += (received > 0) != packet[i];
    }
    return errors;/*}}}*/
}

int simulate_conv(int *packet, double *noise_sequence, int packet_length, double sigma, t_convcode code)
{
    int errors = 0;/*{{{*/
    int *encoded = convcode_encode(packet, packet_length, code);
    int encoded_length = code.components*(packet_length + code.memory);


    double *received = malloc(encoded_length * sizeof *received);
    for (int i = 0; i < encoded_length; i++)
        received[i] = (2*encoded[i] - 1) + sigma*noise_sequence[i];

    int *decoded = convcode_decode(received, encoded_length, code);
    for (int j = 0; j < packet_length; ++j) {
        errors += (decoded[j] != packet[j]);
    }

    free(decoded);
    free(encoded);
    free(received);
    return errors;/*}}}*/
}

