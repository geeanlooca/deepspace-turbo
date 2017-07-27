# deepspace-turbo
Implementation of the [CCSDS](https://public.ccsds.org/default.aspx) (Consultative Committee for Space Data Systems) 131.0-B-2 standard [_TM Synchronization and Channel Coding_](https://github.com/geeanlooca/deepspace-turbo/blob/master/standard.pdf) for Turbo Codes.

---

This is the code I wrote for the final project of the Channel Coding course at the University of Padova (Italy). The goal was to test the performance (both bit-error-rate and packet-error-rate) of an assigned channel coding standard in a AWGN scenario. The slides used for the presentation, along with their LaTeX source code, are available in `presentation/`. 

The libraries I wrote for both convolutional and Turbo Codes are fairly flexible, in the sense that they can handle a generic code given by the user.
Of course, since this was initially just an implementation of the already mentioned standard, the `main.c` file is written with that in mind, meaning that it will contain the initialization of the codes defined in the CCSDS' document and the lines of code to assess their performance.

## Instructions

### Dependencies
- gcc / clang
- OpenMP (only needed in the `main.c` file to speed up the simulation)
- make

If you use [CLion](https://www.jetbrains.com/clion/) as an IDE you can directly import the project and compile/build it from there. In any other case, either compile every single source code or wait for a decent Makefile :)

For the presentation, I used the Beamer theme [Metropolis](https://github.com/matze/mtheme). Refer to the previous link for instructions.

---

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

// initialize convolutional code: mandatory call
t_convcode code = convcode_initialize(forward, backward, N_components);
```

The code is defined by the strings of `1`'s and `0`'s in `forward` and `backward`. The function `convcode_initialize()` computes the state-update and output functions and allocates the necessary memory. The rate of the resulting code will be `1/N_components`. 

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

// generate antipodal symbols and add noise
for (int i = 0; i < encoded_length; i++)
    received[i] = (2*encoded_packet[i] - 1) + noise_sequence[i];

int *decoded = convcode_decode(received_signal, packet_length, code);

```
The function `randn` returns an array of a given length containing independent and identically distributed samples from a Gaussian distribution with given mean and variance.

#### BCJR algorithm
This algorithm isn't useful for plain convolutional decoding, as its performance are identical to those of Viterbi's algorithm, but with a higher complexity. It might be used when we have prior knowledge on certain bits, or if we need the posterior probabilities on the decoded bits. On the other hand, this algorithm is the fundamental building block for the decoding of Turbo Codes.

The BCJR decoding is performed by the `convcode_extrinsic` function, which takes a `2-by-packet_length` matrix containing the logarithm of the A Priori Probabilities (APP) on the packet, both for bit `0` and bit `1`.


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

We can decide wheter we want the function to return the decoded packet or just the posterior probabilities. This is done by setting `perform_decision` to `1`, while setting it to `0` will cause the function to skip the decision process and just compute the posterior probabilities. Note that every cell of the `a_priori` matrix was initialized to `log(0.5)`, indicating that we have no prior knowledge on the bits (they can be either `0` or `1` with probability 0.5).

## Turbo Codes
[Turbo codes](https://en.wikipedia.org/wiki/Turbo_code) are powerful codes that are built by concatenating two (or more) convolutional codes in parallel. These convolutional codes are fed with different versions of the input packet, built by scrambling its symbols according to a certain rule, defined by an interleaving function.

As of now, the library only supports binary Turbo Codes with two inner convolutional codes: we define the **upper code** and the **lower code**. The first is fed with the original input packet, while the second with its interleaved version.

### Building an interleaver
An interleaver takes the original uncoded packet as input and returns it scrambled. The rule according to which the scrambling is done is arbitrary, but keep in mind that the performance of the code greatly depends on the quality of the interleaver.

In terms of programming, the interleaver is just an array: the bit in position `i` of the interleaved packet is the bit in position `interleaver[i]` of the original packet.
As an example, we build an interleaver that just flips the input packet
```C
int *interleaver = malloc(packet_length * sizeof *interleaver);
for (int i = 0; i < packet_length; i++){
    interleaver[i] = packet_length - 1 - i;
}
```

### Defining a code
Now that we built an interleaver, we need to define the two convolutional codes that are used by the Turbo Code
```C
// define first code
int N_components_upper = 2;
char *forward_upper[N_components_upper];
forward_upper[0] = "1001";
forward_upper[1] = "1010";

char *backward_upper;
backward_upper = "011";

// define second code
int N_components_lower = 3;
char *forward_lower[N_components_lower];
forward_lower[0] = "1001";
forward_lower[1] = "1010";
forward_lower[2] = "1110";

char *backward_lower;
backward_lower = "110";

// initialize convolutional codes: mandatory call
t_convcode upper_code = convcode_initialize(forward_upper, backward_upper, N_components_upper);
t_convcode lower_code = convcode_initialize(forward_lower, backward_lower, N_components_lower);
```
After defining the components, we can initialize the Turbo Code in the following way
```C
t_turbocode turbo = turbo_initialize(upper_code, lower_code, interleaver, packet_length);
```
Notice that we already pass the input packet length to the initialization function. This means that the code (along with the interleaver) must be redefined if we want to change this parameter.

### Encoding and decoding
These two operations are fairly straightforward. We illustrate them with the following piece of code

```C
// encode the packet
int *encoded = turbo_encode(packet, turbo);
int encoded_length = code.encoded_length;

// generate PAM symbols and add noise
double *received = malloc(encoded_length * sizeof *received);
for (int i = 0; i < encoded_length; i++)
    received[i] = (2*encoded[i] - 1) + noise_sequence[i];

int *decoded = turbo_decode(received, iterations, sigma*sigma, code);
```

