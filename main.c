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
int simulate_conv(int *packet, double *noise_sequence, int packet_length, double sigma, t_convcode code);

int main(int argc, char *argv[])
{
    // define code
    int N_components = 2;
    char *forward[N_components];
    forward[0] = "111";
    forward[1] = "011";

    char *backward;
    backward = "00";
    t_convcode code = convcode_initialize(forward, backward, N_components);

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

    // simulation parameters
    int packet_length = (int) 1e3;
    int num_packets = (int) 1e3;

    int num_SNR = 20;
    int min_SNR = -2;
    int max_SNR = 10;
    double *SNR_dB = linspace(min_SNR, max_SNR, num_SNR);
    double *sigma = malloc(num_SNR* sizeof *sigma);

    // get noise std variation from SNR
    for (int i = 0; i < num_SNR; i++)
        sigma[i] = sqrt(1/ pow(10, SNR_dB[i]/10));

    printf("**************************************************************\n");
    printf("Simulation starting with the following parameters:\n");
    printf("(Uncoded) Packet Length: %d\tPacket Number: %d\n", packet_length, num_packets);
    printf("Minimum SNR: %d dB\tMax SNR: %d dB\tSNR points: %d\n", min_SNR, max_SNR, num_SNR);
    printf("**************************************************************\n\n");
    printf("Press ENTER to start the simulation.\n");

    // simulation loop/*{{{*/

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

    for (int j = 0; j < num_SNR; ++j) {
       printf("%20f dB%20.4e\n", SNR_dB[j], BER[j]);
    }

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

