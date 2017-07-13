#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <omp.h>
#include <limits.h>
#include <string.h>
#include "libconvcodes.h"
#include "libturbocodes.h"
#include "utilities.h"
#include <getopt.h>
#include "colors.h"

#define MAX_COMPONENTS 4


// puncturing function: return 1 if bit k has to be punctured
int puncturing(int k){

    int bit_idx = k % 3;

    // bit 0,3,6,... corresponding to systematic output
    if (!bit_idx)
        return 0;

    // get block index
    int block_idx = k / 3;

    // on odd blocks puncture second bit
    if (block_idx % 2){
        return bit_idx == 1;
    }

    // on even blocks puncture third bit
    return bit_idx == 2;
}

// thread routines
int simulate_awgn(int *packet, double *noise_sequence, int packet_length, double sigma);
int simulate_conv(int *packet, double *noise_sequence, int packet_length, double sigma, t_convcode code);
int simulate_turbo(int *packet, double *noise_sequence, int packet_length, double sigma, t_turbocode code, int iterations);


int main(int argc, char *argv[])
{
    // option flags
    int skipconfirm_flag = 0;
    int filename_flag = 0;

    // default simulation parameters
    int packet_length = (int) 1e4;
    int num_packets = (int) 10;

    int SNR_points = 8;
    float min_SNR = -2;
    float max_SNR = 2;
    int cores = 1;
    int iterations = 2;
    int octets = 1;

    int code_type = 1;
    char filename[PATH_MAX];


    // parse command line arguments
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
                        {"iterations",      required_argument,  0,  'i'},
                        {"multiplier",      required_argument,  0,  'k'},
                        {"code",            required_argument,  0,  't'},
                        {"help",            no_argument,        0,  'h'},
                        {0, 0, 0, 0}
                };

        int option_index = 0;

        c = getopt_long(argc, argv, "yhl:c:C:m:M:f:b:o:n:i:k:t:", long_options, &option_index);

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

            case 'c':
                num_packets = (int) strtof(optarg, NULL);
                break;

            case 'C':
                cores = (int) strtof(optarg, NULL);
                break;

            case 'l':
                packet_length = (int) strtof(optarg, NULL);
                break;

            case 'h':

                printf(BOLDMAGENTA "%20s" RESET "\n\t%s\n\n" , "-y / --skip-confirm", "skip confirmation dialog after"
                        " summarizing simulation parameters. Useful when automating simulations.");

                printf(BOLDMAGENTA "%20s" RESET "\n\t%s\n\n" , "-h / --help", "print this help dialog.");

                printf(BOLDMAGENTA "%20s" RESET "\n\t%s\n\n" , "-o / --output FILENAME", "save results in "
                        "a comma-separated format in FILENAME. If this argument is not used, the results will be saved in "
                        "a file named with the current date and time.");

                printf(BOLDMAGENTA "%20s" RESET "\n\t%s\n\n" , "-c / --packet-count INTEGER", "set the number of packets to encode/decode."
                        " INTEGER can be given in exponential notation i.e. 1e4 for 10000 packets.");

                printf(BOLDMAGENTA "%20s" RESET "\n\t%s\n\n" , "-C / --cores INTEGER", "set the number of CPU cores to use.");

                printf(BOLDMAGENTA "%20s" RESET "\n\t%s\n\n" , "-k / --multiplier INT", "set the input packet length through the following "
                        " formula: packet-length = 223 * 8 * multiplier");

                printf(BOLDMAGENTA "%20s" RESET "\n\t%s\n\n" , "-l / --packet-length INTEGER", "set the number of information bits"
                        " in a packet. Exponential notation can be used.");

                printf(BOLDMAGENTA "%20s" RESET "\n\t%s\n\n" , "-m / --min-SNR FLOAT", "set the lower extreme of the SNR range to test. ");

                printf(BOLDMAGENTA "%20s" RESET "\n\t%s\n\n" , "-M / --max-SNR FLOAT", "set the upper extreme of the SNR range to test. ");

                printf(BOLDMAGENTA "%20s" RESET "\n\t%s\n\n" , "-n / --SNR-points INTEGER", "set the number of linearly spaced points "
                        "inside the interval [min-SNR, max-SNR]");

                printf(BOLDMAGENTA "%20s" RESET "\n\t%s\n\n" , "-i / --iterations", "set the number of iterations for the turbo decoding algorithm.");
                exit(EXIT_SUCCESS);

            case 'm':
                min_SNR = strtof(optarg, NULL);
                break;

            case 'M':
                max_SNR = strtof(optarg, NULL);
                break;

            case 'n':
                SNR_points = (int)strtol(optarg, NULL, 10);
                break;

            case 'i':
                iterations = (int)strtol(optarg, NULL, 10);
                break;

            case 'k':
                octets = (int)strtol(optarg, NULL, 10);
                break;

            case 't':
                code_type = (int)strtol(optarg, NULL, 10);
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
        printf(BOLDRED "The value of the minimum SNR to test is greater than the maximum SNR (%f > %f).\n" RESET, min_SNR, max_SNR);
        exit(EXIT_FAILURE);
    }

    if (SNR_points <= 0){
        printf(BOLDRED "Number of SNR points to test must be strictly positive.\n" RESET);
        exit(EXIT_FAILURE);
    }

    if (num_packets <= 0){
        printf(BOLDRED "Number of packets to test must be strictly positive.\n" RESET);
        exit(EXIT_FAILURE);
    }

    if (packet_length <= 0){
        printf(BOLDRED "Number of information bits in a packet must be strictly positive.\n" RESET);
        exit(EXIT_FAILURE);
    }

    if (octets <= 0){
        printf(BOLDRED "Packet length multiplier must be strictly positive.\n" RESET);
        exit(EXIT_FAILURE);
    }

    // handle filename
    if (!filename_flag){
        // generate timestamp filename.
        struct tm *time_;
        time_t now = time(NULL);
        time_ = localtime(&now);
        strftime(filename, sizeof(filename), "%Y-%m-%d_%H-%M-%S.csv", time_);

        printf("Output filename not provided. File " BOLDMAGENTA "\'%s\'" RESET " will be used. \n", filename);
    }

    // build interleaver
    int base = 223;
    int info_length = base * 8 * octets;
    int p[8] = {31, 37, 43, 47, 53, 59, 61, 67};
    int k1 = 8;
    int k2 = base * octets;

    int *pi = malloc(info_length * sizeof *pi);

    for (int s = 1; s <= info_length; ++s) {
        int m = (s-1) % 2;
        int i = (int) floor((s-1) / (2 * k2));
        int j = (int) floor((s-1) / 2) - i*k2;
        int t = (19*i + 1) % (k1/2);
        int q = t % 8 + 1;
        int c = (p[q-1]*j + 21*m) % k2;
        pi[s-1] = 2*(t + c*(k1/2) + 1) - m - 1;
    }

    // print simulation parameters
    if (!skipconfirm_flag){
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
        sprintf(data_str, "|%-*d|%-*d|%-*f|%-*f|%-*d|", w, info_length, w, num_packets,
                w, min_SNR, w, max_SNR, w, SNR_points);
        printf("\n%s\n", data_str);

        for (int l = 0; l < header_length; ++l) {
            char ch =  (l % (w+1)) ? '-' : '+';
            printf("%c", ch);
        }
        printf("\n");
    }

    // confirmation
    char x = (skipconfirm_flag) ? 'y' : 0;
    printf("\nStart simulation? y/n: ");
    while (x != 'y' && x != 'n')
        x = getchar();

    if (x == 'n'){
        printf(BOLDRED"\nSimulation aborted.\n" RESET);
        exit(EXIT_SUCCESS);
    }

    printf("\nSimulation starting...\n\n");

    // create output file
    FILE *file = fopen(filename, "w");
    if (!file){
        perror("Something went wrong. Couldn't create output file");
        exit(EXIT_FAILURE);
    }

    // allocate memory to store parameters and results
    double *SNR_dB = linspace(min_SNR, max_SNR, SNR_points);
    double *sigma = malloc(SNR_points* sizeof *sigma);
    double *EbN0 = malloc(SNR_points * sizeof *EbN0);
    long int *errors = calloc(SNR_points, sizeof *errors);
    int *erroneous_packets = calloc(SNR_points, sizeof *erroneous_packets);//needed to estimate PER (Packer Error Probability)
    double *BER = malloc(SNR_points*sizeof *BER);
    double *PER = malloc(SNR_points*sizeof *PER);

    // define codes
    char *forward_upper[MAX_COMPONENTS];
    char *forward_lower[MAX_COMPONENTS];
    t_convcode code1;
    t_convcode code2;
    t_turbocode turbo;
    int N_components_upper = 2;
    int N_components_lower = 1;

    char *backward;
    backward = "0011";


    switch (code_type){

        case 1:
            N_components_upper = 2;
            N_components_lower = 1;


            forward_upper[0] = "10011"; // systematic output
            forward_upper[1] = "11011";

            forward_lower[0] = "11011";
            // need to define puncturing pattern here maybe with a pointer to function
            // 110 101 110 101 110 101

            code1 = convcode_initialize(forward_upper, backward, N_components_upper);
            code2 = convcode_initialize(forward_lower, backward, N_components_lower);
            turbo = turbo_initialize(code1, code2, pi, info_length, &puncturing);
            break;

        case 2:
            N_components_upper = 2;
            N_components_lower = 1;

            forward_upper[0] = "10011"; // systematic output
            forward_upper[1] = "11011";

            forward_lower[0] = "11011"; // no need for puncturing

            code1 = convcode_initialize(forward_upper, backward, N_components_upper);
            code2 = convcode_initialize(forward_lower, backward, N_components_lower);
            turbo = turbo_initialize(code1, code2, pi, info_length, NULL);
            break;

        case 3:
            N_components_upper = 3;
            N_components_lower = 1;

            forward_upper[0] = "10011"; // systematic output
            forward_upper[1] = "10101";
            forward_upper[2] = "11111";

            forward_lower[0] = "11011"; // no need for puncturing

            code1 = convcode_initialize(forward_upper, backward, N_components_upper);
            code2 = convcode_initialize(forward_lower, backward, N_components_lower);
            turbo = turbo_initialize(code1, code2, pi, info_length, NULL);
            break;

        case 4:
            N_components_upper = 4;
            N_components_lower = 2;

            forward_upper[0] = "10011"; // systematic output
            forward_upper[1] = "11011";
            forward_upper[2] = "10101";
            forward_upper[3] = "11111";

            forward_lower[0] = "11011"; // no need for puncturing
            forward_lower[1] = "11111";

            code1 = convcode_initialize(forward_upper, backward, N_components_upper);
            code2 = convcode_initialize(forward_lower, backward, N_components_lower);
            turbo = turbo_initialize(code1, code2, pi, info_length, NULL);
            break;
    }


    double rate = 1.0f/N_components_upper;

    // get noise std variation from SNR
    for (int i = 0; i < SNR_points; i++)
    {
        sigma[i] = sqrt(1/ pow(10, SNR_dB[i]/10));
        EbN0[i] = 1 / (2*rate*pow(sigma[i], 2));
    }

    int max_cores = omp_get_num_procs();
    int max_threads = omp_get_max_threads();

    // simulation loop
    // initialize seed of RNG/*{{{*/
    srand(time(NULL));

    // number of erroneous bits for each tested packet
    int packet_count = 0;
    int interval = num_packets * 0.05 + 1;

    omp_set_num_threads(cores);
    #pragma omp parallel
    {
        #pragma omp for nowait
        for (int k = 0; k < num_packets; k++)
        {
            packet_count++;
            // generate packet
            int *packet = randbits(info_length);

            if (! (packet_count % interval))
                printf("Processing packet #%d/%d\n", packet_count, num_packets);

            //double *noise_sequence = randn(0, 1, packet_length);
            double *noise_seq_coded = randn(0, 1, turbo.encoded_length);

            for (int s = 0; s < SNR_points; s++){
//                errors[s] += simulate_conv(packet, noise_seq_coded, info_length, sigma[s], code);
                errors[s] += simulate_turbo(packet, noise_seq_coded, info_length, sigma[s], turbo, iterations);
                erroneous_packets[s] += errors[s] != 0;
            }

            free(packet);
            //free(noise_sequence);
            free(noise_seq_coded);
        }
    }/*}}}*/

    // compute BER and PER
    for (int i = 0; i < SNR_points; i++)
    {

        PER[i] = (double) erroneous_packets[i]/(num_packets);
        BER[i] = (double) errors[i]/(num_packets*info_length);
    }

    printf(BOLDGREEN "\nSimulation completed.\n\n" RESET);

    // save results
    char *headers[] = {"SNR", "BER", "PER"};
    save_data(SNR_dB, BER, PER, headers, SNR_points, file);
    fclose(file);

    // print results
    printf(BOLDYELLOW "%20s%20s%20s\n" RESET, "SNR [dB]", "BER", "PER");
    for (int j = 0; j < SNR_points; ++j)
       printf("%20f%20.4e%20.4e\n", SNR_dB[j], BER[j], PER[j]);

    // release allocated memory
    free(BER);
    free(PER);
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

//    int *decoded = convcode_decode(received, encoded_length, code);
    double **a_priori = malloc(2*sizeof(double*));
    for (int k = 0; k < 2; ++k) {
        a_priori[k] = malloc(packet_length * sizeof(double));
        for (int i = 0; i < packet_length; ++i) {
            a_priori[k][i] = log(0.5);
        }
    }
    int *decoded = convcode_extrinsic(received, encoded_length, &a_priori, code, sigma*sigma, 1);
    for (int j = 0; j < packet_length; ++j)
        errors += (decoded[j] != packet[j]);

    free(decoded);
    free(encoded);
    free(received);
    return errors;/*}}}*/
}

int simulate_turbo(int *packet, double *noise_sequence, int packet_length, double sigma, t_turbocode code, int iterations)
{
    int errors = 0;/*{{{*/
    int *encoded = turbo_encode(packet, code);
    int encoded_length = code.encoded_length;

    double *received = malloc(encoded_length * sizeof *received);
    for (int i = 0; i < encoded_length; i++)
        received[i] = (2*encoded[i] - 1) + sigma*noise_sequence[i];

    int *decoded = turbo_decode(received, iterations, sigma*sigma, code);
    for (int j = 0; j < packet_length; ++j)
        errors += (decoded[j] != packet[j]);

    free(decoded);
    free(encoded);
    free(received);
    return errors;/*}}}*/
}
