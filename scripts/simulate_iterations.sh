#!/bin/bash
PKT_COUNT=100
MIN_SNR=-2
MAX_SNR=1
SNR_POINTS=10
for i in 1 2 3
do
    FILENAME="${PKT_COUNT}pkts_${i}iter.csv"
    time ../bin/deepspace_turbo -y -m $MIN_SNR -M  $MAX_SNR -n $SNR_POINTS -i $i -o $FILENAME -c $PKT_COUNT
done
