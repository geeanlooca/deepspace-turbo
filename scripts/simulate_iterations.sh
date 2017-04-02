#!/bin/bash
# PKT_COUNT=10000
PKT_COUNT=10
MIN_SNR=-3
MAX_SNR=1
SNR_POINTS=10
TIMESTAMP=$(date +%F-%H-%M-%S)

# create results directory
mkdir -p results

# create directory for this specific simulation
DIR="./results/$TIMESTAMP"
mkdir -p $DIR

# log file to save simulation time
LOG="$DIR/time.log"

# simulate with different number of iterations
for i in 1 2 3 5
do
    # save results in the following file
    FILENAME="$DIR/${PKT_COUNT}pkts_${i}iter.csv"

    # run simulator
    /usr/bin/time -f "%E" -a -o $LOG ../bin/deepspace_turbo -y -m $MIN_SNR -M  $MAX_SNR -n $SNR_POINTS -i $i -o $FILENAME -c $PKT_COUNT > /dev/null

done
