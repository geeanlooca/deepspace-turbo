# deepspace-turbo
Implementation of the [CCSDS](https://public.ccsds.org/default.aspx) (Consultative Committee for Space Data Systems) 131.0-B-2 standard [_TM Synchronization and Channel Coding_](https://github.com/geeanlooca/deepspace-turbo/blob/master/standard.pdf) for Turbo Codes.

This is the code I wrote as the final project for the Channel Coding course at the University of Padova (Italy). The goal was to test the performance (both bit-error-rate and packet-error-rate) of an assigned channel coding standard in a AWGN scenario.

The libraries I wrote for both convolutional and turbo codes are fairly flexible, in the sense that they can handle a generic code given by the user.

## Convolutional Codes
[Convolutional codes](https://en.wikipedia.org/wiki/Convolutional_code) are essentially discrete-time filters that work on a binary field. Although they can be defined on any [finite field](https://en.wikipedia.org/wiki/Finite_field), I only considered the binary case.


### Defining a code
To define a new code, the following code snippet can be used:

```C
    int N_components = 2;
    char *forward[N_components];
    forward[0] = "1001";
    forward[1] = "1010";

    char *backward;
    backward = "011";

    // initialize turbocode: mandatory call
    t_convcode code = convcode_initialize(forward, backward, N_components);
```

The code is defined by the strings of `1`'s and `0`'s in `forward` and `backward`. The function `convcode_initialize()` computes the state-update and output functions and allocates the necessary memory.


### Encoding
To encode a packet, we can simply do
```C
    int packet_length = 1000;
    int *packet = randbits(packet_length);
    int *encoded_packet = convcode_encode(packet, packet_length, code);
    int encoded_length = code.components*(packet_length + code.memory);
```

Function `randbits` simply generates an array of `0`'s and `1`'s of a given length, and is implemented in `utilities.c`. The length of the encoded packet is contained in `encoded_length`.

### Decoding
There are two algorithms that can be used for decoding a received signal:
* [Viterbi algorithm](https://en.wikipedia.org/wiki/Viterbi_decoder), implements optimal maximum-likelihood decoding.
* Forward-Backward algorithm (or [BCJR](http://ieeexplore.ieee.org/document/1055186/)), implements the MAP criterion.

When using a pure convolutional code, the first algorithm should be preferred. 

#### Viterbi algorithm
An example of modulating/trasmitting an encoded packet and then decoding it using the Viterbi algorithm is

```C
    // generate Gaussian noise with 0 mean and unit variance
    double *noise_sequence = randn(0, sigma, packet_length);

    double *received_signal = malloc(encoded_length * sizeof *received_signal);

    // generate PAM symbols and add noise
    for (int i = 0; i < encoded_length; i++)
        received[i] = (2*encoded_packet[i] - 1) +noise_sequence[i];

    int *decoded = convcode_decode(received_signal, packet_length, code);

```

#### BCJR algorithm
This algorithm isn't useful for plain convolutional decoding, as its performance are identical to those of Viterbi's algorithm, but with a higher complexity. It might be used when we have prior knowledge on certain bits, or if we need the posterior probabilities on the decoded bits.

The BCJR decoding is performed by the `convcode_extrinsic` function, which takes a `2-by-packet_length`-size matrix containing the logarithm of the A Priori Probabilities (APP) on the packet, both for bit `0` and bit `1`.


A snipped illustrating its use is given below.
```C
    // build a priori probabilities on bits
    double **a_priori = malloc(2*sizeof(double*));
    for (int k = 0; k < 2; ++k) {
        a_priori[k] = malloc(packet_length * sizeof(double));
        for (int i = 0; i < packet_length; ++i) {
            a_priori[k][i] = log(0.5);
        }
    }

    int perform_decision = 1;

    int *decoded = convcode_extrinsic(received_signal, encoded_length,
    &a_priori, code, sigma*sigma, perform_decision);
```

We can decide wheter we want the function to return the decoded packet or just the posterior probabilities. This is done by assigning `1` to `perform_decision`. The posterior probabilities are saved in the same matrix passed as an argument (`a_priori` in this case). 

