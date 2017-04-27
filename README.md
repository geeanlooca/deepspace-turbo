# deepspace-turbo
Implementation of the [CCSDS](https://public.ccsds.org/default.aspx) (Consultative Committee for Space Data Systems) 131.0-B-2 standard [_TM Synchronization and Channel Coding_](https://github.com/geeanlooca/deepspace-turbo/blob/master/standard.pdf) for Turbo Codes.

This is the code I wrote as the final project for the Channel Coding course at the University of Padova (Italy). The goal was to test the performance (both bit-error-rate and packet-error-rate) of an assigned channel coding standard in a AWGN scenario.

The libraries I wrote for both convolutional and turbo codes are fairly flexible, in the sense that they can handle a generic code given by the user.

## Convolutional Codes
[Convolutional codes](https://en.wikipedia.org/wiki/Convolutional_code) are essentially discrete-time filters that work on a binary field. Although they can be defined on any [finite field](https://en.wikipedia.org/wiki/Finite_field), I only considered the binary case.

To define a new code, the following code snippet can be used:

```C
    int N_components = 2;
    char *forward[N_components];
    forward[0] = "10011";
    forward[1] = "10101";

    char *backward;
    backward = "0011";
    double rate = 1.0f/N_components;

    // get noise std variation from SNR
    for (int i = 0; i < SNR_points; i++)
    {
        sigma[i] = sqrt(1/ pow(10, SNR_dB[i]/10));
        EbN0[i] = 1 / (2*rate*pow(sigma[i], 2));
    }

    // initialize turbocode: mandatory call
    t_convcode code = convcode_initialize(forward, backward, N_components);
```
