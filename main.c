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

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */



// thread routines
int simulate_awgn(int *packet, double *noise_sequence, int packet_length, double sigma);
int simulate_conv(int *packet, double *noise_sequence, int packet_length, double sigma, t_convcode code);


int main(int argc, char *argv[])
{
    // option flags
    int skipconfirm_flag = 0;
    int filename_flag = 0;

    // default simulation parameters
    int packet_length = (int) 1e4;
    int num_packets = (int) 1e2;

    int SNR_points = 10;
    float min_SNR = -4;
    float max_SNR = 6;
    int cores = 4;

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
                        {"help",            no_argument,        0,  'h'},
                        {0, 0, 0, 0}
                };

        int option_index = 0;

        c = getopt_long(argc, argv, "yhl:c:C:m:M:f:b:o:n:", long_options, &option_index);

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

                printf(BOLDMAGENTA "%20s" RESET "\t%s\n\n" , "-y / --skip-confirm", "skip confirmation dialog after"
                        " summarizing simulation parameters. Useful when automating simulations.");

                printf(BOLDMAGENTA "%20s" RESET "\t%s\n\n" , "-h / --help", "print this help dialog.");

                printf(BOLDMAGENTA "%20s" RESET "\t%s\n\n" , "-o / --output FILENAME", "save results in "
                        "a comma-separated format in FILENAME. If this argument is not used, the results will be saved in "
                        "a file named with the current date and time.");

                printf(BOLDMAGENTA "%20s" RESET "\t%s\n\n" , "-c / --packet-count INTEGER", "set the number of packets to encode/decode."
                        " INTEGER can be given in exponential notation i.e. 1e4 for 10000 packets.");

                printf(BOLDMAGENTA "%20s" RESET "\t%s\n\n" , "-l / --packet-length INTEGER", "set the number of information bits"
                        " in a packet. Exponential notation can be used.");

                printf(BOLDMAGENTA "%20s" RESET "\t%s\n\n" , "-m / --min-SNR FLOAT", "set the lower extreme of the SNR range to test. ");

                printf(BOLDMAGENTA "%20s" RESET "\t%s\n\n" , "-M / --max-SNR FLOAT", "set the upper extreme of the SNR range to test. ");

                printf(BOLDMAGENTA "%20s" RESET "\t%s\n\n" , "-n / --SNR-points INTEGER", "set the number of linearly spaced points "
                        "inside the interval [min-SNR, max-SNR]");

                exit(EXIT_SUCCESS);

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

    // handle filename
    if (!filename_flag){
        // generate timestamp filename.
        struct tm *time_;
        time_t now = time(NULL);
        time_ = localtime(&now);
        strftime(filename, sizeof(filename), "%Y-%m-%d_%H-%M-%S.csv", time_);

        printf("Output filename not provided. File" BOLDMAGENTA "\'%s\'" RESET " will be used. \n", filename);
    }

    // print simulation parameters
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
    long int *errors = calloc(SNR_points, sizeof(long int));
    double *BER = malloc(SNR_points*sizeof *BER);

    // define code
    int N_components = 2;
    char *forward[N_components];
    forward[0] = "1101";
    forward[1] = "1011";
//    forward[3] = "11111";

    char *backward;
    backward = "000";

    double rate = 1.0f/N_components;


    // get noise std variation from SNR
    for (int i = 0; i < SNR_points; i++)
    {
        sigma[i] = sqrt(1/ pow(10, SNR_dB[i]/10));
        EbN0[i] = 1 / (2*rate*pow(sigma[i], 2));
    }


    // initialize code: mandatory call
    t_convcode code = convcode_initialize(forward, backward, N_components);


    // build interleaver
    int octets = 1;
    int base = 223;
    int info_length = base * 8 * octets;
    int p[8] = {31, 37, 43, 47, 53, 59, 61, 67};
    int k1 = 8;
    int k2 = base * octets;

    int **interleaver = malloc(2 * sizeof(int*));
    int *pi = malloc(info_length * sizeof *pi);
    int *identity = malloc(info_length * sizeof *identity);

    for (int s = 1; s <= info_length; ++s) {
        int m = (s-1) % 2;
        int i = (int) floor((s-1) / (2 * k2));
        int j = (int) floor((s-1) / 2) - i*k2;
        int t = (19*i + 1) % (k1/2);
        int q = t % 8 + 1;
        int c = (p[q-1]*j + 21*m) % k2;
        pi[s-1] = 2*(t + c*(k1/2) + 1) - m;
        identity[s-1] = s-1;
    }

    interleaver[0] = identity;
    interleaver[1] = pi;

    print_array_int(pi, info_length);

    t_convcode codes[2] = {code, code};
    t_turbocode turbo = turbo_initialize(codes, 2, interleaver, info_length);

    // create packet
    int *pkt = randbits(info_length);
    turbo_encode(pkt, turbo);

    exit(EXIT_SUCCESS);
//
    // simulation loop
    // initialize seed of RNG
    srand(time(NULL));

    // number of erroneous bits for each tested packet
    int packet_count = 0;
    int interval = (int) num_packets * 0.05 + 1;

    omp_set_num_threads(4);
    #pragma omp parallel
    {
        #pragma omp for nowait
        for (int k = 0; k < num_packets; k++)
        {
            packet_count++;
            // generate packet
            int *packet = randbits(packet_length);

            if (! (packet_count % interval))
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
    for (int i = 0; i < SNR_points; i++)
        BER[i] = (double) errors[i]/(num_packets*packet_length);

    printf(BOLDGREEN "\nSimulation completed.\n\n" RESET);

    // save results
    char *headers[] = {"SNR", "BER"};
    save_data(SNR_dB, BER, headers, SNR_points, file);
    fclose(file);

    // print results
    printf(BOLDYELLOW "%20s%20s\n" RESET, "SNR [dB]", "BER");
    for (int j = 0; j < SNR_points; ++j)
       printf("%20f%20.4e\n", SNR_dB[j], BER[j]);

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

//    int *decoded = convcode_decode(received, encoded_length, code);
    int *decoded = convcode_extrinsic(received, encoded_length, NULL, code, sigma*sigma);
    for (int j = 0; j < packet_length; ++j)
        errors += (decoded[j] != packet[j]);

    free(decoded);
    free(encoded);
    free(received);
    return errors;/*}}}*/
}
