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
```

Function `randbits` simply generates an array of `0`'s and `1`'s of a given length, and is implemented in `utilities.c`.

### Decoding
There are two algorithms that can be used for decoding a received signal:
* Viterbi algorithm
* Forward-Backward algorithm (or BCJR)

