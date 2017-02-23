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
#include <getopt.h>


// thread routines
int simulate_awgn(int *packet, double *noise_sequence, int packet_length, double sigma);
int simulate_conv(int *packet, double *noise_sequence, int packet_length, double sigma, t_convcode code);


int main(int argc, char *argv[])
{
    // option flags
    int skipconfirm_flag = 0;
    int filename_flag = 0;

    // default simulation parameters
    int packet_length = (int) 1e3;
    int num_packets = (int) 1e4k;

    int SNR_points = 10;
    float min_SNR = -4;
    float max_SNR = 6;

    char filename[PATH_MAX];


    // arguments parser
    int c;
    while(1)
    {
        static struct option long_options[] =
                {
                        {"skip-confirm",    no_argument,        0,  'y'},
                        {"forward-vector",  required_argument,  0,  'f'},
                        {"backward-vector", required_argument,  0,  'b'},
                        {"output",          required_argument,  0,  'o'},
                        {"packet-length",   required_argument,  0,  'l'},
                        {"packet-count",    required_argument,  0,  'c'},
                        {"cores",           required_argument,  0,  'C'},
                        {"min-SNR",         required_argument,  0,  'm'},
                        {"max-SNR",         required_argument,  0,  'M'},
                        {"SNR-points",      required_argument,  0,  'n'},
                        {0, 0, 0, 0}
                };

        int option_index = 0;

        c = getopt_long(argc, argv, "yl:c:C:m:M:f:b:o:n:", long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case 0:
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;

            case 'm':
                min_SNR = strtof(optarg, NULL);
                break;

            case 'M':
                max_SNR = strtof(optarg, NULL);
                break;

            case 'n':
                SNR_points = (int)strtol(optarg, NULL, 10);
                break;

            case 'o':
                strcpy(filename, optarg);
                filename_flag = 1;
                break;

            case 'y':
                skipconfirm_flag = 1;
                break;

            default:
                abort();
        }
    }

    // check parameters
    if (min_SNR >= max_SNR){
        printf("The value of the minimum SNR to test is greater than the maximum SNR (%f > %f).\n", min_SNR, max_SNR);
        exit(EXIT_FAILURE);
    }

    if (SNR_points <= 0){
        printf("Number of SNR points to test must be strictly positive.\n");
        exit(EXIT_FAILURE);
    }

    // print table
    char header[PATH_MAX];
    int w = 15;
    sprintf(header, "|%-*s|%-*s|%-*s|%-*s|%-*s|", w, "Packet length", w, "Packet count",
                w, "Min SNR [dB]", w, "Max SNR [dB]", w, "SNR points");

    int header_length = (int)strlen(header);
    for (int l = 0; l < header_length; ++l) {
        char ch =  (l % (w+1)) ? '-' : '+';
        printf("%c", ch);
    }

    printf("\n%s\n", header);
    for (int l = 0; l < header_length; ++l) {
        char ch =  (l % (w+1)) ? '-' : '+';
        printf("%c", ch);
    }

    char data_str[256];
    sprintf(data_str, "|%-*d|%-*d|%-*f|%-*f|%-*d|", w, packet_length, w, num_packets,
            w, min_SNR, w, max_SNR, w, SNR_points);
    printf("\n%s\n", data_str);

    for (int l = 0; l < header_length; ++l) {
        char ch =  (l % (w+1)) ? '-' : '+';
        printf("%c", ch);
    }
    printf("\n");


    // confirmation
    char x = (skipconfirm_flag) ? 'y' : 0;
    printf("Start simulation? y/n: ");
    while (x != 'y' && x != 'n')
        x = getchar();

    if (x == 'n'){
        printf("\nSimulation aborted.\n");
        exit(EXIT_SUCCESS);
    }

    printf("\nSimulation starting...\n");

    exit(EXIT_FAILURE);


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
        exit(EXIT_FAILURE);
    }

    // determine filename
    strftime(filename, sizeof(filename), "uncoded_%Y-%m-%d_%H-%M-%S.dat", time_);

    // full path
    strcat(truncated_path, filename);

    printf("Trying to create output file...");
    FILE *file = fopen(truncated_path, "w");

    if (!file){
        perror("\nSomething went wrong. Couldn't create output file");
        exit(EXIT_FAILURE);
    }

    printf("done.\n");/*}}}*/


    double *SNR_dB = linspace(min_SNR, max_SNR, SNR_points);
    double *sigma = malloc(SNR_points* sizeof *sigma);
    double *EbN0 = malloc(SNR_points * sizeof *EbN0);

    // define code
    int N_components = 2;
    char *forward[N_components];
    forward[0] = "101";
    forward[1] = "101";
//    forward[3] = "11111";

    char *backward;
    backward = "00";

    double rate = 1.0f/N_components;


    // get noise std variation from SNR
    for (int i = 0; i < SNR_points; i++)
    {
        sigma[i] = sqrt(1/ pow(10, SNR_dB[i]/10));
        EbN0[i] = 1 / (2*rate*pow(sigma[i], 2));
    }


    // initialize code: mandatory call
    t_convcode code = convcode_initialize(forward, backward, N_components);



    // print info
    printf("**************************************************************\n");
    printf("Simulation starting with the following parameters:\n");
    printf("(Uncoded) Packet Length: %d\tPacket Number: %d\n", packet_length, num_packets);
    printf("Minimum SNR: %f dB\tMax SNR: %f dB\tSNR points: %d\n", min_SNR, max_SNR, SNR_points);
    printf("**************************************************************\n\n");
    printf("Press ENTER to start the simulation.\n");

    // build interleaver
//    int octets = 1;
//    int base = 10;
//    int info_length = base * 8 * octets;
//    int p[8] = {31, 37, 43, 47, 53, 59, 61, 67};
//    int k1 = 8;
//    int k2 = base * octets;
//
//    int **interleaver = malloc(2 * sizeof(int*));
//    int *pi = malloc(info_length * sizeof *pi);
//    int *identity = malloc(info_length * sizeof *identity);
//
//    for (int s = 0; s < info_length; ++s) {
//        int m = (s-1)%2;
//        int i = (int) floor((s-1) / (2 * k2));
//        int j = (int) floor((s-1) / 2 ) - i*k2;
//        int t = (19*i + 1)%(k2/2);
//        int q = t % 8 + 1;
//        int c = (p[q-1]*j + 21*m) % k2;
//        pi[s] = 2*(t + c*(k1/2) + 1) - m;
//        identity[s] = s;
//    }
//
//    interleaver[0] = identity;
//    interleaver[1] = pi;
//    int mask[4] = {1, 2, 3, 4};
//
//    t_convcode codes[2] = {code, code};
//    t_turbocode turbo = turbo_initialize(2, codes, interleaver, info_length, mask, 4, 4);
//
//    // create packet
//    int *pkt = randbits(info_length);
//    turbo_interleave(pkt, info_length, turbo);
//
//    print_array_int(pkt, info_length);
//    print_array_int(turbo.memory_block[0], info_length);
//    print_array_int(turbo.memory_block[1], info_length);


    // simulation loop
    // initialize seed of RNG
    srand(time(NULL));

    // number of erroneous bits for each tested packet
    long int *errors = calloc(SNR_points, sizeof(long int));
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

            for (int s = 0; s < SNR_points; s++){
                errors[s] += simulate_conv(packet, noise_seq_coded, packet_length, sigma[s], code);
                //errors[s] += simulate_awgn(packet, noise_sequence, packet_length, sigma[s]);
            }

            free(packet);
            //free(noise_sequence);
            free(noise_seq_coded);
        }
    }

    // compute BER
    double *BER = malloc(SNR_points*sizeof *BER);
    for (int i = 0; i < SNR_points; i++)
        BER[i] = (double) errors[i]/(num_packets*packet_length);

    printf("\nSimulation completed.\n\n");

    // save results
    char *headers[] = {"SNR", "BER"};
    printf("Saving results...\n");
    save_data(SNR_dB, BER, headers, SNR_points, file);
    fclose(file);

    for (int j = 0; j < SNR_points; ++j) {
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

